#include "rpcchannel.h"
#include "rpccontroller.h"
#include<string>
#include "rpcheader.pb.h"
#include <sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<errno.h>
#include "rpcapplication.h"
#include <unistd.h>
#include "zookeeperutil.h"
#include<iostream>

// 构造函数
RpcChannel::RpcChannel(): m_zkClientInited(false) {
}

// 析构函数
RpcChannel::~RpcChannel() {
}

std::string RpcChannel::getServiceAddress(const std::string& serviceName, const std::string& methodName, google::protobuf::RpcController* controller){
    std::string method_path = "/" + serviceName + "/" + methodName;
    
    // 检查缓存
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        auto it = m_serviceAddressCache.find(method_path);
        if(it != m_serviceAddressCache.end()){
            return it->second;
        }
    }
    
    // 初始化ZooKeeper客户端（只初始化一次）
    if (!m_zkClientInited) {
        m_zkClient.Start();
        m_zkClientInited = true;
    }
    
    // 设置watch回调
    auto watcher = [this](int type, int state, const char* path) {
        this->onServiceAddressChanged(type, state, path);
    };
    
    std::string host_data = m_zkClient.GetData(method_path.c_str(), watcher);
    if(host_data == ""){
        controller->SetFailed(method_path + " is not exist!");
        return "";
    }
    
    // 更新缓存
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        m_serviceAddressCache[method_path] = host_data;
    }
    
    return host_data;
}

// Watch回调函数
void RpcChannel::onServiceAddressChanged(int type, int state, const char* path){
    std::cout << "Service address changed: " << path << " type: " << type << " state: " << state << std::endl;
    
    // 从缓存中删除对应的条目
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    auto it = m_serviceAddressCache.find(path);
    if(it != m_serviceAddressCache.end()){
        m_serviceAddressCache.erase(it);
        std::cout << "Removed cached address for: " << path << std::endl;
    }
}

void RpcChannel::CallMethod(const google::protobuf::MethodDescriptor*method, 
        google::protobuf::RpcController*controller,
        const google::protobuf::Message*request,
        google::protobuf::Message*response,
        google::protobuf::Closure*done){
            const google::protobuf::ServiceDescriptor*sd=method->service();
            std::string service_name=sd->name();
            std::string method_name=method->name();
            int args_size=0;
            std::string args_str;
            if(request->SerializeToString(&args_str)){
                args_size=args_str.size();
            }   
            else{
                controller->SetFailed("serialize error!");
                return;
            }
            rpc::RpcHeader rpcHeader; 
            rpcHeader.set_service_name(service_name);
            rpcHeader.set_method_name(method_name);
            rpcHeader.set_args_size(args_size);

            uint32_t header_size=0;
            std::string rpc_header_str;
            if(rpcHeader.SerializeToString(&rpc_header_str)){
                header_size=rpc_header_str.size();

            }
            else{
                controller->SetFailed("serialize error!");
                return;
            }
            std::string send_rpc_str;
            send_rpc_str.insert(0,std::string((char *)&header_size,4));
            send_rpc_str+=rpc_header_str;
            send_rpc_str+=args_str;
            std::cout<<"header_size:"<<header_size<<std::endl;
            std::cout<<"rpcheaderstr:"<<rpc_header_str<<std::endl;
            std::cout<<"service_name:"<<service_name<<std::endl;
            std::cout<<"method_name:"<<method_name<<std::endl;
            std::cout<<"args_size:"<<args_size<<std::endl;
            int clientfd=socket(AF_INET,SOCK_STREAM,0);
            if(clientfd==-1){
                char errortext[512]={0};
                sprintf(errortext,"errno is %d",errno);
                 controller->SetFailed(errortext);
                return;
            }
            // 从缓存或ZooKeeper获取服务地址
            std::string host_data = getServiceAddress(service_name, method_name, controller);
            if(host_data == ""){
                close(clientfd);
                return;
            }
            int idx=host_data.find(":");
            if(idx==-1){
                controller->SetFailed("address is invalid");
                close(clientfd);
                return;
            }
            std::string ip=host_data.substr(0,idx);
            uint16_t port=atoi(host_data.substr(idx+1,host_data.size()-idx).c_str());
            sockaddr_in server_addr;
            server_addr.sin_family=AF_INET;
            server_addr.sin_port=htons(port);
            server_addr.sin_addr.s_addr=inet_addr(ip.c_str());
            if(-1==connect(clientfd,(sockaddr*)&server_addr,sizeof(server_addr))){ 
                std::cout<<"connect error!"<<errno<<std::endl;
                close(clientfd);
                // 从缓存中删除失效的地址
                std::string method_path = "/" + service_name + "/" + method_name;
                {
                    std::lock_guard<std::mutex> lock(m_cacheMutex);
                    m_serviceAddressCache.erase(method_path);
                }
                return;
            }
            if(-1==send(clientfd,send_rpc_str.c_str(),send_rpc_str.size(),0)){
                std::cout<<"send error!"<<errno <<std::endl;
                close(clientfd);
                return;
            }
            char buf[2048]={0};
            int recv_size=0;
            if(-1==(recv_size=recv(clientfd,buf,2048,0))){ 
                std::cout<<"recv error!"<<errno <<std::endl;
                close(clientfd);
                return;
            }
            // std::string response_str(buf,0,recv_size);
            // if(!response->ParseFromString(response_str)){
            if(!response->ParseFromArray(buf,recv_size)){
                std::cout<<"parse error!"<<buf<<std::endl;
                close(clientfd);
                return;
            }
            close(clientfd);

        }
