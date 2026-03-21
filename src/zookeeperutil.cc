#include "zookeeperutil.h"
#include "rpcapplication.h"
#include <semaphore.h>
#include<iostream>

// 全局watcher函数 - 用于session连接
void global_watcher(zhandle_t*zh,int type,int state,const char* path,void *watcherCtx){
    if(type==ZOO_SESSION_EVENT){
        if(state==ZOO_CONNECTED_STATE){
            sem_t *sem=(sem_t*)zoo_get_context(zh);
            sem_post(sem);
        }
    }
}

// ZkClient的全局watch回调 - 用于监听节点变化
void ZkClient::GlobalWatcher(zhandle_t *zh, int type, int state, const char *path, void *ctx) {
    ZkClient* client = static_cast<ZkClient*>(ctx);
    if (client == nullptr) {
        return;
    }
    
    // 忽略session事件，只处理节点变化
    if (type == ZOO_CHANGED_EVENT || type == ZOO_DELETED_EVENT || type == ZOO_CREATED_EVENT) {
        std::cout << "[ZooKeeper Watch] Event received: type=" << type 
                  << ", state=" << state << ", path=" << (path ? path : "null") << std::endl;
        
        // 将事件放入队列
        std::lock_guard<std::mutex> lock(client->m_queueMutex);
        client->m_eventQueue.emplace(type, state, path ? path : "");
        client->m_queueCV.notify_one();
    }
}

// Watcher线程函数
void ZkClient::WatcherThreadFunc() {
    while (!m_stopWatcher) {
        // 等待事件或超时
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_queueCV.wait_for(lock, std::chrono::milliseconds(100), [this] {
            return !m_eventQueue.empty() || m_stopWatcher;
        });
        
        if (m_stopWatcher) {
            break;
        }
        
        // 处理所有待处理的事件
        while (!m_eventQueue.empty()) {
            auto event = m_eventQueue.front();
            m_eventQueue.pop();
            lock.unlock();
            
            int type = std::get<0>(event);
            int state = std::get<1>(event);
            std::string path = std::get<2>(event);
            
            // 调用用户的回调函数
            if (m_watchCallback) {
                m_watchCallback(type, state, path.c_str());
            }
            
            lock.lock();
        }
    }
}

ZkClient::ZkClient():m_zhandle(nullptr), m_stopWatcher(false)
{

}
ZkClient::~ZkClient(){
    // 停止watcher线程
    m_stopWatcher = true;
    m_queueCV.notify_one();
    if (m_watcherThread.joinable()) {
        m_watcherThread.join();
    }
    
    if(m_zhandle!=nullptr){
        zookeeper_close(m_zhandle);
    }
}
void ZkClient::Start(){
    std::string host=RpcApplication::GetInstance().GetConfig().Load("zookeeperip");
    std::string port=RpcApplication::GetInstance().GetConfig().Load("zookeeperport");
    std::string   connstr=host+":"+port;
    // 设置全局watcher，传入this作为上下文
    m_zhandle=zookeeper_init(connstr.c_str(),global_watcher,30000,nullptr,this,0);
    if(nullptr==m_zhandle){
        std::cout<<"zookeeper init error";
        exit(EXIT_FAILURE);
    }
    sem_t sem;
    sem_init(&sem,0,0);
    zoo_set_context(m_zhandle,&sem);
    sem_wait(&sem);
    std::cout<<"zookeeper init success"<<std::endl;
    
    // 启动watcher线程
    m_watcherThread = std::thread(&ZkClient::WatcherThreadFunc, this);
}
void ZkClient::Create(const char*path,const char*data,int datalen,int state){
    char path_buffer[128];
    int bufflen=sizeof(path_buffer);
    int flag;
    flag=zoo_exists(m_zhandle,path,0,nullptr);
    if(ZNONODE==flag){
        flag=zoo_create(m_zhandle,path,data,datalen,&ZOO_OPEN_ACL_UNSAFE,state,path_buffer,bufflen);
        if(flag==ZOK){
            std::cout<<"znode create success path:"<<path<<std::endl;
        }
        else{
            std::cout<<flag<<std::endl;
            std::cout<<"znode create error path:"<<path<<std::endl;
            exit(EXIT_FAILURE);

        }
    }
}

// 带watch的GetData方法 - 每次调用都会重新设置watch
std::string ZkClient::GetData(const char* path, WatchCallback watcher){
    if(watcher){
        m_watchCallback = watcher;
    }
    
    char buffer[128];
    int bufferlen=sizeof(buffer);
    Stat stat;
    
    // 每次获取数据时都重新设置watch（ZooKeeper的watch是一次性的）
    int flag=zoo_wget(m_zhandle,path,GlobalWatcher,this,buffer,&bufferlen,&stat);
    if(flag!=ZOK){
        // 如果节点不存在，尝试创建
        if(flag == ZNONODE) {
            std::cout<<"znode not exist, trying to create: "<<path<<std::endl;
            return "";
        }
        std::cout<<"get znode error! path:"<<path<<" flag:"<<flag<<std::endl;
        return "";
    }
    else {
        std::cout<<"get znode success! path:"<<path<<" data:"<<buffer<<" (watch registered)"<<std::endl;
        return buffer;
    }
}

// 处理watch事件（可选调用，用于主动触发回调）
bool ZkClient::ProcessWatchEvent(int timeout_ms) {
    std::unique_lock<std::mutex> lock(m_queueMutex);
    if (m_queueCV.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this] {
        return !m_eventQueue.empty();
    })) {
        if (!m_eventQueue.empty()) {
            auto event = m_eventQueue.front();
            m_eventQueue.pop();
            lock.unlock();
            
            int type = std::get<0>(event);
            int state = std::get<1>(event);
            std::string path = std::get<2>(event);
            
            if (m_watchCallback) {
                m_watchCallback(type, state, path.c_str());
            }
            return true;
        }
    }
    return false;
}

// 设置watch回调
void ZkClient::SetWatchCallback(WatchCallback callback){
    m_watchCallback = callback;
}
