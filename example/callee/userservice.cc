#include<iostream>
#include<string>

#include "user.pb.h"
#include "rpcapplication.h"
#include "rpcprovider.h"
#include "logger.h"
class UserService:public fixbug::UserServiceRpc
{

public:
    bool Login(std::string name,std::string pwd){
        std::cout<<name<<" "<<pwd<<std::endl;
        return true;
    }
     bool Register(uint32_t id,std::string name,std::string pwd){
        std::cout<<id<<" "<<name<<" "<<pwd<<std::endl;
        return true;
    }
   void Login(::google::protobuf::RpcController* controller,
                       const ::fixbug::LoginRequest* request,
                       ::fixbug::LoginResponse* response,
                       ::google::protobuf::Closure* done){
        std::string   name=request->name();
        std::string   pwd=request->pwd();
        bool login_result=Login(name,pwd);
        fixbug::ResultCode *code=response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("");
        response->set_success(login_result);
        done->Run();
    }
     void Register(::google::protobuf::RpcController* controller,
                       const ::fixbug::RegisterRequest* request,
                       ::fixbug::RegisterResponse* response,
                       ::google::protobuf::Closure* done){
        uint32_t id=request->id();
        std::string   name=request->name();
        std::string   pwd=request->pwd();
        bool register_result=Register(id,name,pwd);
        fixbug::ResultCode *code=response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("");
        response->set_success(register_result);
        done->Run();
    }
};


int main(int argc, char ** argv){
    LOG_INFO("first log message!\n");
    LOG_INFO("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
    RpcApplication::Init(argc,argv);
    RpcProvider provider;
    provider.NotifyService(new UserService());

    provider.Run();
    return 0;
}