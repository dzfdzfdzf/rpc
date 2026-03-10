#pragma once
#include "google/protobuf/service.h"
#include <memory>
#include <muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include<muduo/net/TcpConnection.h>
#include<string>
#include<functional>
#include<google/protobuf/descriptor.h>
#include<unordered_map>
#include "logger.h"
#include "zookeeperutil.h"
class RpcProvider{
    public:
        void NotifyService(google::protobuf::Service*service);
        void Run();
        
    private:
        struct ServiceInfo{
            google::protobuf::Service*m_service;
            std::unordered_map<std::string,const google::protobuf::MethodDescriptor*>m_methodMap;
        };
        std::unordered_map<std::string,ServiceInfo>m_serviceMap;
        muduo::net::EventLoop m_eventLoop;
        void OnConnection(const muduo::net::TcpConnectionPtr&conn);
        void OnMessage(const muduo::net::TcpConnectionPtr& conn,muduo::net::Buffer* b,muduo::Timestamp ts);
        void SendRpcResponse(const muduo::net::TcpConnectionPtr& conn,google::protobuf::Message*m);
};