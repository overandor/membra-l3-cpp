#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <chrono>

namespace membra {

enum class FutureType : uint8_t {
    FORWARD_CONTRACT = 0,  // Lock in compute for future use
    SPECULATIVE_LONG = 1,  // Bet compute price will rise
    SPECULATIVE_SHORT = 2  // Bet compute price will fall
};

enum class FutureStatus : uint8_t {
    OPEN = 0,
    SETTLED = 1,
    LIQUIDATED = 2,
    EXPIRED = 3
};

struct ComputePricePoint {
    double cpu_price_sol;     // price per core in SOL
    double ram_price_sol;     // price per GB in SOL
    double compute_index;     // combined index (weighted average)
    double timestamp;
};

struct ComputeFuture {
    std::string future_id;
    std::string trader_address;
    FutureType type;
    FutureStatus status;
    
    uint64_t cpu_cores;       // amount of compute
    uint64_t ram_gb;          // amount of RAM
    
    double entry_price;       // price at entry (compute_index)
    double target_price;      // target price for settlement
    double strike_price;      // strike price for forwards
    double leverage;          // leverage multiplier
    
    double collateral_sol;    // collateral posted
    double notional_value;    // total value
    
    double opened_at;
    double expires_at;
    double settled_at;
    
    double pnl;               // profit/loss at settlement
};

class ComputePriceOracle {
private:
    std::vector<ComputePricePoint> price_history_;
    size_t price_index_;
    std::mutex mutex_;
    std::atomic<double> current_index_;
    std::atomic<double> current_cpu_price_;
    std::atomic<double> current_ram_price_;
    
    static constexpr size_t PRICE_HISTORY_SIZE = 100;
    static constexpr double CPU_WEIGHT = 0.6;  // CPU is 60% of index
    static constexpr double RAM_WEIGHT = 0.4;  // RAM is 40% of index

public:
    ComputePriceOracle()
        : price_history_(PRICE_HISTORY_SIZE), price_index_(0),
          current_index_(1.0), current_cpu_price_(0.1), current_ram_price_(0.05) {}
    
    void feed_price(double cpu_price_sol, double ram_price_sol) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = current_timestamp();
        double compute_index = cpu_price_sol * CPU_WEIGHT + ram_price_sol * RAM_WEIGHT;
        
        price_history_[price_index_] = {cpu_price_sol, ram_price_sol, compute_index, now};
        price_index_ = (price_index_ + 1) % PRICE_HISTORY_SIZE;
        
        current_index_.store(compute_index);
        current_cpu_price_.store(cpu_price_sol);
        current_ram_price_.store(ram_price_sol);
    }
    
    double current_index() const { return current_index_.load(); }
    double current_cpu_price() const { return current_cpu_price_.load(); }
    double current_ram_price() const { return current_ram_price_.load(); }
    
    double compute_index_for_cpu_ram(uint64_t cpu_cores, uint64_t ram_gb) {
        double cpu_price = current_cpu_price_.load();
        double ram_price = current_ram_price_.load();
        return (cpu_cores * cpu_price + ram_gb * ram_price);
    }
    
    double price_change_pct(double seconds_ago) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = current_timestamp();
        double target_time = now - seconds_ago;
        
        double oldest_price = 0;
        for (const auto& pp : price_history_) {
            if (pp.timestamp > 0 && pp.timestamp < target_time) {
                oldest_price = pp.compute_index;
            }
        }
        
        if (oldest_price == 0) return 0.0;
        return (current_index_.load() - oldest_price) / oldest_price * 100.0;
    }
    
private:
    static double current_timestamp() {
        return std::chrono::duration<double>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
};

class ComputeFuturesMarket {
private:
    std::unordered_map<std::string, ComputeFuture> futures_;
    std::mutex mutex_;
    std::atomic<uint64_t> future_counter_;
    ComputePriceOracle* price_oracle_;
    
    static constexpr double DEFAULT_LEVERAGE = 2.0;
    static constexpr double MIN_COLLATERAL_RATIO = 0.5;  // 50% collateral

public:
    ComputeFuturesMarket(ComputePriceOracle* oracle)
        : future_counter_(0), price_oracle_(oracle) {}
    
    // Open a forward contract (lock compute for future use)
    std::string open_forward(
        const std::string& trader_address,
        uint64_t cpu_cores,
        uint64_t ram_gb,
        double duration_seconds,
        double collateral_sol
    ) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = current_timestamp();
        double notional = price_oracle_->compute_index_for_cpu_ram(cpu_cores, ram_gb);
        
        ComputeFuture future;
        future.future_id = "fwd_" + std::to_string(future_counter_++);
        future.trader_address = trader_address;
        future.type = FutureType::FORWARD_CONTRACT;
        future.status = FutureStatus::OPEN;
        future.cpu_cores = cpu_cores;
        future.ram_gb = ram_gb;
        future.entry_price = price_oracle_->current_index();
        future.target_price = future.entry_price;  // forwards are at current price
        future.leverage = 1.0;  // no leverage for forwards
        future.collateral_sol = collateral_sol;
        future.notional_value = notional;
        future.opened_at = now;
        future.expires_at = now + duration_seconds;
        future.settled_at = 0;
        future.pnl = 0;
        
