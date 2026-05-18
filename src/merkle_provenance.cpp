#include "merkle_provenance.hpp"
#include <algorithm>
#include <stdexcept>

namespace membra {
namespace tokenomics {

// ============================================================================
// MerkleNode Implementation
// ============================================================================

MerkleNode MerkleNode::create_leaf(const std::vector<uint8_t>& data) {
    MerkleNode node;
    Crypto crypto;
    node.hash = crypto.sha256(data);
    return node;
}

MerkleNode MerkleNode::create_internal(const std::array<uint8_t, HASH_SIZE>& left,
                                       const std::array<uint8_t, HASH_SIZE>& right) {
    MerkleNode node;
    Crypto crypto;
    
    std::vector<uint8_t> combined;
    combined.insert(combined.end(), left.begin(), left.end());
    combined.insert(combined.end(), right.begin(), right.end());
    
    node.hash = crypto.sha256(combined);
    node.left_child = left;
    node.right_child = right;
    node.has_left = true;
    node.has_right = true;
    
    return node;
}

// ============================================================================
// MerkleProof Implementation
// ============================================================================

bool MerkleProof::verify(const std::array<uint8_t, HASH_SIZE>& root) const {
    Crypto crypto;
    std::array<uint8_t, HASH_SIZE> current_hash = leaf_hash;
    uint64_t current_index = leaf_index;
    
    for (const auto& sibling : siblings) {
        std::vector<uint8_t> combined;
        
        if (current_index % 2 == 0) {
            // Current hash is left child
            combined.insert(combined.end(), current_hash.begin(), current_hash.end());
            combined.insert(combined.end(), sibling.begin(), sibling.end());
        } else {
            // Current hash is right child
            combined.insert(combined.end(), sibling.begin(), sibling.end());
            combined.insert(combined.end(), current_hash.begin(), current_hash.end());
        }
        
        current_hash = crypto.sha256(combined);
        current_index /= 2;
    }
    
    return current_hash == root;
}

// ============================================================================
// TokenAppraisal Implementation
// ============================================================================

std::array<uint8_t, HASH_SIZE> TokenAppraisal::compute_merkle_root() const {
    Crypto crypto;
    
    std::vector<uint8_t> data;
    data.insert(data.end(), novelty_factors.begin(), novelty_factors.end());
    data.insert(data.end(), rarity_factors.begin(), rarity_factors.end());
    data.insert(data.end(), execution_factors.begin(), execution_factors.end());
    data.insert(data.end(), historical_data.begin(), historical_data.end());
    
    return crypto.sha256(data);
}

std::tuple<uint16_t, uint16_t, uint16_t> TokenAppraisal::calculate_scores() const {
    auto calculate_score = [](const std::vector<uint8_t>& factors) -> uint16_t {
        if (factors.empty()) return 0;
        
        uint32_t sum = 0;
        for (uint8_t factor : factors) {
            sum += factor;
        }
        
        uint32_t average = sum / factors.size();
        uint16_t score = static_cast<uint16_t>((average * 100) % 10001);
        return std::min(score, MAX_SCORE_BPS);
    };
    
    uint16_t novelty = calculate_score(novelty_factors);
    uint16_t rarity = calculate_score(rarity_factors);
    uint16_t execution = calculate_score(execution_factors);
    
    return std::make_tuple(novelty, rarity, execution);
}

// ============================================================================
// MarketCapCalculator Implementation
// ============================================================================

uint64_t MarketCapCalculator::calculate_appraisal(uint64_t base_value,
                                                   uint16_t novelty_score,
                                                   uint16_t rarity_score,
                                                   uint16_t execution_difficulty) {
    // Formula: base * (1 + novelty/10000) * (1 + rarity/10000) * (1 + difficulty/10000)
    
    uint64_t novelty_multiplier = 10000 + static_cast<uint64_t>(novelty_score);
    uint64_t rarity_multiplier = 10000 + static_cast<uint64_t>(rarity_score);
    uint64_t difficulty_multiplier = 10000 + static_cast<uint64_t>(execution_difficulty);
    
    // Use 128-bit arithmetic to prevent overflow
    __uint128_t total = static_cast<__uint128_t>(base_value);
    total = total * novelty_multiplier;
    total = total * rarity_multiplier;
    total = total * difficulty_multiplier;
    total = total / 1000000000ULL; // Normalize by 10000^3
    
    return static_cast<uint64_t>(total);
}

// ============================================================================
// ProvenanceAttestationBuilder Implementation
// ============================================================================

ProvenanceAttestationBuilder::ProvenanceAttestationBuilder()
    : crypto_(std::make_unique<Crypto>()) {
}

ProvenanceAttestation ProvenanceAttestationBuilder::create_attestation(
    const std::array<uint8_t, 32>& token_mint,
    const TokenAppraisal& appraisal,
    uint64_t base_value,
    const std::array<uint8_t, 32>& authority,
    uint64_t attestation_id) {
    
    ProvenanceAttestation attestation;
    
    // Compute merkle root and scores
    attestation.merkle_root = appraisal.compute_merkle_root();
    auto [novelty, rarity, execution] = appraisal.calculate_scores();
    
    attestation.novelty_score = novelty;
    attestation.rarity_score = rarity;
    attestation.execution_difficulty = execution;
    
    // Calculate market cap appraisal
    attestation.market_cap_appraisal = MarketCapCalculator::calculate_appraisal(
        base_value, novelty, rarity, execution
    );
    
    // Set other fields
    attestation.token_mint = token_mint;
    attestation.authority = authority;
    attestation.attestation_id = attestation_id;
    attestation.verified = false;
    
    // Set timestamp (current time in seconds since epoch)
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()
    ).count();
    attestation.attestation_timestamp = timestamp;
    
