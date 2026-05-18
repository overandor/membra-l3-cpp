#ifndef LLM_MEMPOOL_GOVERNOR_HPP
#define LLM_MEMPOOL_GOVERNOR_HPP

#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <memory>
#include <map>
#include <functional>
#include <chrono>
#include "crypto.hpp"
#include "merkle_provenance.hpp"

namespace membra {
namespace governor {

// Constants
constexpr size_t HASH_SIZE = 32;
constexpr size_t WALLET_ADDRESS_SIZE = 32;
constexpr uint8_t MAX_CONFIDENCE = 100;
constexpr uint8_t MAX_RISK_SCORE = 100;

/**
 * Transaction classification intent types
 */
enum class TransactionIntent {
    UNKNOWN,
    ARBITRAGE,
    LIQUIDITY_PROVISION,
    LIQUIDITY_REMOVAL,
    SWAP,
    BRIDGE,
    STAKE,
    UNSTAKE,
    GOVERNANCE,
    NFT_MINT,
    NFT_TRANSFER,
    SPAM,
    ATTACK,
    MEV_EXTRACTION
};

/**
 * Next action types for protocol response
 */
enum class NextAction {
    DO_NOTHING,
    UPDATE_ORACLE,
    TRIGGER_ALERT,
    CREATE_MERKLE_PROOF,
    SCHEDULE_WORKER,
    REROUTE_LIQUIDITY,
    REBALANCE_POOL,
    HEDGE_EXPOSURE,
    BRIDGE_LIQUIDITY,
    SPONSOR_USER_TX,
    DELAY_TX,
    REQUIRE_HUMAN_APPROVAL,
    BLOCK_UNSAFE_ACTION
};

/**
 * Pending transaction representation
 */
struct PendingTransaction {
    std::string transaction_hash;
    std::string network; // "solana", "ethereum", etc.
    std::string from_address;
    std::string to_address;
    uint64_t value;
    std::string data; // Transaction calldata/payload
    uint64_t gas_price;
    uint64_t gas_limit;
    int64_t timestamp;
    
    PendingTransaction() : value(0), gas_price(0), gas_limit(0), timestamp(0) {}
};

/**
 * Mempool state snapshot
 */
struct MempoolState {
    std::vector<PendingTransaction> pending_transactions;
    std::vector<PendingTransaction> confirmed_transactions;
    std::map<std::string, double> oracle_prices;
    std::map<std::string, double> liquidity_depth;
    std::map<std::string, std::string> route_state;
    std::map<std::string, uint8_t> wallet_permissions;
    std::map<std::string, double> protocol_kpis;
    int64_t snapshot_timestamp;
    
    MempoolState() : snapshot_timestamp(0) {}
};

/**
 * Transaction analysis result
 */
struct TransactionAnalysis {
    std::string transaction_hash;
    TransactionIntent intent;
    double arbitrage_opportunity; // 0.0 to 1.0
    double mev_risk; // 0.0 to 1.0
    double bridge_demand; // 0.0 to 1.0
    double liquidity_shift; // 0.0 to 1.0
    double spam_or_attack_probability; // 0.0 to 1.0
    double price_impact_estimate; // 0.0 to 1.0
    double confirmation_risk; // 0.0 to 1.0
    uint8_t confidence; // 0 to 100
    uint8_t risk_score; // 0 to 100
    std::string rationale;
    
    TransactionAnalysis() 
        : intent(TransactionIntent::UNKNOWN),
          arbitrage_opportunity(0.0), mev_risk(0.0), bridge_demand(0.0),
          liquidity_shift(0.0), spam_or_attack_probability(0.0),
          price_impact_estimate(0.0), confirmation_risk(0.0),
          confidence(0), risk_score(0) {}
};

/**
 * Next action recommendation
 */
struct ActionRecommendation {
    NextAction action;
    std::string target_protocol;
    uint8_t confidence; // 0 to 100
    uint8_t risk_score; // 0 to 100
    bool requires_human_approval;
    std::string unsigned_transaction_plan;
    std::string simulation_result;
    std::string policy_compliance_proof;
    int64_t timestamp;
    
    ActionRecommendation()
        : action(NextAction::DO_NOTHING),
          confidence(0), risk_score(0),
          requires_human_approval(false),
          timestamp(0) {}
};

/**
 * Mempool observation proof
 */
struct MempoolObservationProof {
    std::string observation_id;
    std::array<uint8_t, HASH_SIZE> mempool_state_hash;
    std::array<uint8_t, HASH_SIZE> transaction_hash;
    int64_t observation_timestamp;
    int64_t snapshot_timestamp;
    std::string network;
    std::string observer_signature;
    
    MempoolObservationProof() : observation_timestamp(0), snapshot_timestamp(0) {
        mempool_state_hash.fill(0);
        transaction_hash.fill(0);
    }
};

/**
 * Policy rule for deterministic execution
 */
struct PolicyRule {
    std::string rule_id;
    std::string condition;
    std::string action;
    uint8_t priority; // 0 to 100
    bool requires_human_approval;
    double max_risk_threshold;
    double min_confidence_threshold;
    
    PolicyRule()
        : priority(50), requires_human_approval(false),
          max_risk_threshold(0.7), min_confidence_threshold(0.6) {}
};

/**
 * Simulation result for action validation
 */
struct SimulationResult {
    bool success;
    double expected_profit;
    double expected_gas_cost;
    double expected_slippage;
    double execution_time_seconds;
    std::string error_message;
    std::vector<std::string> warnings;
    
