#include "../include/llm_mempool_governor.hpp"
#include <iostream>
#include <cassert>
#include <iomanip>

using namespace membra::governor;

void test_mempool_observation() {
    std::cout << "Testing mempool observation..." << std::endl;
    
    LLMMempoolGovernor governor;
    
    MempoolState state;
    state.snapshot_timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    
    // Add a pending transaction
    PendingTransaction tx;
    tx.transaction_hash = "abc123";
    tx.network = "ethereum";
    tx.value = 1000000;
    state.pending_transactions.push_back(tx);
    
    // Observe mempool
    auto proof = governor.observe_mempool(state);
    
    assert(!proof.observation_id.empty());
    assert(proof.network == "ethereum");
    
    std::cout << "✓ Mempool observation passed" << std::endl;
    std::cout << "  Observation ID: " << proof.observation_id << std::endl;
}

void test_transaction_review() {
    std::cout << "Testing transaction review..." << std::endl;
    
    LLMMempoolGovernor governor;
    
    PendingTransaction tx;
    tx.transaction_hash = "def456";
    tx.network = "solana";
    tx.value = 5000000;
    tx.data = "swap_tokens";
    
    MempoolState context;
    context.oracle_prices["SOL"] = 150.0;
    context.liquidity_depth["SOL-USDC"] = 1000000.0;
    
    // Review transaction
    auto analysis = governor.review_transaction(tx, context);
    
    assert(analysis.transaction_hash == "def456");
    assert(analysis.confidence > 0);
    assert(analysis.risk_score <= 100);
    
    std::cout << "✓ Transaction review passed" << std::endl;
    std::cout << "  Intent: " << static_cast<int>(analysis.intent) << std::endl;
    std::cout << "  Confidence: " << static_cast<int>(analysis.confidence) << "%" << std::endl;
    std::cout << "  Risk Score: " << static_cast<int>(analysis.risk_score) << "%" << std::endl;
}

void test_next_action_computation() {
    std::cout << "Testing next action computation..." << std::endl;
    
    LLMMempoolGovernor governor;
    
    MempoolState state;
    state.snapshot_timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    
    // Add some transactions
    for (int i = 0; i < 5; i++) {
        PendingTransaction tx;
        tx.transaction_hash = "tx" + std::to_string(i);
        tx.network = "ethereum";
        tx.value = 1000000 * (i + 1);
        state.pending_transactions.push_back(tx);
    }
    
    // Compute next action
    auto action = governor.compute_next_action(state);
    
    assert(action.confidence > 0);
    assert(action.timestamp > 0);
    
    std::cout << "✓ Next action computation passed" << std::endl;
    std::cout << "  Action: " << static_cast<int>(action.action) << std::endl;
    std::cout << "  Confidence: " << static_cast<int>(action.confidence) << "%" << std::endl;
}

void test_policy_compliance() {
    std::cout << "Testing policy compliance..." << std::endl;
    
    LLMMempoolGovernor governor;
    
    ActionRecommendation action;
    action.action = NextAction::REROUTE_LIQUIDITY;
    action.confidence = 80;
    action.risk_score = 30;
    action.requires_human_approval = false;
    
    PolicyRule rule;
    rule.rule_id = "test_rule";
    rule.max_risk_threshold = 0.7;
    rule.min_confidence_threshold = 0.6;
    
    std::vector<PolicyRule> rules = {rule};
    
    bool compliant = governor.check_policy_compliance(action, rules);
    assert(compliant == true);
    
    // Test non-compliant action
    action.risk_score = 80;
    compliant = governor.check_policy_compliance(action, rules);
    assert(compliant == false);
    
    std::cout << "✓ Policy compliance passed" << std::endl;
}

void test_action_simulation() {
    std::cout << "Testing action simulation..." << std::endl;
    
    LLMMempoolGovernor governor;
    
    ActionRecommendation action;
    action.action = NextAction::REROUTE_LIQUIDITY;
    action.confidence = 80;
    action.risk_score = 20;
    
    MempoolState context;
    
    // Simulate action
    auto simulation = governor.simulate_action(action, context);
    
    assert(simulation.success == true);
    assert(simulation.expected_profit >= 0);
    
    std::cout << "✓ Action simulation passed" << std::endl;
    std::cout << "  Expected Profit: " << simulation.expected_profit << std::endl;
    std::cout << "  Execution Time: " << simulation.execution_time_seconds << "s" << std::endl;
}

void test_overmanifold_policy() {
    std::cout << "Testing Overmanifold policy..." << std::endl;
    
    OvermanifoldPolicy policy;
    
    ActionRecommendation safe_action;
    safe_action.action = NextAction::DO_NOTHING;
    safe_action.confidence = 100;
    safe_action.risk_score = 0;
    safe_action.requires_human_approval = false;
    
    SimulationResult safe_simulation;
    safe_simulation.success = true;
    
    // Test safe action execution
    bool executed = policy.execute_action(safe_action, safe_simulation);
    assert(executed == true);
    
    // Test unsafe action
    ActionRecommendation unsafe_action;
    unsafe_action.action = NextAction::REROUTE_LIQUIDITY;
    unsafe_action.confidence = 30;
    unsafe_action.risk_score = 80;
    unsafe_action.requires_human_approval = true;
    
    bool unsafe_executed = policy.execute_action(unsafe_action, safe_simulation);
    assert(unsafe_executed == false);
    
    std::cout << "✓ Overmanifold policy passed" << std::endl;
}

