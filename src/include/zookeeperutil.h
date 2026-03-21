#pragma once
#include<semaphore.h>
#include<zookeeper/zookeeper.h>
#include<string>
#include<functional>
#include<thread>
#include<queue>
#include<mutex>
#include<condition_variable>

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
        // 检查是否有watch事件
        bool ProcessWatchEvent(int timeout_ms = 100);
    private:
        zhandle_t *m_zhandle;
        WatchCallback m_watchCallback;
        std::thread m_watcherThread;
        std::queue<std::tuple<int, int, std::string>> m_eventQueue;
        std::mutex m_queueMutex;
        std::condition_variable m_queueCV;
        bool m_stopWatcher;
        
        // 静态watch回调函数
        static void GlobalWatcher(zhandle_t *zh, int type, int state, const char *path, void *ctx);
        // watcher线程函数
        void WatcherThreadFunc();
};