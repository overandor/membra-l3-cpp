#include "../include/merkle_provenance.hpp"
#include <iostream>
#include <cassert>
#include <iomanip>

using namespace membra::tokenomics;

void print_hash(const std::array<uint8_t, 32>& hash) {
    for (uint8_t byte : hash) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    std::cout << std::dec;
}

void test_merkle_node_creation() {
    std::cout << "Testing MerkleNode creation..." << std::endl;
    
    std::vector<uint8_t> data = {'t', 'e', 's', 't'};
    MerkleNode leaf = MerkleNode::create_leaf(data);
    
    assert(leaf.has_left == false);
    assert(leaf.has_right == false);
    
    std::cout << "✓ Leaf node creation passed" << std::endl;
}

void test_merkle_proof_verification() {
    std::cout << "Testing MerkleProof verification..." << std::endl;
    
    MerkleTreeBuilder builder;
    
    // Add some leaves
    builder.add_leaf({'d', 'a', 't', 'a', '1'});
    builder.add_leaf({'d', 'a', 't', 'a', '2'});
    builder.add_leaf({'d', 'a', 't', 'a', '3'});
    builder.add_leaf({'d', 'a', 't', 'a', '4'});
    
    std::array<uint8_t, 32> root = builder.build_root();
    
    // Generate proof for leaf 1
    MerkleProof proof = builder.generate_proof(1);
    
    // Verify proof
    bool verified = proof.verify(root);
    assert(verified == true);
    
    std::cout << "✓ Merkle proof verification passed" << std::endl;
}

void test_token_appraisal_scoring() {
    std::cout << "Testing TokenAppraisal scoring..." << std::endl;
    
    TokenAppraisal appraisal;
    appraisal.novelty_factors = {80, 90, 85};
    appraisal.rarity_factors = {70, 75, 80};
    appraisal.execution_factors = {60, 65, 70};
    
    auto [novelty, rarity, execution] = appraisal.calculate_scores();
    
    assert(novelty > 8000 && novelty <= 10000);
    assert(rarity > 7000 && rarity <= 10000);
    assert(execution > 6000 && execution <= 10000);
    
    std::cout << "✓ Token appraisal scoring passed" << std::endl;
    std::cout << "  Novelty: " << novelty << " bps" << std::endl;
    std::cout << "  Rarity: " << rarity << " bps" << std::endl;
    std::cout << "  Execution: " << execution << " bps" << std::endl;
}

void test_market_cap_calculation() {
    std::cout << "Testing MarketCapCalculator..." << std::endl;
    
    uint64_t base_value = 1000000; // 1 SOL in lamports
    uint64_t appraisal = MarketCapCalculator::calculate_appraisal(
        base_value, 5000, 3000, 2000
    );
    
    // Should be higher than base due to positive scores
    assert(appraisal > base_value);
    
    std::cout << "✓ Market cap calculation passed" << std::endl;
    std::cout << "  Base value: " << base_value << " lamports" << std::endl;
    std::cout << "  Appraisal: " << appraisal << " lamports" << std::endl;
}

void test_provenance_attestation() {
    std::cout << "Testing ProvenanceAttestation..." << std::endl;
    
    ProvenanceAttestationBuilder builder;
    
    std::array<uint8_t, 32> token_mint;
    token_mint.fill(0x42);
    
    std::array<uint8_t, 32> authority;
    authority.fill(0x13);
    
    TokenAppraisal appraisal;
    appraisal.novelty_factors = {85, 90, 88};
    appraisal.rarity_factors = {75, 80, 78};
    appraisal.execution_factors = {65, 70, 68};
    
    ProvenanceAttestation attestation = builder.create_attestation(
        token_mint, appraisal, 1000000, authority, 12345
    );
    
    assert(attestation.attestation_id == 12345);
    assert(attestation.verified == false);
    assert(attestation.validate_scores() == true);
    assert(attestation.market_cap_appraisal > 0);
    
    std::cout << "✓ Provenance attestation creation passed" << std::endl;
    std::cout << "  Attestation ID: " << attestation.attestation_id << std::endl;
    std::cout << "  Market Cap: " << attestation.market_cap_appraisal << " lamports" << std::endl;
}

void test_merkle_tree_builder() {
    std::cout << "Testing MerkleTreeBuilder..." << std::endl;
    
    MerkleTreeBuilder builder;
    
    // Test empty tree
    std::array<uint8_t, 32> empty_root = builder.build_root();
    bool all_zeros = true;
    for (uint8_t byte : empty_root) {
        if (byte != 0) {
            all_zeros = false;
            break;
        }
    }
    assert(all_zeros == true);
    
    // Test single leaf
    builder.add_leaf({'s', 'i', 'n', 'g', 'l', 'e'});
    std::array<uint8_t, 32> single_root = builder.build_root();
    all_zeros = true;
    for (uint8_t byte : single_root) {
        if (byte != 0) {
            all_zeros = false;
            break;
        }
    }
    assert(all_zeros == false);
    
    // Test multiple leaves
    MerkleTreeBuilder multi_builder;
    for (int i = 0; i < 8; i++) {
        std::vector<uint8_t> data = {'l', 'e', 'a', 'f', static_cast<uint8_t>(i)};
        multi_builder.add_leaf(data);
    }
    
    std::array<uint8_t, 32> multi_root = multi_builder.build_root();
    assert(multi_builder.leaf_count() == 8);
    
    std::cout << "✓ MerkleTreeBuilder passed" << std::endl;
    std::cout << "  Multi-root: ";
    print_hash(multi_root);
    std::cout << std::endl;
}

void test_attestation_verification() {
    std::cout << "Testing attestation verification..." << std::endl;
    
    ProvenanceAttestationBuilder builder;
    MerkleTreeBuilder tree_builder;
    
    // Create appraisal and add to merkle tree
    TokenAppraisal appraisal;
    appraisal.novelty_factors = {90, 85, 88};
    appraisal.rarity_factors = {80, 75, 78};
    appraisal.execution_factors = {70, 65, 68};
    
    std::vector<uint8_t> appraisal_data = {1, 2, 3, 4, 5};
    tree_builder.add_leaf(appraisal_data);
    
    std::array<uint8_t, 32> root = tree_builder.build_root();
    MerkleProof proof = tree_builder.generate_proof(0);
    
    // Create attestation
    std::array<uint8_t, 32> token_mint;
    token_mint.fill(0x42);
    
    std::array<uint8_t, 32> authority;
    authority.fill(0x13);
    
    ProvenanceAttestation attestation = builder.create_attestation(
        token_mint, appraisal, 1000000, authority, 54321
    );
    
    attestation.merkle_root = root;
    
    // Verify attestation
    bool verified = builder.verify_attestation(attestation, proof);
    assert(verified == true);
    
    // Check that attestation is now marked as verified
    assert(attestation.verified == false); // Builder doesn't modify attestation
    
    std::cout << "✓ Attestation verification passed" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Merkle Provenance System Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        test_merkle_node_creation();
        test_merkle_proof_verification();
        test_token_appraisal_scoring();
        test_market_cap_calculation();
        test_provenance_attestation();
        test_merkle_tree_builder();
        test_attestation_verification();
        
        std::cout << "========================================" << std::endl;
        std::cout << "✅ All tests passed!" << std::endl;
        std::cout << "========================================" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
}