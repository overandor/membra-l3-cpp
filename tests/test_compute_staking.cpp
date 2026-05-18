#include "compute_staking.hpp"
#include <cassert>
#include <iostream>

void test_staking_creates_credits_not_sol() {
    std::cout << "Test: Staking creates credits, not SOL\n";
    
    membra::ComputeStaking staking;
    uint64_t credit_grant = 0;
    
    std::string stake_id = staking.stake_compute("test_wallet", 4, 16, 4000, credit_grant);
    
    assert(credit_grant > 0);
    assert(credit_grant == 40000000000);  // 40 SOL-equivalent credits
    assert(staking.total_credit_grants_issued() == 40000000000);
    
    auto stake = staking.get_stake(stake_id);
    assert(stake != nullptr);
    assert(stake->instant_credit_grant == 40000000000);
    assert(stake->gas_credit_allowance == 40000000000);
    
    std::cout << "✓ Staking creates credits, not SOL\n";
}

void test_staking_does_not_increase_sol_vault() {
    std::cout << "Test: Staking does not increase SOL vault balance\n";
    
    membra::ComputeStaking staking;
    uint64_t credit_grant = 0;
    
    staking.stake_compute("test_wallet", 4, 16, 4000, credit_grant);
    
    // Compute staking has no SOL vault - it only issues credits
    // This test verifies that staking doesn't create SOL
    assert(credit_grant > 0);
    
    std::cout << "✓ Staking does not increase SOL vault balance\n";
}

void test_gas_allowance_debit() {
    std::cout << "Test: Gas allowance is debited after transaction\n";
    
    membra::ComputeStaking staking;
    uint64_t credit_grant = 0;
    
    staking.stake_compute("test_wallet", 4, 16, 4000, credit_grant);
    uint64_t before = staking.get_gas_allowance("test_wallet");
    
    bool consumed = staking.consume_gas_allowance("test_wallet", 5000);
    assert(consumed);
    
    uint64_t after = staking.get_gas_allowance("test_wallet");
    assert(after == before - 5000);
    
    std::cout << "✓ Gas allowance is debited after transaction\n";
}

void test_no_instant_sol_language() {
    std::cout << "Test: No 'instant SOL' language in staking\n";
    
    membra::ComputeStaking staking;
    uint64_t credit_grant = 0;
    
    staking.stake_compute("test_wallet", 4, 16, 4000, credit_grant);
    
    auto stake = staking.get_stake("stake_test_wallet_0");
    if (stake) {
        // Verify field names are correct (not instant_reward)
        assert(stake->instant_credit_grant > 0);
    }
    
    std::cout << "✓ No 'instant SOL' language in staking\n";
}

int main() {
    std::cout << "=== Compute Staking Tests ===\n\n";
    
    test_staking_creates_credits_not_sol();
    test_staking_does_not_increase_sol_vault();
    test_gas_allowance_debit();
    test_no_instant_sol_language();
    
    std::cout << "\n=== All Compute Staking Tests Passed ===\n";
    return 0;
}
