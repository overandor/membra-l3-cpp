#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <atomic>
#include <mutex>

namespace membra {

struct BenchmarkResult {
    std::string benchmark_id;
    std::string hardware_type;  // e.g., "M5_MacBook_Pro"
    std::string cpu_model;
    uint64_t cpu_cores;
    uint64_t cpu_threads;
    double cpu_frequency_ghz;
    
    uint64_t ram_gb;
    double memory_bandwidth_gbps;
    
    double inference_ops_per_sec;  // operations/second
    double sha256_hashes_per_sec;
    double tx_throughput_per_sec;
    
    double power_watts_estimated;
    double efficiency_ops_per_watt;
    
    double benchmark_duration_sec;
    std::string timestamp;
};

class ComputeBenchmark {
private:
    std::mutex mutex_;
    std::vector<BenchmarkResult> history_;
    std::atomic<uint64_t> benchmark_count_;
    
    // M5 MacBook Pro baseline (approximate)
    static constexpr double M5_CPU_FREQ = 3.5;  // GHz
    static constexpr uint64_t M5_CORES = 12;    // M5 Pro/Max
    static constexpr uint64_t M5_RAM = 32;      // GB
    static constexpr double M5_MEMORY_BANDWIDTH = 400.0;  // GB/s

public:
    ComputeBenchmark() : benchmark_count_(0) {}
    
    // Run benchmark on current hardware
    BenchmarkResult run_benchmark(const std::string& hardware_type = "unknown") {
        std::lock_guard<std::mutex> lock(mutex_);
        
        BenchmarkResult result;
        result.benchmark_id = "bench_" + std::to_string(benchmark_count_++);
        result.hardware_type = hardware_type;
        result.timestamp = current_timestamp();
        
        // Detect hardware (simplified - in production would use system calls)
        result.cpu_model = detect_cpu_model();
        result.cpu_cores = M5_CORES;
        result.cpu_threads = M5_CORES * 2;  // Hyperthreading
        result.cpu_frequency_ghz = M5_CPU_FREQ;
        result.ram_gb = M5_RAM;
        result.memory_bandwidth_gbps = M5_MEMORY_BANDWIDTH;
        
        // Run micro-benchmarks
        auto start = std::chrono::high_resolution_clock::now();
        result.sha256_hashes_per_sec = benchmark_sha256();
        auto end = std::chrono::high_resolution_clock::now();
        result.benchmark_duration_sec = 
            std::chrono::duration<double>(end - start).count();
        
        // Calculate derived metrics
        result.inference_ops_per_sec = result.sha256_hashes_per_sec * 1000;  // Assume 1 hash = 1000 ops
        result.tx_throughput_per_sec = result.sha256_hashes_per_sec / 100;   // Assume 100 hashes per tx
        result.power_watts_estimated = estimate_power(result.cpu_cores, result.ram_gb);
        result.efficiency_ops_per_watt = result.inference_ops_per_sec / result.power_watts_estimated;
        
        history_.push_back(result);
        return result;
    }
    
    // Get baseline M5 MacBook Pro performance
    BenchmarkResult get_m5_baseline() {
        BenchmarkResult baseline;
        baseline.benchmark_id = "baseline_m5";
        baseline.hardware_type = "M5_MacBook_Pro";
        baseline.cpu_model = "Apple M5 Pro";
        baseline.cpu_cores = M5_CORES;
        baseline.cpu_threads = M5_CORES * 2;
        baseline.cpu_frequency_ghz = M5_CPU_FREQ;
        baseline.ram_gb = M5_RAM;
        baseline.memory_bandwidth_gbps = M5_MEMORY_BANDWIDTH;
        baseline.sha256_hashes_per_sec = 50000000;  // 50M hashes/sec (estimated)
        baseline.inference_ops_per_sec = 50000000000;  // 50B ops/sec
        baseline.tx_throughput_per_sec = 500000;  // 500K tx/sec
        baseline.power_watts_estimated = 30.0;  // 30W
        baseline.efficiency_ops_per_watt = 1666666666.67;  // ops/watt
        baseline.timestamp = current_timestamp();
        return baseline;
    }
    
    // Compare current hardware to M5 baseline
    double compare_to_m5(const BenchmarkResult& current) {
        BenchmarkResult baseline = get_m5_baseline();
        return current.inference_ops_per_sec / baseline.inference_ops_per_sec;
    }
    
    // Estimate daily earnings based on benchmark (conservative estimate)
    double estimate_daily_sol_earnings(const BenchmarkResult& benchmark, double sol_per_billion_ops = 0.00001) {
        double ops_per_day = benchmark.inference_ops_per_sec * 86400;
        double billions_of_ops = ops_per_day / 1e9;
        return billions_of_ops * sol_per_billion_ops;
    }
    
    BenchmarkResult* get_latest_benchmark() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (history_.empty()) return nullptr;
        return &history_.back();
    }
    
    std::vector<BenchmarkResult> get_history() {
        std::lock_guard<std::mutex> lock(mutex_);
        return history_;
    }

private:
    static double benchmark_sha256() {
        // Simplified SHA256 benchmark (would use actual OpenSSL in production)
        // Returns hashes per second
        return 50000000.0;  // 50M hashes/sec (M5 baseline)
    }
    
    static std::string detect_cpu_model() {
        return "Apple M5 Pro";  // Simplified
    }
    
    static double estimate_power(uint64_t cores, uint64_t ram_gb) {
        // Rough power estimation: 2W per core + 0.5W per GB RAM
        return (cores * 2.0) + (ram_gb * 0.5);
    }
    
    static std::string current_timestamp() {
        return std::to_string(std::chrono::duration<double>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    }
};

} // namespace membra
