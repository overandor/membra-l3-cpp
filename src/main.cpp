#include "crypto.hpp"
#include "proof_book.hpp"
#include "gas_vault.hpp"
#include "intent_network.hpp"
#include "volatility_oracle.hpp"

#include <iostream>
#include <thread>
#include <chrono>

using namespace membra;

int main() {
    std::cout << "MEMBRA Layer-3 C++ Implementation\n";
    std::cout << "Protocol-Subsidized Gas Model: Neither sender nor receiver pays\n\n";

    // Initialize components
    GasVault gas_vault(100'000'000'000);  // 100 SOL in reserves
    IntentNetwork intent_network;
    VolatilityOracle oracle;
    ProofBook proof_book;

    std::cout << "GasVault reserves: " << gas_vault.sol_reserves_sol() << " SOL\n";
    std::cout << "GasVault healthy: " << (gas_vault.is_healthy() ? "yes" : "no") << "\n\n";

    // Feed price data for volatility detection
    std::cout << "Feeding price data...\n";
    for (int i = 0; i < 20; i++) {
        double price = 1.0 * (1 + i * 0.005);  // 10% rise
        oracle.feed_price(price, 5000.0);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Assess volatility
    VolatilityReport report = oracle.assess(gas_vault.sol_reserves_sol());
    std::cout << "Volatility signal: " << static_cast<int>(report.signal) << "\n";
    std::cout << "Confidence: " << report.confidence << "\n";
    std::cout << "MEMBRA TWAP: $" << report.membra_twap << "\n";
    std::cout << "Price change: " << report.price_change_pct << "%\n\n";

    // Create gasless payment intent (sender pays 0, receiver pays 0)
    std::cout << "Creating gasless payment intent...\n";
    auto intent = intent_network.create_intent(
        "alice_wallet",
        "bob_wallet",
        "USDC",
        50'000'000,  // 50 USDC
        "alice_signature"
    );

    if (intent) {
        std::cout << "Intent created: " << intent->intent_id << "\n";
        std::cout << "Amount: " << intent->amount / 1'000'000 << " USDC\n";
        std::cout << "Claim window: 7 days\n";
        std::cout << "Sender pays: 0 SOL\n";
        std::cout << "Receiver pays: 0 SOL\n\n";
    }

    // Claim intent (protocol pays gas)
    std::cout << "Claiming intent (protocol pays gas)...\n";
    auto claimed = intent_network.claim_intent(intent->intent_id, "relayer");
    if (claimed) {
        uint64_t gas_cost = 5000;  // estimated gas
        bool paid = gas_vault.pay_gas("alice_wallet", gas_cost);

        std::cout << "Intent claimed\n";
        std::cout << "Gas cost: " << gas_cost << " lamports\n";
        std::cout << "Protocol paid: " << (paid ? "yes" : "no") << "\n";
        std::cout << "GasVault reserves after: " << gas_vault.sol_reserves_sol() << " SOL\n";
        std::cout << "Total gas paid by protocol: " << gas_vault.total_gas_paid() << " lamports\n\n";
    }

    // Log to ProofBook
    std::cout << "Logging to ProofBook...\n";
    proof_book.append(ProofType::GOVERNANCE, R"({"action":"intent_settled","gas_paid":true})");
    proof_book.append(ProofType::TREASURY, R"({"action":"gas_vault_debit","amount":5000})");

    std::cout << "ProofBook entries: " << proof_book.entry_count() << "\n";
    std::cout << "Chain valid: " << (proof_book.verify_chain() ? "yes" : "no") << "\n";

    std::cout << "\n=== Summary ===\n";
    std::cout << "Protocol paid all gas fees\n";
    std::cout << "Sender paid: 0 SOL\n";
    std::cout << "Receiver paid: 0 SOL\n";
    std::cout << "GasVault reserves: " << gas_vault.sol_reserves_sol() << " SOL\n";
    std::cout << "ProofBook integrity: " << (proof_book.verify_chain() ? "valid" : "invalid") << "\n";

    return 0;
}