    SimulationResult()
        : success(false), expected_profit(0.0), expected_gas_cost(0.0),
          expected_slippage(0.0), execution_time_seconds(0.0) {}
};

/**
 * LLM mempool governor - main orchestrator
 */
class LLMMempoolGovernor {
public:
    LLMMempoolGovernor();
    ~LLMMempoolGovernor() = default;
    
    // Mempool observation
    MempoolObservationProof observe_mempool(const MempoolState& state);
    
    // Transaction analysis
    TransactionAnalysis review_transaction(const PendingTransaction& tx,
                                         const MempoolState& context);
    
    // Next action computation
    ActionRecommendation compute_next_action(const MempoolState& state);
    
    // Routing from mempool signal
    std::string route_from_signal(const TransactionAnalysis& analysis,
                                  const MempoolState& context);
    
    // Action simulation
    SimulationResult simulate_action(const ActionRecommendation& action,
                                    const MempoolState& context);
    
    // Policy engine
    bool check_policy_compliance(const ActionRecommendation& action,
                                const std::vector<PolicyRule>& rules);
    
    // Proof generation
    std::array<uint8_t, HASH_SIZE> generate_observation_proof(
        const MempoolObservationProof& observation);
    
    // Configuration
    void set_policy_rules(const std::vector<PolicyRule>& rules);
    void set_risk_threshold(double threshold);
    void set_confidence_threshold(double threshold);
    
private:
    std::vector<PolicyRule> policy_rules_;
    double risk_threshold_;
    double confidence_threshold_;
    std::unique_ptr<Crypto> crypto_;
    std::unique_ptr<tokenomics::MerkleTreeBuilder> merkle_builder_;
    
    // Internal analysis methods
    TransactionIntent classify_intent(const PendingTransaction& tx);
    double detect_arbitrage(const PendingTransaction& tx, const MempoolState& context);
    double detect_mev_risk(const PendingTransaction& tx, const MempoolState& context);
    double estimate_price_impact(const PendingTransaction& tx, const MempoolState& context);
    
    // Hash computation for state
    std::array<uint8_t, HASH_SIZE> hash_mempool_state(const MempoolState& state);
};

/**
 * Overmanifold execution policy layer
 */
class OvermanifoldPolicy {
public:
    OvermanifoldPolicy();
    ~OvermanifoldPolicy() = default;
    
    // Policy execution
    bool execute_action(const ActionRecommendation& action,
                      const SimulationResult& simulation);
    
    // Safety checks
    bool is_action_safe(const ActionRecommendation& action);
    bool is_within_risk_limits(const ActionRecommendation& action);
    bool has_required_approvals(const ActionRecommendation& action);
    
    // Policy management
    void add_policy_rule(const PolicyRule& rule);
    void remove_policy_rule(const std::string& rule_id);
    std::vector<PolicyRule> get_active_rules() const;
    
private:
    std::vector<PolicyRule> policy_rules_;
    std::unique_ptr<Crypto> crypto_;
};

/**
 * Router - action compiler from mempool signals
 */
class MempoolRouter {
public:
    MempoolRouter();
    ~MempoolRouter() = default;
    
    // Route compilation
    std::string compile_route(const TransactionAnalysis& analysis,
                              const MempoolState& context);
    
    // Hedge generation
    std::string generate_hedge(const TransactionAnalysis& analysis,
                               const MempoolState& context);
    
    // Bridge routing
    std::string route_bridge(const TransactionAnalysis& analysis,
                             const MempoolState& context);
    
    // Alert generation
    std::string generate_alert(const TransactionAnalysis& analysis,
                              const std::string& severity);
    
    // Worker task scheduling
    std::string schedule_worker_task(const TransactionAnalysis& analysis,
                                     const std::string& task_type);
    
private:
    std::unique_ptr<Crypto> crypto_;
};

/**
 * API endpoint handlers
 */
class MempoolGovernorAPI {
public:
    MempoolGovernorAPI(std::shared_ptr<LLMMempoolGovernor> governor,
                       std::shared_ptr<OvermanifoldPolicy> policy,
                       std::shared_ptr<MempoolRouter> router);
    ~MempoolGovernorAPI() = default;
    
    // Endpoint handlers
    std::string handle_observe_mempool(const std::string& request_json);
    std::string handle_review_transaction(const std::string& request_json);
    std::string handle_compute_next_action(const std::string& request_json);
    std::string handle_route_from_signal(const std::string& request_json);
    std::string handle_simulate_action(const std::string& request_json);
    
private:
    std::shared_ptr<LLMMempoolGovernor> governor_;
    std::shared_ptr<OvermanifoldPolicy> policy_;
    std::shared_ptr<MempoolRouter> router_;
    
    // JSON serialization helpers
    std::string serialize_analysis(const TransactionAnalysis& analysis);
    std::string serialize_recommendation(const ActionRecommendation& recommendation);
    std::string serialize_simulation(const SimulationResult& simulation);
};

/**
 * Factory function
 */
struct MempoolGovernorStack {
    std::shared_ptr<LLMMempoolGovernor> governor;
    std::shared_ptr<OvermanifoldPolicy> policy;
    std::shared_ptr<MempoolRouter> router;
    std::shared_ptr<MempoolGovernorAPI> api;
};

MempoolGovernorStack create_mempool_governor();

} // namespace governor
} // namespace membra

#endif // LLM_MEMPOOL_GOVERNOR_HPP