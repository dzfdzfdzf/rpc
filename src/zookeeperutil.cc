#include "zookeeperutil.h"
#include "rpcapplication.h"
#include <semaphore.h>
#include<iostream>

// 全局watcher函数
void global_watcher(zhandle_t*zh,int type,int state,const char* path,void *watcherCtx){
    if(type==ZOO_SESSION_EVENT){
        if(state==ZOO_CONNECTED_STATE){
            sem_t *sem=(sem_t*)zoo_get_context(zh);
            sem_post(sem);
        }
    }
}

// 静态watcher方法
void ZkClient::Watcher(zhandle_t *zh, int type, int state, const char *path, void *watcherCtx) {
}

ZkClient::ZkClient():m_zhandle(nullptr)
{

}
ZkClient::~ZkClient(){
    if(m_zhandle!=nullptr){
        zookeeper_close(m_zhandle);
    }
}
void ZkClient::Start(){
    std::string host=RpcApplication::GetInstance().GetConfig().Load("zookeeperip");
    std::string port=RpcApplication::GetInstance().GetConfig().Load("zookeeperport");
    std::string   connstr=host+":"+port;
    m_zhandle=zookeeper_init(connstr.c_str(),global_watcher,30000,nullptr,nullptr,0);
    if(nullptr==m_zhandle){
        std::cout<<"zookeeper init error";
        exit(EXIT_FAILURE);
    }
    sem_t sem;
    sem_init(&sem,0,0);
    zoo_set_context(m_zhandle,&sem);
    sem_wait(&sem);
    std::cout<<"zookeeper init success";
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

// 带watch的GetData方法
std::string ZkClient::GetData(const char* path, WatchCallback watcher){
    if(watcher){
        m_watchCallback = watcher;
    }
    
    char buffer[64];
    int bufferlen=sizeof(buffer);
    int flag=zoo_get(m_zhandle,path,1,buffer,&bufferlen,nullptr);
    if(flag!=ZOK){
        std::cout<<"get znode error! path:"<<path<<std::endl;
        return "";
    }
    else return buffer;
}

// 设置watch回调
void ZkClient::SetWatchCallback(WatchCallback callback){
    m_watchCallback = callback;
}
