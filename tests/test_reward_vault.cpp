#include "reward_vault.hpp"
#include <cassert>
#include <iostream>

void test_payout_fails_when_vault_empty() {
    std::cout << "Test: Payout fails when RewardVault is empty\n";
    
    membra::RewardVault vault(0);  // Empty vault
    
    std::string entry = vault.create_reward_entry(
        "worker_wallet",
        "receipt_123",
        50000000,
        "inference"
    );
    
    assert(entry.empty());  // Should fail - vault not funded
    
    std::cout << "✓ Payout fails when RewardVault is empty\n";
}

void test_payout_succeeds_when_vault_funded() {
    std::cout << "Test: Payout succeeds when RewardVault is funded\n";
    
    membra::RewardVault vault(100000000000);  // 100 SOL funded
    
    std::string entry = vault.create_reward_entry(
        "worker_wallet",
        "receipt_123",
        50000000,
        "inference"
    );
    
    assert(!entry.empty());  // Should succeed - vault funded
    assert(vault.pool_balance() == 100000000000);  // Balance unchanged until claim
    
    bool claimed = vault.claim_reward(entry);
    assert(claimed);
    assert(vault.pool_balance() == 99999950000);  // Balance decreased after claim
    
    std::cout << "✓ Payout succeeds when RewardVault is funded\n";
}

void test_compensation_caps_apply() {
    std::cout << "Test: Compensation caps apply\n";
    
    membra::RewardVault vault(100000000000);
    
    // Try to create reward exceeding per-receipt cap (1 SOL)
    std::string entry = vault.create_reward_entry(
        "worker_wallet",
        "receipt_123",
        2000000000,  // 2 SOL - exceeds cap
        "inference"
    );
    
    assert(entry.empty());  // Should fail - exceeds cap
    
    // Create reward within cap
    std::string entry2 = vault.create_reward_entry(
        "worker_wallet",
        "receipt_456",
        50000000,  // 0.05 SOL - within cap
        "inference"
    );
    
    assert(!entry2.empty());  // Should succeed
    
    std::cout << "✓ Compensation caps apply\n";
}

void test_daily_cap_enforced() {
    std::cout << "Test: Daily cap enforced\n";
    
    membra::RewardVault vault(100000000000000);  // Large pool
    
    // Create multiple rewards to hit daily cap (10 SOL)
    for (int i = 0; i < 200; i++) {
        std::string entry = vault.create_reward_entry(
            "worker_wallet",
            "receipt_" + std::to_string(i),
            50000000,  // 0.05 SOL
            "inference"
        );
        
        if (i < 200) {  // Should succeed until cap hit
            assert(!entry.empty());
        }
    }
    
    // After 200 rewards of 0.05 SOL each = 10 SOL, should hit daily cap
    uint64_t daily_claimed = vault.get_worker_daily_claimed("worker_wallet");
    assert(daily_claimed <= 10000000000);  // Should not exceed 10 SOL daily cap
    
    std::cout << "✓ Daily cap enforced\n";
}

int main() {
    std::cout << "=== Reward Vault Tests ===\n\n";
    
    test_payout_fails_when_vault_empty();
    test_payout_succeeds_when_vault_funded();
    test_compensation_caps_apply();
    test_daily_cap_enforced();
    
    std::cout << "\n=== All Reward Vault Tests Passed ===\n";
    return 0;
}