        futures_[future.future_id] = std::move(future);
        return future.future_id;
    }
    
    // Open a speculative long position (bet price rises)
    std::string open_long(
        const std::string& trader_address,
        uint64_t cpu_cores,
        uint64_t ram_gb,
        double leverage,
        double collateral_sol,
        double duration_seconds
    ) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = current_timestamp();
        double notional = price_oracle_->compute_index_for_cpu_ram(cpu_cores, ram_gb) * leverage;
        
        ComputeFuture future;
        future.future_id = "long_" + std::to_string(future_counter_++);
        future.trader_address = trader_address;
        future.type = FutureType::SPECULATIVE_LONG;
        future.status = FutureStatus::OPEN;
        future.cpu_cores = cpu_cores;
        future.ram_gb = ram_gb;
        future.entry_price = price_oracle_->current_index();
        future.target_price = future.entry_price * 1.5;  // target 50% gain
        future.leverage = leverage;
        future.collateral_sol = collateral_sol;
        future.notional_value = notional;
        future.opened_at = now;
        future.expires_at = now + duration_seconds;
        future.settled_at = 0;
        future.pnl = 0;
        
        futures_[future.future_id] = std::move(future);
        return future.future_id;
    }
    
    // Open a speculative short position (bet price falls)
    std::string open_short(
        const std::string& trader_address,
        uint64_t cpu_cores,
        uint64_t ram_gb,
        double leverage,
        double collateral_sol,
        double duration_seconds
    ) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = current_timestamp();
        double notional = price_oracle_->compute_index_for_cpu_ram(cpu_cores, ram_gb) * leverage;
        
        ComputeFuture future;
        future.future_id = "short_" + std::to_string(future_counter_++);
        future.trader_address = trader_address;
        future.type = FutureType::SPECULATIVE_SHORT;
        future.status = FutureStatus::OPEN;
        future.cpu_cores = cpu_cores;
        future.ram_gb = ram_gb;
        future.entry_price = price_oracle_->current_index();
        future.target_price = future.entry_price * 0.5;  // target 50% drop
        future.leverage = leverage;
        future.collateral_sol = collateral_sol;
        future.notional_value = notional;
        future.opened_at = now;
        future.expires_at = now + duration_seconds;
        future.settled_at = 0;
        future.pnl = 0;
        
        futures_[future.future_id] = std::move(future);
        return future.future_id;
    }
    
    // Settle a future
    bool settle_future(const std::string& future_id, double& pnl) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = futures_.find(future_id);
        if (it == futures_.end()) return false;
        if (it->second.status != FutureStatus::OPEN) return false;
        
        ComputeFuture& future = it->second;
        double current_price = price_oracle_->current_index();
        
        // Calculate PnL based on type
        if (future.type == FutureType::SPECULATIVE_LONG) {
            future.pnl = (current_price - future.entry_price) * future.leverage * 
                         (future.cpu_cores * 0.1 + future.ram_gb * 0.05);  // simplified
        } else if (future.type == FutureType::SPECULATIVE_SHORT) {
            future.pnl = (future.entry_price - current_price) * future.leverage *
                         (future.cpu_cores * 0.1 + future.ram_gb * 0.05);
        } else {
            // Forward contract: price difference
            future.pnl = (future.target_price - current_price) *
                         (future.cpu_cores * 0.1 + future.ram_gb * 0.05);
        }
        
        future.settled_at = current_timestamp();
        future.status = FutureStatus::SETTLED;
        pnl = future.pnl;
        
        return true;
    }
    
    // Check for liquidations
    std::vector<std::string> check_liquidations() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<std::string> liquidated;
        double current_price = price_oracle_->current_index();
        
        for (auto& pair : futures_) {
            ComputeFuture& future = pair.second;
            if (future.status != FutureStatus::OPEN) continue;
            
            double unrealized_pnl = 0;
            if (future.type == FutureType::SPECULATIVE_LONG) {
                unrealized_pnl = (current_price - future.entry_price) * future.leverage;
            } else if (future.type == FutureType::SPECULATIVE_SHORT) {
                unrealized_pnl = (future.entry_price - current_price) * future.leverage;
            }
            
            double collateral_ratio = (future.collateral_sol + unrealized_pnl) / future.notional_value;
            if (collateral_ratio < MIN_COLLATERAL_RATIO) {
                future.status = FutureStatus::LIQUIDATED;
                future.pnl = -future.collateral_sol;  // lose all collateral
                liquidated.push_back(future.future_id);
            }
        }
        
        return liquidated;
    }
    
    ComputeFuture* get_future(const std::string& future_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = futures_.find(future_id);
        return it == futures_.end() ? nullptr : &it->second;
    }
    
    size_t open_futures_count() {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t count = 0;
        for (const auto& pair : futures_) {
            if (pair.second.status == FutureStatus::OPEN) count++;
        }
        return count;
    }

private:
    static double current_timestamp() {
        return std::chrono::duration<double>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
};

} // namespace membra
