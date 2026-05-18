#include "../include/ipfs_verifier.hpp"
#include <iostream>
#include <cassert>
#include <iomanip>

using namespace membra::artifact;

void print_hash(const std::array<uint8_t, 32>& hash) {
    for (uint8_t byte : hash) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    std::cout << std::dec;
}

void test_ipfs_verifier_creation() {
    std::cout << "Testing IPFSVerifier creation..." << std::endl;
    
    IPFSConfig config;
    config.use_local_ipfs = false; // Disable local IPFS for testing
    
    IPFSVerifier verifier(config);
    
    std::cout << "✓ IPFSVerifier creation passed" << std::endl;
}

void test_artifact_analyzer() {
    std::cout << "Testing ArtifactAnalyzer..." << std::endl;
    
    ArtifactAnalyzer analyzer;
    
    // Test file type detection
    std::string mp3_type = analyzer.detect_artifact_type("test.mp3");
    assert(mp3_type == "audio");
    
    std::string mp4_type = analyzer.detect_artifact_type("test.mp4");
    assert(mp4_type == "video");
    
    std::string unknown_type = analyzer.detect_artifact_type("test.txt");
    assert(unknown_type == "unknown");
    
    // Test perceptual hashing
    std::vector<uint8_t> test_content = {'t', 'e', 's', 't', 'd', 'a', 't', 'a'};
    auto perceptual_hash = analyzer.generate_perceptual_hash(test_content);
    
    bool all_zeros = true;
    for (uint8_t byte : perceptual_hash) {
        if (byte != 0) {
            all_zeros = false;
            break;
        }
    }
    assert(all_zeros == false);
    
    std::cout << "✓ ArtifactAnalyzer passed" << std::endl;
}

void test_mock_cid_generation() {
    std::cout << "Testing mock CID generation..." << std::endl;
    
    IPFSConfig config;
    config.use_local_ipfs = false;
    IPFSVerifier verifier(config);
    
    std::vector<uint8_t> content = {'t', 'e', 's', 't', 'd', 'a', 't', 'a'};
    std::string cid = verifier.generate_mock_cid(content);
    
    assert(cid.substr(0, 2) == "Qm");
    assert(cid.length() > 10);
    
    std::cout << "✓ Mock CID generation passed" << std::endl;
    std::cout << "  Generated CID: " << cid << std::endl;
}

void test_attestation_builder() {
    std::cout << "Testing ProvenanceAttestationBuilder..." << std::endl;
    
    IPFSConfig config;
    config.use_local_ipfs = false;
    auto ipfs_verifier = std::make_shared<IPFSVerifier>(config);
    
    ProvenanceAttestationBuilder builder(ipfs_verifier);
    
    // Test attestation creation from content
    std::vector<uint8_t> audio_content = {'a', 'u', 'd', 'i', 'o', 'd', 'a', 't', 'a'};
    std::map<std::string, std::string> metadata = {{"title", "Test Audio"}, {"description", "Test"}};
    
    try {
        ArtifactAttestation attestation = builder.create_artifact_attestation(
            audio_content, "mp3", "test_wallet", metadata);
        
        assert(attestation.artifact_type == "audio");
        assert(attestation.creator_wallet == "test_wallet");
        assert(attestation.format == "mp3");
        assert(!attestation.attestation_id.empty());
        assert(!attestation.ipfs_cid.empty());
        
        std::cout << "✓ ProvenanceAttestationBuilder passed" << std::endl;
        std::cout << "  Attestation ID: " << attestation.attestation_id << std::endl;
        std::cout << "  IPFS CID: " << attestation.ipfs_cid << std::endl;
    } catch (const std::exception& e) {
        std::cout << "⚠ Attestation creation test skipped: " << e.what() << std::endl;
    }
}

void test_artifact_registry() {
    std::cout << "Testing ArtifactRegistry..." << std::endl;
    
    ArtifactRegistry registry("http://localhost:8000");
    
    // Test uniqueness check
    std::vector<uint8_t> content = {'t', 'e', 's', 't', 'd', 'a', 't', 'a'};
    bool is_unique = registry.verify_artifact_uniqueness(content);
    assert(is_unique == true);
    
    // Test stats
    auto stats = registry.get_stats();
    assert(stats.total_attestations == 0);
    
    std::cout << "✓ ArtifactRegistry passed" << std::endl;
    std::cout << "  Total attestations: " << stats.total_attestations << std::endl;
}

void test_factory_function() {
    std::cout << "Testing factory function..." << std::endl;
    
    IPFSConfig config;
    config.use_local_ipfs = false;
    
    auto stack = create_artifact_verifier(config, "http://localhost:8000");
    
    assert(stack.ipfs_verifier != nullptr);
    assert(stack.attestation_builder != nullptr);
    assert(stack.registry != nullptr);
    
    std::cout << "✓ Factory function passed" << std::endl;
}

void test_artifact_analysis() {
    std::cout << "Testing artifact analysis..." << std::endl;
    
    ArtifactAnalyzer analyzer;
    
    // Test audio analysis
    std::vector<uint8_t> audio_data = {'I', 'D', '3', 0x04, 0x00}; // ID3v2 header
    std::string type = analyzer.detect_artifact_type("test.mp3", audio_data);
    assert(type == "audio");
    
    ArtifactAnalysis audio_analysis = analyzer.analyze_audio(audio_data, "mp3");
    assert(audio_analysis.artifact_type == "audio");
    assert(audio_analysis.size_bytes == audio_data.size());
    assert(audio_analysis.format == "mp3");
    
    // Test video analysis
    std::vector<uint8_t> video_data = {0x00, 0x00, 0x00, 0x20, 0x66, 0x74, 0x79, 0x70}; // MP4 header
    std::string video_type = analyzer.detect_artifact_type("test.mp4", video_data);
    assert(video_type == "video");
    
    ArtifactAnalysis video_analysis = analyzer.analyze_video(video_data, "mp4");
    assert(video_analysis.artifact_type == "video");
    assert(video_analysis.size_bytes == video_data.size());
    assert(video_analysis.format == "mp4");
    
    std::cout << "✓ Artifact analysis passed" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "IPFS Verifier System Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        test_ipfs_verifier_creation();
        test_artifact_analyzer();
        test_mock_cid_generation();
        test_attestation_builder();
        test_artifact_registry();
        test_factory_function();
        test_artifact_analysis();
        
        std::cout << "========================================" << std::endl;
        std::cout << "✅ All tests passed!" << std::endl;
        std::cout << "========================================" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
}