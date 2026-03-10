#include<iostream>
#include<thread>
#include<vector>
#include<atomic>
#include<chrono>
#include<mutex>
#include<queue>
#include<numeric>
#include<algorithm>
#include<iomanip>
#include<string>

#include"rpcapplication.h"
#include"user.pb.h"
#include"rpcchannel.h"

struct BenchmarkStats {
    std::atomic<uint64_t> total_requests{0};
    std::atomic<uint64_t> success_requests{0};
    std::atomic<uint64_t> failed_requests{0};
    std::vector<uint64_t> latencies;
    std::mutex latency_mutex;
    
    void addLatency(uint64_t latency_us) {
        std::lock_guard<std::mutex> lock(latency_mutex);
        latencies.push_back(latency_us);
    }
    
    void printStats(double duration_seconds) {
        uint64_t total = total_requests.load();
        uint64_t success = success_requests.load();
        uint64_t failed = failed_requests.load();
        
        std::lock_guard<std::mutex> lock(latency_mutex);
        
        if (latencies.empty()) {
            std::cout << "No requests completed\n";
            return;
        }
        
        std::sort(latencies.begin(), latencies.end());
        uint64_t min_lat = latencies.front();
        uint64_t max_lat = latencies.back();
        uint64_t sum = std::accumulate(latencies.begin(), latencies.end(), 0ULL);
        double avg_lat = static_cast<double>(sum) / latencies.size();
        uint64_t p50 = latencies[latencies.size() * 50 / 100];
        uint64_t p90 = latencies[latencies.size() * 90 / 100];
        uint64_t p95 = latencies[latencies.size() * 95 / 100];
        uint64_t p99 = latencies[latencies.size() * 99 / 100];
        
        double qps = static_cast<double>(success) / duration_seconds;
        double success_rate = static_cast<double>(success) / total * 100.0;
        
        std::cout << "\n========== Benchmark Results ==========\n";
        std::cout << "Total Requests:    " << total << "\n";
        std::cout << "Success Requests:  " << success << "\n";
        std::cout << "Failed Requests:   " << failed << "\n";
        std::cout << "Success Rate:      " << std::fixed << std::setprecision(2) << success_rate << "%\n";
        std::cout << "Duration:          " << std::fixed << std::setprecision(2) << duration_seconds << "s\n";
        std::cout << "QPS:               " << std::fixed << std::setprecision(2) << qps << "\n";
        std::cout << "\nLatency (us):\n";
        std::cout << "  Min:  " << min_lat << "\n";
        std::cout << "  Avg:  " << static_cast<uint64_t>(avg_lat) << "\n";
        std::cout << "  P50:  " << p50 << "\n";
        std::cout << "  P90:  " << p90 << "\n";
        std::cout << "  P95:  " << p95 << "\n";
        std::cout << "  P99:  " << p99 << "\n";
        std::cout << "  Max:  " << max_lat << "\n";
        std::cout << "========================================\n";
    }
};

int main(int argc, char **argv) {
    // 保存原始的命令行参数，跳过-i和配置文件路径
    int num_threads = 2;
    int requests_per_thread = 10;
    
    // 先解析线程数和请求数参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0) {
            // 跳过配置文件路径
            i++;
            continue;
        }
        if (num_threads == 2) {
            num_threads = std::atoi(argv[i]);
            if (num_threads <= 0) num_threads = 2;
        } else if (requests_per_thread == 10) {
            requests_per_thread = std::atoi(argv[i]);
            if (requests_per_thread <= 0) requests_per_thread = 10;
        }
    }
    
    RpcApplication::Init(argc, argv);
    
    std::cout << "Starting benchmark with " << num_threads << " threads, " 
              << requests_per_thread << " requests per thread (total: " 
              << num_threads * requests_per_thread << " requests)\n";
    
    BenchmarkStats stats;
    std::vector<std::thread> workers;
    
    try {
        auto global_start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < num_threads; i++) {
            workers.emplace_back([i, requests_per_thread, &stats]() {
                try {
                    // 每个线程只创建一个stub
                    fixbug::UserServiceRpc_Stub stub(new RpcChannel());
                    
                    for (int j = 0; j < requests_per_thread; j++) {
                        auto start = std::chrono::high_resolution_clock::now();
                        
                        fixbug::LoginRequest request;
                        request.set_name("user_" + std::to_string(i) + "_" + std::to_string(j));
                        request.set_pwd("password_" + std::to_string(j));
                        
                        fixbug::LoginResponse response;
                        RpcController controller;
                        
                        // 调用Login方法
                        stub.Login(&controller, &request, &response, nullptr);
                        
                        auto end = std::chrono::high_resolution_clock::now();
                        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
                        
                        stats.total_requests++;
                        
                        if (controller.Failed()) {
                            stats.failed_requests++;
                        } else {
                            if (response.result().errcode() == 0) {
                                stats.success_requests++;
                                stats.addLatency(duration);
                            } else {
                                stats.failed_requests++;
                            }
                        }
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Thread " << i << " exception: " << e.what() << std::endl;
                    stats.failed_requests += requests_per_thread;
                    stats.total_requests += requests_per_thread;
                }
            });
        }
        
        for (auto& t : workers) {
            t.join();
        }
        
        auto global_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(global_end - global_start).count();
        double duration_seconds = static_cast<double>(duration) / 1000000.0;
        
        stats.printStats(duration_seconds);
    } catch (const std::exception& e) {
        std::cerr << "Main exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
