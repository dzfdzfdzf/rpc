#include<iostream>

#include"rpcapplication.h"

#include"user.pb.h"

int main(int argc,char **argv){
    RpcApplication::Init(argc,argv);
    fixbug::UserServiceRpc_Stub stub(new RpcChannel());
    fixbug::LoginRequest request;
    request.set_name("belly");
    request.set_pwd("123456");
    fixbug::LoginResponse response;
    RpcController controller;
    stub.Login(&controller,&request,&response,nullptr);
    if(controller.Failed()){
        std::cout<<controller.ErrorText()<<std::endl;
    }
    else{

    
        if(0==response.result().errcode()){
            std::cout<<"rpc  login response success"<<std::endl;
        }
        else {
            std::cout<<"rpc login error"<<std::endl;
        }
        fixbug::RegisterRequest req;
        req.set_id(200);
        req.set_name("belly");
        req.set_pwd("020412");
        fixbug::RegisterResponse rsp;
        stub.Register(nullptr,&req,&rsp,nullptr);
        if(0==rsp.result().errcode()){
            std::cout<<"rpc register response success"<<rsp.success()<< std::endl;
        }
        else {
            std::cout<<"rpc register error"<<std::endl;
        }
    }
    return 0;
}