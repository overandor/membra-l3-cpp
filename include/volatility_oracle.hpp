#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <chrono>
#include <cmath>

namespace membra {

enum class VolatilitySignal : uint8_t {
    VOLATILITY_DETECTED = 0,
    VOLATILITY_PAUSED = 1,
    GAS_SPIKE = 2,
    GAS_NORMAL = 3,
    COVERAGE_WARNING = 4,
    COVERAGE_HEALTHY = 5,
    ORACLE_STALE = 6,
    ORACLE_FRESH = 7
};

struct PricePoint {
    double price_usd;
    double timestamp;
    double volume_usd;
};

struct GasConditions {
    uint64_t base_fee_lamports;
    uint64_t priority_fee_lamports;
    bool congested;
    double timestamp;
};

struct VolatilityReport {
    VolatilitySignal signal;
    double confidence;
    double membra_twap;
    double price_change_pct;
    double coverage_ratio;
    double oracle_age_seconds;
    double timestamp;
};

class VolatilityOracle {
private:
    // Ring buffer for price history (fixed size, no allocations)
    static constexpr size_t PRICE_HISTORY_SIZE = 100;
    std::vector<PricePoint> price_history_;
    size_t price_index_;
    std::mutex price_mutex_;

    GasConditions latest_gas_;
    std::mutex gas_mutex_;

    std::atomic<double> last_oracle_update_;
    std::atomic<double> last_twap_;
    std::atomic<double> last_price_change_;

    // Configuration
    static constexpr double VOLATILITY_THRESHOLD = 0.02;  // 2%
    static constexpr double ORACLE_STALE_SECONDS = 300.0;  // 5 min
    static constexpr size_t MIN_DATA_POINTS = 12;

public:
    VolatilityOracle()
        : price_history_(PRICE_HISTORY_SIZE), price_index_(0),
          last_oracle_update_(0), last_twap_(0), last_price_change_(0) {
        latest_gas_ = {5000, 0, false, 0};
    }

    void feed_price(double price_usd, double volume_usd = 0) {
        std::lock_guard<std::mutex> lock(price_mutex_);
        auto now = current_timestamp();

        price_history_[price_index_] = {price_usd, now, volume_usd};
        price_index_ = (price_index_ + 1) % PRICE_HISTORY_SIZE;

        last_oracle_update_.store(now);
    }

    void feed_gas(uint64_t base_fee, uint64_t priority_fee, bool congested) {
        std::lock_guard<std::mutex> lock(gas_mutex_);
        latest_gas_ = {base_fee, priority_fee, congested, current_timestamp()};
    }

    VolatilityReport assess(double coverage_ratio) {
        auto now = current_timestamp();
        double twap, price_change;

        {
            std::lock_guard<std::mutex> lock(price_mutex_);
            std::tie(twap, price_change) = compute_twap();
        }

        double oracle_age = now - last_oracle_update_.load();
        bool oracle_stale = oracle_age > ORACLE_STALE_SECONDS;
        bool coverage_low = coverage_ratio < 1.5;

        // Determine signal
        VolatilitySignal signal;
        double confidence;

        if (oracle_stale) {
            signal = VolatilitySignal::ORACLE_STALE;
            confidence = 0.95;
        } else if (coverage_low) {
            signal = VolatilitySignal::COVERAGE_WARNING;
            confidence = 0.9;
        } else if (std::abs(price_change) >= VOLATILITY_THRESHOLD) {
            signal = VolatilitySignal::VOLATILITY_DETECTED;
            confidence = std::min(0.95, 0.5 + std::abs(price_change) * 10.0);
        } else {
            signal = VolatilitySignal::VOLATILITY_PAUSED;
            confidence = 0.7;
        }

        last_twap_.store(twap);
        last_price_change_.store(price_change);

        return {
            signal, confidence, twap, price_change * 100,
            coverage_ratio, oracle_age, now
        };
    }

    bool can_unlock_rebase() const {
        double change = last_price_change_.load();
        return std::abs(change) >= VOLATILITY_THRESHOLD;
    }

    double last_twap() const { return last_twap_.load(); }
    double last_price_change() const { return last_price_change_.load(); }

private:
    std::pair<double, double> compute_twap() {
        // Simple TWAP from recent data
        double total_price = 0;
        double total_volume = 0;
        size_t count = 0;

        for (const auto& pp : price_history_) {
            if (pp.timestamp > 0) {  // valid entry
                if (pp.volume_usd > 0) {
                    total_price += pp.price_usd * pp.volume_usd;
                    total_volume += pp.volume_usd;
                } else {
                    total_price += pp.price_usd;
                    total_volume += 1;
                }
                count++;
            }
        }

        if (count < MIN_DATA_POINTS || total_volume == 0) {
            return {0, 0};
        }

        double twap = total_price / total_volume;

        // Price change from first valid to last valid
        double first_price = 0, last_price = 0;
        bool found_first = false, found_last = false;

        for (size_t i = 0; i < PRICE_HISTORY_SIZE; i++) {
            size_t idx = (price_index_ + i) % PRICE_HISTORY_SIZE;
            if (price_history_[idx].timestamp > 0) {
                if (!found_first) {
                    first_price = price_history_[idx].price_usd;
                    found_first = true;
                }
                last_price = price_history_[idx].price_usd;
                found_last = true;
            }
        }

        double change = (first_price > 0) ? (last_price - first_price) / first_price : 0;
        return {twap, change};
    }

    static double current_timestamp() {
        return std::chrono::duration<double>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
};

} // namespace membra
