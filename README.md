# RPC 分布式RPC框架

基于C++11开发的高性能分布式RPC框架，支持服务注册与发现、负载均衡、服务监控等功能。

## 项目特点

- **基于Protobuf**: 使用Protocol Buffers进行高效的数据序列化
- **ZooKeeper服务发现**: 支持服务自动注册与发现
- **服务地址缓存**: 减少ZooKeeper查询开销，提升性能
- **Watch机制**: 实时感知服务地址变化，自动更新缓存
- **高性能网络**: 基于muduo网络库实现高并发通信
- **异步日志**: 支持异步日志记录，不影响主业务性能

## 架构设计

```
┌─────────────────────────────────────────────────────────────┐
│                      RPC Framework                          │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   ┌──────────────┐      ┌──────────────┐                   │
│   │   Consumer   │      │   Provider   │                   │
│   │  (服务消费者) │      │  (服务提供者) │                   │
│   └──────────────┘      └──────────────┘                   │
│          │                     │                            │
│          │     ┌─────────┐     │                            │
│          └────▶│  ZK     │◀────┘                            │
│                │ Cluster │                                  │
│                └─────────┘                                  │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## 目录结构

```
rpc/
├── src/                   # 源代码
│   ├── include/           # 头文件
│   │   ├── rpcapplication.h    # RPC框架应用类
│   │   ├── rpcchannel.h        # RPC通道
│   │   ├── rpcconfig.h         # 配置管理
│   │   ├── rpccontroller.h     # RPC控制器
│   │   ├── rpcprovider.h       # 服务提供者
│   │   ├── zookeeperutil.h     # ZooKeeper工具类
│   │   └── logger.h            # 日志系统
│   ├── rpcapplication.cc
│   ├── rpcchannel.cc
│   ├── rpcconfig.cc
│   ├── rpccontroller.cc
│   ├── rpcprovider.cc
│   ├── zookeeperutil.cc
│   └── logger.cc
├── example/               # 示例代码
│   ├── callee/            # 服务端示例
│   ├── caller/            # 客户端示例
│   └── user.proto         # Protobuf定义
├── benchmark/             # 性能测试工具
├── lib/                   # 编译生成的库文件
├── bin/                   # 编译生成的可执行文件
├── CMakeLists.txt         # CMake配置
└── autobuild.sh           # 自动编译脚本
```

## 环境依赖

- **C++11** 或更高版本
- **CMake 3.0+**
- **Protobuf 3.0+**
- **ZooKeeper 3.4+**
- **muduo网络库**

### Ubuntu安装依赖

```bash
# 安装Protobuf
sudo apt install libprotobuf-dev protobuf-compiler

# 安装ZooKeeper
sudo apt install libzookeeper-mt-dev zookeeperd

# 安装muduo网络库
sudo apt install libmuduo-dev
```

## 编译与运行

### 1. 编译项目

```bash
./autobuild.sh
```

### 2. 配置文件

在 `bin/test.conf` 中配置：

```ini
rpcserverip=127.0.0.1
rpcserverport=8000
zookeeperip=127.0.0.1
zookeeperport=2181
```

### 3. 启动服务提供者

```bash
cd bin
./provider -i test.conf
```

### 4. 运行服务消费者

```bash
cd bin
./consumer -i test.conf
```

## 使用示例

### 1. 定义Protobuf服务

```protobuf
syntax = "proto3";

package fixbug;

service UserServiceRpc {
    rpc Login(LoginRequest) returns (LoginResponse);
}

message LoginRequest {
    string name = 1;
    string pwd = 2;
}

message LoginResponse {
    int32 errcode = 1;
    string errmsg = 2;
    bool success = 3;
}
```

### 2. 实现服务端

```cpp
class UserService : public fixbug::UserServiceRpc {
public:
    void Login(google::protobuf::RpcController* controller,
               const fixbug::LoginRequest* request,
               fixbug::LoginResponse* response,
               google::protobuf::Closure* done) override {
        // 实现业务逻辑
        std::string name = request->name();
        std::string pwd = request->pwd();
        
        // 返回结果
        response->set_success(true);
        done->Run();
    }
};

int main(int argc, char** argv) {
    RpcApplication::Init(argc, argv);
    RpcProvider provider;
    provider.NotifyService(new UserService());
    provider.Run();
    return 0;
}
```

### 3. 实现客户端

```cpp
int main(int argc, char** argv) {
    RpcApplication::Init(argc, argv);
    
    fixbug::UserServiceRpc_Stub stub(new RpcChannel());
    fixbug::LoginRequest request;
    request.set_name("zhangsan");
    request.set_pwd("123456");
    
    fixbug::LoginResponse response;
    RpcController controller;
    stub.Login(&controller, &request, &response, nullptr);
    
    if (controller.Failed()) {
        std::cout << controller.ErrorText() << std::endl;
    } else {
        std::cout << "Login success: " << response.success() << std::endl;
    }
    return 0;
}
```

## 性能测试

使用benchmark工具进行压力测试：

```bash
cd bin
./benchmark -i test.conf [线程数] [每线程请求数]
```

示例输出：

```
========== Benchmark Results ==========
Total Requests:    250
Success Requests:  250
Failed Requests:   0
Success Rate:      100.00%
Duration:          0.39s
QPS:               638.89

Latency (us):
  Min:  2107
  Avg:  6449
  P50:  5640
  P90:  10702
  P95:  12734
  P99:  18993
  Max:  26718
========================================
```

## 核心功能

### 1. 服务注册与发现

- Provider启动时自动向ZooKeeper注册服务地址
- Consumer从ZooKeeper获取可用的服务提供者列表
- 支持多节点部署，实现服务冗余

### 2. 服务地址缓存

- 本地缓存服务地址，减少ZooKeeper查询
- 连接失败时自动清理失效的缓存地址
- 提升RPC调用性能

### 3. ZooKeeper Watch机制

- 监听服务地址变化事件
- 自动感知服务上下线
- 实时更新本地缓存

### 4. 高可用设计

- 支持多个Provider实例部署
- 自动故障转移
- 服务健康检查

## 技术要点

### 1. 序列化

使用Protobuf进行高效的数据序列化，相比JSON/XML具有更小的数据体积和更快的序列化速度。

### 2. 网络通信

基于muduo网络库实现，支持高并发TCP连接，采用Reactor模式处理网络事件。

### 3. 服务发现

使用ZooKeeper作为注册中心，实现服务的动态注册与发现，支持服务健康检查。

### 4. 线程安全

服务地址缓存使用mutex保护，确保多线程环境下的安全性。

## 扩展性

框架设计遵循开闭原则，易于扩展：

- **负载均衡**: 可扩展实现轮询、随机、权重等负载均衡策略
- **序列化**: 可扩展支持JSON、MessagePack等其他序列化方式
- **注册中心**: 可扩展支持Consul、etcd等其他注册中心
- **网络库**: 可扩展支持其他网络库或自定义实现

## License

MIT License