    return attestation;
}

bool ProvenanceAttestationBuilder::verify_attestation(
    const ProvenanceAttestation& attestation,
    const MerkleProof& proof) {
    
    if (attestation.verified) {
        return false; // Already verified
    }
    
    // Verify merkle proof
    return proof.verify(attestation.merkle_root);
}

// ============================================================================
// MerkleTreeBuilder Implementation
// ============================================================================

MerkleTreeBuilder::MerkleTreeBuilder()
    : crypto_(std::make_unique<Crypto>()) {
}

void MerkleTreeBuilder::add_leaf(const std::vector<uint8_t>& data) {
    leaves_.push_back(MerkleNode::create_leaf(data));
}

std::array<uint8_t, HASH_SIZE> MerkleTreeBuilder::build_root() {
    if (leaves_.empty()) {
        std::array<uint8_t, HASH_SIZE> empty_hash;
        empty_hash.fill(0);
        return empty_hash;
    }
    
    if (leaves_.size() == 1) {
        return leaves_[0].hash;
    }
    
    build_tree();
    
    if (nodes_.empty()) {
        return leaves_[0].hash;
    }
    
    return nodes_.back().hash;
}

void MerkleTreeBuilder::build_tree() {
    nodes_.clear();
    
    std::vector<MerkleNode> current_level = leaves_;
    
    while (current_level.size() > 1) {
        std::vector<MerkleNode> next_level;
        
        for (size_t i = 0; i < current_level.size(); i += 2) {
            if (i + 1 < current_level.size()) {
                // Pair of nodes
                auto internal = MerkleNode::create_internal(
                    current_level[i].hash,
                    current_level[i + 1].hash
                );
                next_level.push_back(internal);
            } else {
                // Odd number of nodes, promote the last one
                next_level.push_back(current_level[i]);
            }
        }
        
        nodes_.insert(nodes_.end(), current_level.begin(), current_level.end());
        current_level = next_level;
    }
    
    if (!current_level.empty()) {
        nodes_.push_back(current_level[0]);
    }
}

MerkleProof MerkleTreeBuilder::generate_proof(size_t leaf_index) {
    if (leaf_index >= leaves_.size()) {
        throw std::out_of_range("Leaf index out of range");
    }
    
    if (leaves_.empty()) {
        return MerkleProof();
    }
    
    if (leaves_.size() == 1) {
        MerkleProof proof;
        proof.leaf_hash = leaves_[0].hash;
        proof.leaf_index = 0;
        return proof;
    }
    
    build_tree();
    return generate_proof_recursive(leaf_index, 0, leaves_.size());
}

MerkleProof MerkleTreeBuilder::generate_proof_recursive(
    size_t leaf_index, size_t /* node_index */, size_t /* tree_size */) {
    
    MerkleProof proof;
    proof.leaf_hash = leaves_[leaf_index].hash;
    proof.leaf_index = leaf_index;
    
    std::vector<MerkleNode> current_level = leaves_;
    size_t current_index = leaf_index;
    
    while (current_level.size() > 1) {
        size_t sibling_index = (current_index % 2 == 0) ? current_index + 1 : current_index - 1;
        
        if (sibling_index < current_level.size()) {
            proof.siblings.push_back(current_level[sibling_index].hash);
        }
        
        current_index /= 2;
        
        // Build next level
        std::vector<MerkleNode> next_level;
        for (size_t i = 0; i < current_level.size(); i += 2) {
            if (i + 1 < current_level.size()) {
                auto internal = MerkleNode::create_internal(
                    current_level[i].hash,
                    current_level[i + 1].hash
                );
                next_level.push_back(internal);
            } else {
                next_level.push_back(current_level[i]);
            }
        }
        
        current_level = next_level;
    }
    
    return proof;
}

} // namespace tokenomics
} // namespace membra