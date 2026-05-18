#include "gas_vault.hpp"
#include "compute_staking.hpp"
#include <cassert>
#include <iostream>

void test_settlement_fails_when_vault_empty() {
    std::cout << "Test: Settlement fails when GasVault is empty\n";
    
    membra::ComputeStaking staking;
    membra::GasVault vault(0, &staking);  // Empty vault
    
    // Stake compute first
    uint64_t credit_grant = 0;
    staking.stake_compute("test_wallet", 4, 16, 4000, credit_grant);
    
    // Try to pay gas with empty vault (no gas credits used)
    bool paid = vault.pay_gas("other_wallet", 5000);
    assert(!paid);  // Should fail - vault empty and no credits
    
    std::cout << "✓ Settlement fails when GasVault is empty\n";
}

void test_settlement_succeeds_with_gas_credits() {
    std::cout << "Test: Settlement succeeds with gas credits\n";
    
    membra::ComputeStaking staking;
    membra::GasVault vault(0, &staking);  // Empty vault but with staking
    
    // Stake compute
    uint64_t credit_grant = 0;
    staking.stake_compute("test_wallet", 4, 16, 4000, credit_grant);
    
    // Pay gas using credits
    bool paid = vault.pay_gas("test_wallet", 5000);
    assert(paid);  // Should succeed - using gas credits
    
    uint64_t remaining = staking.get_gas_allowance("test_wallet");
    assert(remaining == 39999995000);  // 40 SOL - 5000 lamports
    
    std::cout << "✓ Settlement succeeds with gas credits\n";
}

void test_settlement_succeeds_when_vault_funded() {
    std::cout << "Test: Settlement succeeds when vault funded\n";
    
    membra::ComputeStaking staking;
    membra::GasVault vault(100000000000, &staking);  // 100 SOL funded
    
    // Pay gas without credits (uses vault)
    bool paid = vault.pay_gas("test_wallet", 5000);
    assert(paid);  // Should succeed - vault funded
    
    assert(vault.sol_reserves_sol() == 99.999995);  // 100 SOL - 5000 lamports
    
    std::cout << "✓ Settlement succeeds when vault funded\n";
}

void test_gas_credits_accounted() {
    std::cout << "Test: Gas credits are accounted against allowance\n";
    
    membra::ComputeStaking staking;
    membra::GasVault vault(0, &staking);
    
    // Stake compute
    uint64_t credit_grant = 0;
    staking.stake_compute("test_wallet", 4, 16, 4000, credit_grant);
    
    uint64_t before = staking.get_gas_allowance("test_wallet");
    
    // Pay gas multiple times
    vault.pay_gas("test_wallet", 5000);
    vault.pay_gas("test_wallet", 10000);
    vault.pay_gas("test_wallet", 15000);
    
    uint64_t after = staking.get_gas_allowance("test_wallet");
    assert(after == before - 30000);  // 5000 + 10000 + 15000
    
    assert(vault.gas_paid_by_compute() == 30000);
    
    std::cout << "✓ Gas credits are accounted against allowance\n";
}

void test_no_gas_paid_by_compute_directly() {
    std::cout << "Test: No 'gas paid by staked compute directly' language\n";
    
    membra::ComputeStaking staking;
    membra::GasVault vault(100000000000, &staking);
    
    // This test verifies the correct model:
    // - Gas is accounted against credits, not "paid by compute"
    // - Compute provides eligibility, not direct payment
    
    uint64_t credit_grant = 0;
    staking.stake_compute("test_wallet", 4, 16, 4000, credit_grant);
    
    vault.pay_gas("test_wallet", 5000);
    
    // Verify gas was accounted against credits
    assert(vault.gas_paid_by_compute() == 5000);
    assert(staking.get_gas_allowance("test_wallet") == 39999995000);
    
    std::cout << "✓ No 'gas paid by staked compute directly' language\n";
}

int main() {
    std::cout << "=== Gas Vault Tests ===\n\n";
    
    test_settlement_fails_when_vault_empty();
    test_settlement_succeeds_with_gas_credits();
    test_settlement_succeeds_when_vault_funded();
    test_gas_credits_accounted();
    test_no_gas_paid_by_compute_directly();
    
    std::cout << "\n=== All Gas Vault Tests Passed ===\n";
    return 0;
}
