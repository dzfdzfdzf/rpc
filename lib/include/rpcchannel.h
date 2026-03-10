#pragma once
#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include<google/protobuf/message.h>
#include<string>
#include<unordered_map>
#include<mutex>

class RpcChannel:public google::protobuf::RpcChannel{
    public:
        void CallMethod(const google::protobuf::MethodDescriptor*method, 
        google::protobuf::RpcController*controller,
        const google::protobuf::Message*request,
        google::protobuf::Message*response,
        google::protobuf::Closure*done);
    
    private:
        // 服务地址缓存
        std::unordered_map<std::string, std::string> m_serviceAddressCache;
        // 缓存锁
        std::mutex m_cacheMutex;
        // 从ZooKeeper获取服务地址（带缓存）
        std::string getServiceAddress(const std::string& serviceName, const std::string& methodName, google::protobuf::RpcController* controller);
        // Watch回调函数
        void onServiceAddressChanged(int type, int state, const char* path);
};