#ifndef MERKLE_PROVENANCE_HPP
#define MERKLE_PROVENANCE_HPP

#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <memory>
#include "crypto.hpp"

namespace membra {
namespace tokenomics {

// Constants
constexpr size_t HASH_SIZE = 32;
constexpr size_t ATTESTATION_ID_SIZE = 16;
constexpr uint16_t MAX_SCORE_BPS = 10000;

/**
 * Merkle tree node for provenance tracking
 */
struct MerkleNode {
    std::array<uint8_t, HASH_SIZE> hash;
    std::array<uint8_t, HASH_SIZE> left_child;
    std::array<uint8_t, HASH_SIZE> right_child;
    bool has_left = false;
    bool has_right = false;
    
    MerkleNode() {
        hash.fill(0);
        left_child.fill(0);
        right_child.fill(0);
    }
    
    static MerkleNode create_leaf(const std::vector<uint8_t>& data);
    static MerkleNode create_internal(const std::array<uint8_t, HASH_SIZE>& left,
                                      const std::array<uint8_t, HASH_SIZE>& right);
};

/**
 * Merkle proof for verification
 */
struct MerkleProof {
    std::array<uint8_t, HASH_SIZE> leaf_hash;
    std::vector<std::array<uint8_t, HASH_SIZE>> siblings;
    uint64_t leaf_index;
    
    MerkleProof() : leaf_index(0) {
        leaf_hash.fill(0);
    }
    
    bool verify(const std::array<uint8_t, HASH_SIZE>& root) const;
};

/**
 * Token appraisal metrics
 */
struct TokenAppraisal {
    std::vector<uint8_t> novelty_factors;
    std::vector<uint8_t> rarity_factors;
    std::vector<uint8_t> execution_factors;
    std::vector<uint8_t> historical_data;
    
    std::array<uint8_t, HASH_SIZE> compute_merkle_root() const;
    std::tuple<uint16_t, uint16_t, uint16_t> calculate_scores() const;
};

/**
 * Market cap calculator based on appraisal scores
 */
class MarketCapCalculator {
public:
    static uint64_t calculate_appraisal(uint64_t base_value,
                                        uint16_t novelty_score,
                                        uint16_t rarity_score,
                                        uint16_t execution_difficulty);
};

/**
 * Provenance attestation for token appraisal
 */
struct ProvenanceAttestation {
    uint64_t attestation_id;
    std::array<uint8_t, 32> token_mint;  // Solana public key
    std::array<uint8_t, HASH_SIZE> merkle_root;
    uint64_t market_cap_appraisal;
    uint16_t novelty_score;
    uint16_t rarity_score;
    uint16_t execution_difficulty;
    int64_t attestation_timestamp;
    std::array<uint8_t, 32> authority;
    bool verified;
    
    ProvenanceAttestation() 
        : attestation_id(0), market_cap_appraisal(0), 
          novelty_score(0), rarity_score(0), execution_difficulty(0),
          attestation_timestamp(0), verified(false) {
        token_mint.fill(0);
        merkle_root.fill(0);
        authority.fill(0);
    }
    
    bool validate_scores() const {
        return novelty_score <= MAX_SCORE_BPS &&
               rarity_score <= MAX_SCORE_BPS &&
               execution_difficulty <= MAX_SCORE_BPS;
    }
};

/**
 * Provenance attestation builder
 */
class ProvenanceAttestationBuilder {
public:
    ProvenanceAttestationBuilder();
    
    ProvenanceAttestation create_attestation(
        const std::array<uint8_t, 32>& token_mint,
        const TokenAppraisal& appraisal,
        uint64_t base_value,
        const std::array<uint8_t, 32>& authority,
        uint64_t attestation_id
    );
    
    bool verify_attestation(const ProvenanceAttestation& attestation,
                           const MerkleProof& proof);
    
private:
    std::unique_ptr<Crypto> crypto_;
};

/**
 * Merkle tree builder for batch attestations
 */
class MerkleTreeBuilder {
public:
    MerkleTreeBuilder();
    
    void add_leaf(const std::vector<uint8_t>& data);
    std::array<uint8_t, HASH_SIZE> build_root();
    MerkleProof generate_proof(size_t leaf_index);
    
    size_t leaf_count() const { return leaves_.size(); }
    
private:
    std::vector<MerkleNode> leaves_;
    std::vector<MerkleNode> nodes_;
    std::unique_ptr<Crypto> crypto_;
    
    void build_tree();
    MerkleProof generate_proof_recursive(size_t leaf_index, size_t node_index, size_t tree_size);
};

} // namespace tokenomics
} // namespace membra

#endif // MERKLE_PROVENANCE_HPP