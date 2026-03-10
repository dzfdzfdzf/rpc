#include "test.pb.h"
#include <iostream>
#include <string>
using namespace fixbug;
int main(){
    // LoginResponse rsp;
    // ResultCode*rc= rsp.mutable_result();
    // rc->set_errcode(0);
    // rc->set_errmsg("失败");
    GetFriendListsResponse rsp;
    ResultCode*rc= rsp.mutable_result();
    rc->set_errcode(0);
    User *usr1=rsp.add_friend_list();
    usr1->set_name("belly");
    usr1->set_age(11);
    usr1->set_sex(User::Man);

    User *usr2=rsp.add_friend_list();
    usr2->set_name("belly2");
    usr2->set_age(111);
    usr2->set_sex(User::Man);

    std::cout<<rsp.friend_list_size()<<std::endl;


    return 0;
}
int main1(){
    LoginRequest req;
    req.set_name("belly");
    req.set_pwd("111");

    std::string send_str; 
    if(req.SerializeToString(&send_str)){
        std::cout<<send_str.c_str()<<std::endl;
    }
    LoginRequest reqB;
    if(reqB.ParseFromString(send_str)){
        std::cout<<reqB.name()<<std::endl<<reqB.pwd()<<std::endl;
    }
    return 0;

}