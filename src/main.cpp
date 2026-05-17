#include "crypto.hpp"
#include "proof_book.hpp"
#include "gas_vault.hpp"
#include "intent_network.hpp"
#include "volatility_oracle.hpp"
#include "compute_staking.hpp"
#include "compute_futures.hpp"
#include "compute_benchmark.hpp"
#include "proof_of_inference.hpp"
#include "reward_vault.hpp"
#include "receiver_paid_gas.hpp"

#include <iostream>
#include <thread>
#include <chrono>

using namespace membra;

int main() {
    std::cout << "MEMBRA Layer-3 C++ Implementation\n";
    std::cout << "Compute-Collateralized Gas Model with MEMBRA Gas Credits\n";
    std::cout << "Users lock CPU/RAM to receive MEMBRA Gas Credits\n";
    std::cout << "Gas Credits make MEMBRA intents sender-gasless\n";
    std::cout << "Real Solana fees require funded GasVault or relayer\n";
    std::cout << "Mainnet disabled by default\n\n";

    // Initialize components
    ComputeStaking compute_staking;
    ComputePriceOracle compute_oracle;
    ComputeFuturesMarket futures_market(&compute_oracle);
    ComputeBenchmark benchmark;
    ProofOfInference proof_of_inference;
    RewardVault reward_vault(100'000'000'000);  // 100 SOL initial pool
    ReceiverPaidGas receiver_gas;
    GasVault gas_vault(100'000'000'000, &compute_staking);  // 100 SOL + compute staking
    IntentNetwork intent_network;
    VolatilityOracle oracle;
    ProofBook proof_book;

    std::cout << "GasVault reserves: " << gas_vault.sol_reserves_sol() << " SOL (backup)\n";
    std::cout << "Staked compute: " << compute_staking.total_cpu_cores() << " cores\n";
    std::cout << "GasVault healthy: " << (gas_vault.is_healthy() ? "yes" : "no") << "\n\n";

    // Alice stakes CPU/RAM to receive MEMBRA Gas Credits
    std::cout << "=== Step 1: Alice stakes CPU/RAM to receive MEMBRA Gas Credits ===\n";
    uint64_t alice_credit_grant = 0;
    std::string stake_id = compute_staking.stake_compute("alice_wallet", 4, 16, 4000, alice_credit_grant);
    std::cout << "Stake ID: " << stake_id << "\n";
    std::cout << "Alice staked: 4 CPU cores, 16 GB RAM, 4000 compute units\n";
    std::cout << "MEMBRA issues: " << alice_credit_grant / 1e9 << " SOL-equivalent MEMBRA Gas Credits\n";
    std::cout << "Alice's gas credit allowance: " << compute_staking.get_gas_allowance("alice_wallet") / 1e9 << " SOL-equivalent credits\n";
    std::cout << "⚠️  Gas Credits are NOT real SOL - for MEMBRA fee accounting only\n";
    std::cout << "⚠️  Real Solana settlement requires funded GasVault or relayer\n";
    std::cout << "Total staked cores: " << compute_staking.total_cpu_cores() << "\n\n";

    // Bob also stakes
    uint64_t bob_credit_grant = 0;
    std::string bob_stake = compute_staking.stake_compute("bob_wallet", 8, 32, 8000, bob_credit_grant);
    std::cout << "Bob staked: 8 CPU cores, 32 GB RAM, 8000 compute units\n";
    std::cout << "MEMBRA issues: " << bob_credit_grant / 1e9 << " SOL-equivalent MEMBRA Gas Credits\n";
    std::cout << "Bob's gas credit allowance: " << compute_staking.get_gas_allowance("bob_wallet") / 1e9 << " SOL-equivalent credits\n";
    std::cout << "Total staked cores: " << compute_staking.total_cpu_cores() << "\n";
    std::cout << "Total credit grants issued: " << compute_staking.total_credit_grants_issued() / 1e9 << " SOL-equivalent credits\n\n";

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

    // Create gasless payment intent (Alice uses her gas credits)
    std::cout << "=== Step 3: Alice sends gasless MEMBRA intent ===\n";
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
    std::cout << "Alice's gas credit allowance before: " << compute_staking.get_gas_allowance("alice_wallet") << " credit units\n";
    std::cout << "Claim window: 7 days\n\n";

    // Claim intent (gas accounted against Alice's gas credits)
    std::cout << "=== Step 4: Settlement with Gas Credit Accounting ===\n";
    auto claimed = intent_network.claim_intent(intent.intent_id, "relayer");
    if (claimed) {
        uint64_t gas_cost = 5000;  // estimated gas
        bool paid = gas_vault.pay_gas("alice_wallet", gas_cost);

        std::cout << "Intent claimed\n";
        std::cout << "Gas cost: " << gas_cost << " lamports\n";
        std::cout << "Gas accounted against: Alice's MEMBRA Gas Credits\n";
        std::cout << "Alice's gas credit allowance after: " << compute_staking.get_gas_allowance("alice_wallet") << " credit units\n";
        std::cout << "GasVault reserves: " << gas_vault.sol_reserves_sol() << " SOL\n";
        std::cout << "Total gas accounted: " << gas_vault.total_gas_paid() << " lamports\n";
        std::cout << "Gas accounted against credits: " << gas_vault.gas_paid_by_compute() << " lamports\n";
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

    // Unstake and claim additional credits
    std::cout << "=== Step 6: Unstake Compute (after 1 hour minimum) ===\n";
    uint64_t additional_credits = 0;
    bool unstaked = compute_staking.unstake_compute(stake_id, additional_credits);
    std::cout << "Unstake result: " << (unstaked ? "success" : "failed (too early)") << "\n";
    if (unstaked) {
        auto stake = compute_staking.get_stake(stake_id);
        std::cout << "Instant credit grant (received at stake): " << stake->instant_credit_grant << " credit units\n";
        std::cout << "Additional time-based credits: " << additional_credits << " credit units\n";
        std::cout << "Total credit grants: " << stake->total_credit_grants_issued << " credit units\n";
        std::cout << "⚠️  Credits are MEMBRA internal accounting, not redeemable for SOL\n";
    }
    std::cout << "Remaining staked cores: " << compute_staking.total_cpu_cores() << "\n\n";

    // === Step 7: Compute Futures Market ===
    std::cout << "=== Step 7: Compute Futures Market ===\n";
    
    // Feed compute prices
    compute_oracle.feed_price(0.1, 0.05);  // CPU: 0.1 SOL/core, RAM: 0.05 SOL/GB
    std::cout << "Current compute index: " << compute_oracle.current_index() << "\n";
    std::cout << "CPU price: " << compute_oracle.current_cpu_price() << " SOL/core\n";
    std::cout << "RAM price: " << compute_oracle.current_ram_price() << " SOL/GB\n\n";
    
    // Charlie opens a forward contract (lock compute for future use)
    std::string forward_id = futures_market.open_forward(
        "charlie_wallet",
        8,      // 8 cores
        32,     // 32 GB RAM
        86400.0,  // 24 hours
        10.0    // 10 SOL collateral
    );
    std::cout << "Charlie opened forward contract: " << forward_id << "\n";
    std::cout << "Locked: 8 cores + 32 GB RAM for 24 hours\n\n";
    
    // Dave opens a speculative long (bet price rises)
    std::string long_id = futures_market.open_long(
        "dave_wallet",
        4,      // 4 cores
        16,     // 16 GB RAM
        5.0,    // 5x leverage
        20.0,   // 20 SOL collateral
        43200.0 // 12 hours
    );
    std::cout << "Dave opened long position: " << long_id << "\n";
    std::cout << "Bet: compute price will rise (5x leverage)\n\n";
    
    // Eve opens a speculative short (bet price falls)
    std::string short_id = futures_market.open_short(
        "eve_wallet",
        4,      // 4 cores
        16,     // 16 GB RAM
        3.0,    // 3x leverage
        15.0,   // 15 SOL collateral
        43200.0 // 12 hours
    );
    std::cout << "Eve opened short position: " << short_id << "\n";
    std::cout << "Bet: compute price will fall (3x leverage)\n\n";
    
    std::cout << "Open futures count: " << futures_market.open_futures_count() << "\n\n";
    
    // Simulate price movement
    std::cout << "Simulating compute price movement...\n";
    compute_oracle.feed_price(0.15, 0.08);  // Price rises
    std::cout << "New compute index: " << compute_oracle.current_index() << " (+50%)\n";
    std::cout << "Price change: " << compute_oracle.price_change_pct(10.0) << "%\n\n";
    
    // Check liquidations
    auto liquidated = futures_market.check_liquidations();
    std::cout << "Liquidated positions: " << liquidated.size() << "\n\n";

    // === Step 8: Phase-Two Infrastructure (Development) ===
    std::cout << "=== Step 8: Phase-Two Infrastructure ===\n";
    
    // M5 Benchmark
    std::cout << "--- M5 Benchmark ---\n";
    auto bench_result = benchmark.run_benchmark("M5_MacBook_Pro");
    std::cout << "Hardware: " << bench_result.hardware_type << "\n";
    std::cout << "CPU: " << bench_result.cpu_cores << " cores @ " << bench_result.cpu_frequency_ghz << " GHz\n";
    std::cout << "RAM: " << bench_result.ram_gb << " GB @ " << bench_result.memory_bandwidth_gbps << " GB/s\n";
    std::cout << "SHA256 hashes/sec: " << bench_result.sha256_hashes_per_sec << "\n";
    std::cout << "Inference ops/sec: " << bench_result.inference_ops_per_sec << "\n";
    double m5_ratio = benchmark.compare_to_m5(bench_result);
    std::cout << "Performance vs M5 baseline: " << (m5_ratio * 100) << "%\n";
    double daily_earnings = benchmark.estimate_daily_sol_earnings(bench_result);
    std::cout << "Estimated daily earnings (conservative): $" << daily_earnings << " SOL\n";
    std::cout << "⚠️  Actual earnings depend on network demand\n\n";
    
    // Proof-of-Inference
    std::cout << "--- Proof-of-Inference ---\n";
    std::string receipt_id = proof_of_inference.create_receipt(
        "worker_wallet",
        "task_123",
        InferenceType::PROOF_GENERATION,
        123456789,
        987654321,
        10000,
        5000000,
        1024000,
        0.5
    );
    std::cout << "Created inference receipt: " << receipt_id << "\n";
    bool verified = proof_of_inference.verify_receipt(receipt_id);
    std::cout << "Receipt verified: " << (verified ? "yes" : "no") << "\n";
    std::cout << "Total compute units: " << proof_of_inference.total_compute_units() << "\n\n";
    
    // Reward Vault
    std::cout << "--- Reward Vault ---\n";
    std::string reward_entry = reward_vault.create_reward_entry(
        "worker_wallet",
        receipt_id,
        50000000,  // 0.05 SOL
        "inference"
    );
    std::cout << "Created reward entry: " << reward_entry << "\n";
    std::cout << "Pool balance: " << reward_vault.pool_balance() / 1e9 << " SOL\n";
    std::cout << "Total rewards claimed: " << reward_vault.total_rewards_claimed() / 1e9 << " SOL\n";
    bool reward_claimed = reward_vault.claim_reward(reward_entry);
    std::cout << "Reward claimed: " << (reward_claimed ? "yes" : "no") << "\n";
    std::cout << "Pool balance after claim: " << reward_vault.pool_balance() / 1e9 << " SOL\n\n";
    
    // Receiver-Paid Gas (devnet only)
    std::cout << "--- Receiver-Paid Gas (devnet only) ---\n";
    receiver_gas.update_oracle(5000, 1000, 150.0, 0.10);
    std::cout << "Gas oracle updated\n";
    std::string gas_receipt = receiver_gas.create_receipt(
        "tx_456",
        "receiver_wallet",
        "sender_wallet",
        5000,    // gas paid
        1000,    // priority fee
        12345,
        "signature_xyz"
    );
    std::cout << "Created gas receipt: " << gas_receipt << "\n";
    std::cout << "Compensation enabled: " << (receiver_gas.get_oracle_state().is_stale() ? "no (stale)" : "yes") << "\n";
    std::cout << "⚠️  Receiver-paid gas is devnet only\n\n";

    std::cout << "=== Final Summary ===\n";
    std::cout << "Gas model: Users lock CPU/RAM to receive MEMBRA Gas Credits\n";
    std::cout << "Alice staked 4 cores + 16 GB RAM → received 40 SOL-equivalent MEMBRA Gas Credits\n";
    std::cout << "Alice sent gasless MEMBRA intent using gas credits\n";
    std::cout << "Alice's remaining gas credit allowance: " << compute_staking.get_gas_allowance("alice_wallet") << " credit units\n";
    std::cout << "Sender paid: 0 SOL (gas accounted against MEMBRA credits)\n";
    std::cout << "Receiver paid: 0 SOL\n";
    std::cout << "GasVault reserves: " << gas_vault.sol_reserves_sol() << " SOL\n";
    std::cout << "Total staked compute cores: " << compute_staking.total_cpu_cores() << "\n";
    std::cout << "Total credit grants issued: " << compute_staking.total_credit_grants_issued() / 1e9 << " SOL-equivalent credits\n";
    std::cout << "Compute futures market: " << futures_market.open_futures_count() << " open positions\n";
    std::cout << "Compute index: " << compute_oracle.current_index() << "\n";
    std::cout << "M5 benchmark: " << bench_result.inference_ops_per_sec << " ops/sec\n";
    std::cout << "Proof-of-inference receipts: " << proof_of_inference.receipt_count() << "\n";
    std::cout << "Reward vault pool: " << reward_vault.pool_balance() / 1e9 << " SOL\n";
    std::cout << "⚠️  MEMBRA Gas Credits are NOT real SOL\n";
    std::cout << "⚠️  Real Solana settlement requires funded GasVault, relayer, or receiver-paid model\n";
    std::cout << "⚠️  Mainnet disabled by default\n";
    std::cout << "ProofBook integrity: " << (proof_book.verify_chain() ? "valid" : "invalid") << "\n";

    return 0;
}
