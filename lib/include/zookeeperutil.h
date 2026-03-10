#pragma once
#include<semaphore.h>
#include<zookeeper/zookeeper.h>
#include<string>
#include<functional>

// Watch回调函数类型
typedef std::function<void(int type, int state, const char* path)> WatchCallback;

class ZkClient{
    public:
        ZkClient();
        ~ZkClient();
        void Start();
        void Create(const char*path,const char* data,int datalen,int state=0);
        std::string GetData(const char* path, WatchCallback watcher = nullptr);
        void SetWatchCallback(WatchCallback callback);
    private:
        zhandle_t *m_zhandle;
        WatchCallback m_watchCallback;
        static void Watcher(zhandle_t *zh, int type, int state, const char *path, void *watcherCtx);

};