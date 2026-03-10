#include "rpcconfig.h"
#include<iostream>
void RpcConfig::Trim(std::string &src_buf){
     int idx=src_buf.find_first_not_of(' ');
    if(idx!=-1){
        src_buf=src_buf.substr(idx,src_buf.size()-idx);
    }
    idx=src_buf.find_last_not_of(' ');
    if(idx!=-1){
        src_buf=src_buf.substr(0,idx+1);
    }
}
std::string RpcConfig::Load(const std::string&key){
    if(m_configMap.find(key)==m_configMap.end()){
        return "";
    }
    return m_configMap[key]; 
}

void RpcConfig::LoadConfigFile(const char* config_file){
    FILE *pf=fopen(config_file,"r");
    if(pf==nullptr){
        std::cout<<config_file<<"does not exist!"<<std::endl;
        exit(EXIT_FAILURE);
    }
    while(!feof(pf)){
        char buf[512]={0};
        fgets(buf,512,pf);
        std::string src_buf(buf);
       Trim(src_buf);
        if(src_buf.empty()||src_buf[0]=='#'){
            continue;
        }
        int idx=src_buf.find('=');
        if(idx==-1){
            continue;
        }
        std::string key;
        std::string value;
        key=src_buf.substr(0,idx);
        Trim(key);
        int endidx=src_buf.find('\n',idx);
        value=src_buf.substr(idx+1,endidx-idx-1);
        Trim(value);
        m_configMap.insert({key,value});
    }
}  