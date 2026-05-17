#pragma once

#include "crypto.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <chrono>

namespace membra {

enum class IntentStatus : uint8_t {
    PENDING = 0,
    CLAIMED = 1,
    EXPIRED = 2,
    CANCELLED = 3
};

struct PaymentIntent {
    std::string intent_id;
    std::string sender_address;
    std::string receiver_address;
    std::string token_symbol;
    uint64_t amount;
    double created_at;
    double expires_at;
    uint64_t nonce;
    std::string sender_signature;
    IntentStatus status{IntentStatus::PENDING};
    double claimed_at{0};
    std::string settlement_tx;

    bool is_expired() const {
        auto now = std::chrono::duration<double>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        return now >= expires_at;
    }

    bool is_claimable() const {
        return status == IntentStatus::PENDING && !is_expired();
    }

    std::string intent_hash() const {
        std::string payload = intent_id + sender_address + receiver_address +
                             token_symbol + std::to_string(amount) +
                             std::to_string(created_at) + std::to_string(expires_at) +
                             std::to_string(nonce);
        return hash_to_hex(sha256(payload));
    }
};

class IntentNetwork {
private:
    std::unordered_map<std::string, PaymentIntent> intents_;
    std::unordered_map<std::string, uint64_t> user_nonces_;
    std::mutex mutex_;
    std::atomic<uint64_t> intent_counter_;

    // 7-day claim window
    static constexpr double CLAIM_WINDOW_SECONDS = 604800.0;

public:
    IntentNetwork() : intent_counter_(1) {}

    PaymentIntent create_intent(
        const std::string& sender,
        const std::string& receiver,
        const std::string& token,
        uint64_t amount,
        const std::string& signature
    ) {
        std::lock_guard<std::mutex> lock(mutex_);

        uint64_t nonce = user_nonces_[sender]++;
        auto now = std::chrono::duration<double>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        PaymentIntent intent;
        intent.intent_id = generate_intent_id();
        intent.sender_address = sender;
        intent.receiver_address = receiver;
        intent.token_symbol = token;
        intent.amount = amount;
        intent.created_at = now;
        intent.expires_at = now + CLAIM_WINDOW_SECONDS;
        intent.nonce = nonce;
        intent.sender_signature = signature;
        intent.status = IntentStatus::PENDING;

        intents_[intent.intent_id] = intent;
        return intent;
    }

    PaymentIntent* claim_intent(const std::string& intent_id, const std::string& claimer) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = intents_.find(intent_id);
        if (it == intents_.end()) return nullptr;

        PaymentIntent& intent = it->second;
        if (!intent.is_claimable()) return nullptr;

        // Allow anyone to claim (relayer-friendly)
        intent.status = IntentStatus::CLAIMED;
        intent.claimed_at = std::chrono::duration<double>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        return &intent;
    }

    bool confirm_settlement(
        const std::string& intent_id,
        const std::string& tx_signature
    ) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = intents_.find(intent_id);
        if (it == intents_.end()) return false;

        PaymentIntent& intent = it->second;
        if (intent.status != IntentStatus::CLAIMED) return false;

        intent.settlement_tx = tx_signature;
        return true;
    }

    PaymentIntent* get_intent(const std::string& intent_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = intents_.find(intent_id);
        return it == intents_.end() ? nullptr : &it->second;
    }

    void expire_intents() {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::duration<double>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        for (auto& pair : intents_) {
            PaymentIntent& intent = pair.second;
            if (intent.status == IntentStatus::PENDING && now >= intent.expires_at) {
                intent.status = IntentStatus::EXPIRED;
            }
        }
    }

    size_t pending_count() {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t count = 0;
        for (const auto& pair : intents_) {
            if (pair.second.status == IntentStatus::PENDING) count++;
        }
        return count;
    }

private:
    std::string generate_intent_id() {
        uint64_t id = intent_counter_.fetch_add(1);
        return "int_" + std::to_string(id);
    }
};

} // namespace membra