void test_mempool_router() {
    std::cout << "Testing mempool router..." << std::endl;
    
    MempoolRouter router;
    
    TransactionAnalysis analysis;
    analysis.transaction_hash = "test_tx";
    analysis.intent = TransactionIntent::ARBITRAGE;
    analysis.confidence = 85;
    analysis.risk_score = 15;
    analysis.price_impact_estimate = 0.01;
    
    MempoolState context;
    context.route_state["ethereum"] = "active";
    
    // Test route compilation
    std::string route = router.compile_route(analysis, context);
    assert(!route.empty());
    
    // Test hedge generation
    std::string hedge = router.generate_hedge(analysis, context);
    assert(!hedge.empty());
    
    // Test alert generation
    std::string alert = router.generate_alert(analysis, "high");
    assert(!alert.empty());
    
    std::cout << "✓ Mempool router passed" << std::endl;
    std::cout << "  Route: " << route << std::endl;
}

void test_api_endpoints() {
    std::cout << "Testing API endpoints..." << std::endl;
    
    auto stack = create_mempool_governor();
    
    // Test observe endpoint
    std::string observe_response = stack.api->handle_observe_mempool("{}");
    assert(!observe_response.empty());
    
    // Test review endpoint
    std::string review_response = stack.api->handle_review_transaction("{}");
    assert(!review_response.empty());
    
    // Test next action endpoint
    std::string action_response = stack.api->handle_compute_next_action("{}");
    assert(!action_response.empty());
    
    // Test route endpoint
    std::string route_response = stack.api->handle_route_from_signal("{}");
    assert(!route_response.empty());
    
    // Test simulate endpoint
    std::string simulate_response = stack.api->handle_simulate_action("{}");
    assert(!simulate_response.empty());
    
    std::cout << "✓ API endpoints passed" << std::endl;
}

void test_safety_invariant() {
    std::cout << "Testing safety invariant (LLM recommends, deterministic policy executes)..." << std::endl;
    
    auto stack = create_mempool_governor();
    
    // LLM recommends an action
    MempoolState state;
    state.snapshot_timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    
    PendingTransaction tx;
    tx.transaction_hash = "safety_test";
    tx.network = "ethereum";
    tx.value = 1000000;
    state.pending_transactions.push_back(tx);
    
    // LLM analysis
    auto analysis = stack.governor->review_transaction(tx, state);
    auto action = stack.governor->compute_next_action(state);
    
    // Deterministic policy check
    SimulationResult simulation = stack.governor->simulate_action(action, state);
    bool policy_approved = stack.policy->execute_action(action, simulation);
    
    // Safety invariant: LLM doesn't directly move funds
    assert(action.unsigned_transaction_plan.empty() || policy_approved == false);
    
    std::cout << "✓ Safety invariant passed" << std::endl;
    std::cout << "  LLM confidence: " << static_cast<int>(action.confidence) << "%" << std::endl;
    std::cout << "  Policy approved: " << (policy_approved ? "true" : "false") << std::endl;
}

void test_proof_generation() {
    std::cout << "Testing proof generation..." << std::endl;
    
    LLMMempoolGovernor governor;
    
    MempoolState state;
    state.snapshot_timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    
    PendingTransaction tx;
    tx.transaction_hash = "proof_test";
    tx.network = "solana";
    state.pending_transactions.push_back(tx);
    
    // Generate observation proof
    auto proof = governor.observe_mempool(state);
    auto proof_hash = governor.generate_observation_proof(proof);
    
    // Verify proof is not empty
    bool all_zeros = true;
    for (uint8_t byte : proof_hash) {
        if (byte != 0) {
            all_zeros = false;
            break;
        }
    }
    assert(all_zeros == false);
    
    std::cout << "✓ Proof generation passed" << std::endl;
    std::cout << "  Proof ID: " << proof.observation_id << std::endl;
}

void test_transaction_intent_classification() {
    std::cout << "Testing transaction intent classification..." << std::endl;
    
    LLMMempoolGovernor governor;
    
    // Test various transaction types
    PendingTransaction swap_tx;
    swap_tx.transaction_hash = "swap";
    swap_tx.data = "";
    swap_tx.value = 1000;
    
    auto swap_analysis = governor.review_transaction(swap_tx, MempoolState());
    assert(swap_analysis.intent == TransactionIntent::SWAP);
    
    PendingTransaction bridge_tx;
    bridge_tx.transaction_hash = "bridge";
    bridge_tx.data = "bridge_tokens";
    bridge_tx.value = 1000;
    
    auto bridge_analysis = governor.review_transaction(bridge_tx, MempoolState());
    assert(bridge_analysis.intent == TransactionIntent::BRIDGE);
    
    PendingTransaction stake_tx;
    stake_tx.transaction_hash = "stake";
    stake_tx.data = "stake_tokens";
    stake_tx.value = 1000;
    
    auto stake_analysis = governor.review_transaction(stake_tx, MempoolState());
    assert(stake_analysis.intent == TransactionIntent::STAKE);
    
    std::cout << "✓ Transaction intent classification passed" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "LLM Mempool Governor Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        test_mempool_observation();
        test_transaction_review();
        test_next_action_computation();
        test_policy_compliance();
        test_action_simulation();
        test_overmanifold_policy();
        test_mempool_router();
        test_api_endpoints();
        test_safety_invariant();
        test_proof_generation();
        test_transaction_intent_classification();
        
        std::cout << "========================================" << std::endl;
        std::cout << "✅ All tests passed!" << std::endl;
        std::cout << "========================================" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
}