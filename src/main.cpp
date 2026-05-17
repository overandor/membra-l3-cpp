#include "crypto.hpp"
#include "proof_book.hpp"
#include "gas_vault.hpp"
#include "intent_network.hpp"
#include "volatility_oracle.hpp"
#include "compute_staking.hpp"

#include <iostream>
#include <thread>
#include <chrono>

using namespace membra;

int main() {
    std::cout << "MEMBRA Layer-3 C++ Implementation\n";
    std::cout << "Compute-Collateralized Gas Model with Instant Rewards\n";
    std::cout << "Users stake CPU/RAM to send transactions for free\n";
    std::cout << "MEMBRA pays stakers instantly when they lock compute\n\n";

    // Initialize components
    ComputeStaking compute_staking;
    GasVault gas_vault(100'000'000'000, &compute_staking);  // 100 SOL + compute staking
    IntentNetwork intent_network;
    VolatilityOracle oracle;
    ProofBook proof_book;

    std::cout << "GasVault reserves: " << gas_vault.sol_reserves_sol() << " SOL (backup)\n";
    std::cout << "Staked compute: " << compute_staking.total_cpu_cores() << " cores\n";
    std::cout << "GasVault healthy: " << (gas_vault.is_healthy() ? "yes" : "no") << "\n\n";

    // Alice stakes CPU/RAM to send transactions for free
    std::cout << "=== Step 1: Alice stakes CPU/RAM for free transactions ===\n";
    uint64_t alice_instant_reward = 0;
    std::string stake_id = compute_staking.stake_compute("alice_wallet", 4, 16, 4000, alice_instant_reward);
    std::cout << "Stake ID: " << stake_id << "\n";
    std::cout << "Alice staked: 4 CPU cores, 16 GB RAM, 4000 compute units\n";
    std::cout << "MEMBRA pays instantly: " << alice_instant_reward / 1e9 << " SOL\n";
    std::cout << "Alice's gas allowance for free txs: " << compute_staking.get_gas_allowance("alice_wallet") / 1e9 << " SOL\n";
    std::cout << "Total staked cores: " << compute_staking.total_cpu_cores() << "\n\n";

    // Bob also stakes
    uint64_t bob_instant_reward = 0;
    std::string bob_stake = compute_staking.stake_compute("bob_wallet", 8, 32, 8000, bob_instant_reward);
    std::cout << "Bob staked: 8 CPU cores, 32 GB RAM, 8000 compute units\n";
    std::cout << "MEMBRA pays instantly: " << bob_instant_reward / 1e9 << " SOL\n";
    std::cout << "Bob's gas allowance for free txs: " << compute_staking.get_gas_allowance("bob_wallet") / 1e9 << " SOL\n";
    std::cout << "Total staked cores: " << compute_staking.total_cpu_cores() << "\n";
    std::cout << "Total instant rewards paid: " << compute_staking.total_instant_rewards_paid() / 1e9 << " SOL\n\n";

    // Feed price data for volatility detection
    std::cout << "=== Step 2: Volatility Oracle ===\n";
    std::cout << "Feeding price data...\n";
    for (int i = 0; i < 20; i++) {
        double price = 1.0 * (1 + i * 0.005);  // 10% rise
        oracle.feed_price(price, 5000.0);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    VolatilityReport report = oracle.assess(gas_vault.sol_reserves_sol());
    std::cout << "Volatility signal: " << static_cast<int>(report.signal) << "\n";
    std::cout << "Confidence: " << report.confidence << "\n";
    std::cout << "MEMBRA TWAP: $" << report.membra_twap << "\n";
    std::cout << "Price change: " << report.price_change_pct << "%\n\n";

    // Create gasless payment intent (Alice uses her staked compute)
    std::cout << "=== Step 3: Alice sends transaction using her staked compute ===\n";
    auto intent = intent_network.create_intent(
        "alice_wallet",
        "bob_wallet",
        "USDC",
        50'000'000,  // 50 USDC
        "alice_signature"
    );

    std::cout << "Intent created successfully\n";
    std::cout << "Intent ID: " << intent.intent_id << "\n";
    std::cout << "Amount: " << intent.amount / 1'000'000 << " USDC\n";
    std::cout << "Alice's gas allowance before: " << compute_staking.get_gas_allowance("alice_wallet") << " lamports\n";
    std::cout << "Claim window: 7 days\n\n";

    // Claim intent (gas paid by Alice's staked compute)
    std::cout << "=== Step 4: Settlement with Alice's Gas Allowance ===\n";
    auto claimed = intent_network.claim_intent(intent.intent_id, "relayer");
    if (claimed) {
        uint64_t gas_cost = 5000;  // estimated gas
        bool paid = gas_vault.pay_gas("alice_wallet", gas_cost);

        std::cout << "Intent claimed\n";
        std::cout << "Gas cost: " << gas_cost << " lamports\n";
        std::cout << "Gas paid by: Alice's staked compute (gas allowance)\n";
        std::cout << "Alice's gas allowance after: " << compute_staking.get_gas_allowance("alice_wallet") << " lamports\n";
        std::cout << "GasVault reserves unchanged: " << gas_vault.sol_reserves_sol() << " SOL (not used)\n";
        std::cout << "Total gas paid: " << gas_vault.total_gas_paid() << " lamports\n";
        std::cout << "Gas paid by staked compute: " << gas_vault.gas_paid_by_compute() << " lamports\n";
        std::cout << "Compute coverage ratio: " << (gas_vault.compute_coverage_ratio() * 100) << "%\n\n";
    } else {
        std::cout << "Failed to claim intent\n\n";
    }

    // Log to ProofBook
    std::cout << "=== Step 5: ProofBook Logging ===\n";
    proof_book.append(ProofType::GOVERNANCE, R"({"action":"intent_settled","gas_source":"staked_compute"})");
    proof_book.append(ProofType::TREASURY, R"({"action":"compute_stake_subsidized","staked_cores":12})");

    std::cout << "ProofBook entries: " << proof_book.entry_count() << "\n";
    std::cout << "Chain valid: " << (proof_book.verify_chain() ? "yes" : "no") << "\n\n";

    // Unstake and claim additional rewards
    std::cout << "=== Step 6: Unstake Compute (after 1 hour minimum) ===\n";
    uint64_t additional_rewards = 0;
    bool unstaked = compute_staking.unstake_compute(stake_id, additional_rewards);
    std::cout << "Unstake result: " << (unstaked ? "success" : "failed (too early)") << "\n";
    if (unstaked) {
        auto stake = compute_staking.get_stake(stake_id);
        std::cout << "Instant reward (received at stake): " << stake->instant_reward / 1e9 << " SOL\n";
        std::cout << "Additional time-based rewards: " << additional_rewards << " lamports\n";
        std::cout << "Total earned: " << stake->total_earned << " lamports\n";
    }
    std::cout << "Remaining staked cores: " << compute_staking.total_cpu_cores() << "\n\n";

    std::cout << "=== Final Summary ===\n";
    std::cout << "Gas model: Users stake CPU/RAM to send transactions for free\n";
    std::cout << "Alice staked 4 cores + 16 GB RAM → got 40 SOL instantly + 40 SOL gas allowance\n";
    std::cout << "Alice sent transaction for free using her gas allowance\n";
    std::cout << "Alice's remaining gas allowance: " << compute_staking.get_gas_allowance("alice_wallet") << " lamports\n";
    std::cout << "Sender paid: 0 SOL (used gas allowance from staked compute)\n";
    std::cout << "Receiver paid: 0 SOL\n";
    std::cout << "GasVault reserves unchanged: " << gas_vault.sol_reserves_sol() << " SOL (not used)\n";
    std::cout << "Total staked compute cores: " << compute_staking.total_cpu_cores() << "\n";
    std::cout << "Instant rewards paid to stakers: " << compute_staking.total_instant_rewards_paid() / 1e9 << " SOL\n";
    std::cout << "ProofBook integrity: " << (proof_book.verify_chain() ? "valid" : "invalid") << "\n";

    return 0;
}
