#include "rpcprovider.h"
#include "zookeeperutil.h"
#include "rpcapplication.h"
#include "rpcheader.pb.h"
void RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr& conn,google::protobuf::Message*response){
    std::string response_str;
    if(response->SerializeToString(&response_str)){
        conn->send(response_str);
        
    }
    else{
        std::cout<<"serialize response_str error"<<std::endl;
    }
    conn->shutdown();
}
void RpcProvider::NotifyService(google::protobuf::Service*service){
    ServiceInfo service_info;
    const google::protobuf::ServiceDescriptor* pserviceDesc=service->GetDescriptor();

    std::string service_name=pserviceDesc->name();
    int methodCnt=pserviceDesc->method_count();
    std::cout<<"service name:"<<service_name<<std::endl;
    for(int i=0;i<methodCnt;++i){
       const google::protobuf::MethodDescriptor* pmethodDesc=pserviceDesc->method(i);
       std::string method_name=pmethodDesc->name();
       service_info.m_methodMap.insert({method_name,pmethodDesc});
        std::cout<<"method name:"<<method_name<<std::endl;
    }
    service_info.m_service=service;
    m_serviceMap.insert({service_name,service_info});
}
void RpcProvider::Run(){
    std::string ip=RpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    uint16_t port=atoi(RpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
    muduo::net::InetAddress address(ip,port); 
    muduo::net::TcpServer server(&m_eventLoop,address,"RpcProvider");
    server.setConnectionCallback(std::bind(&RpcProvider::OnConnection,this,std::placeholders::_1));
    server.setMessageCallback(std::bind(&RpcProvider::OnMessage,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
    server.setThreadNum(4);
   
    ZkClient zkCli;
    zkCli.Start();
    for(auto &sp:m_serviceMap){
        std::string service_path ="/"+sp.first;
        zkCli.Create(service_path.c_str(),nullptr,0);
        for(auto&mp:sp.second.m_methodMap){
            std::string method_path=service_path+"/"+mp.first;
            char method_path_data[128]={0};
            sprintf(method_path_data,"%s:%d",ip.c_str(),port);
            zkCli.Create(method_path.c_str(),method_path_data,strlen(method_path_data),ZOO_EPHEMERAL);
        }
    }
    std::cout<<"RpcProvider ip:"<<ip<<" Port:"<<port<<std::endl;
    server.start();
    m_eventLoop.loop();
}
void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr& conn){
    if(!conn->connected()){
        conn->shutdown();
    }
}
void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr& conn,muduo::net::Buffer* b,muduo::Timestamp ts){
    std::string recv_buffer=b->retrieveAllAsString();
    uint32_t header_size=0;
    recv_buffer.copy((char *)&header_size,4,0);
    std::string rpc_header_str=recv_buffer.substr(4,header_size);
    rpc::RpcHeader rpcHeader;
    std::string service_name;
    std::string method_name;
    uint32_t args_size;
   if( rpcHeader.ParseFromString(rpc_header_str))
   {
    service_name=rpcHeader.service_name();
    method_name=rpcHeader.method_name();
    args_size=rpcHeader.args_size();
   }
   else{
    std::cout<<"parse error!"<<std::endl;
    return;
   }
   std::string args_string=recv_buffer.substr(4+header_size,args_size);
   std::cout<<"header_size:"<<header_size<<std::endl;
   std::cout<<"rpcheaderstr:"<<rpc_header_str<<std::endl;
   std::cout<<"service_name:"<<service_name<<std::endl;
   std::cout<<"method_name:"<<method_name<<std::endl;
   std::cout<<"args_size:"<<args_size<<std::endl;
    auto it=m_serviceMap.find(service_name);
    if(it==m_serviceMap.end()){
        std::cout<<service_name<<"does not exist!"<<std::endl;
        return;
    }
    google::protobuf::Service *service=it->second.m_service;
    auto mit=it->second.m_methodMap.find(method_name);
    if(mit==it->second.m_methodMap.end()){
         std::cout<<service_name<<":"<<method_name<<"does not exist!"<<std::endl;
        return;
    }
    const google::protobuf::MethodDescriptor *method=mit->second;
    google::protobuf::Message*request=service->GetRequestPrototype(method).New();
    if(!request->ParseFromString(args_string)){
        std::cout<<"request parse error, content:"<<args_string<<std::endl;
        return;
    }
    google::protobuf::Message *response=service->GetResponsePrototype(method).New();
    google::protobuf::Closure*done= google::protobuf::NewCallback<RpcProvider,const muduo::net::TcpConnectionPtr&,google::protobuf::Message*>(this,&RpcProvider::SendRpcResponse,conn,response);
    service->CallMethod(method,nullptr,request,response,done);
}