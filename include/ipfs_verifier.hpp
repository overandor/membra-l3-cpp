#ifndef IPFS_VERIFIER_HPP
#define IPFS_VERIFIER_HPP

#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <memory>
#include <map>
#include "crypto.hpp"

namespace membra {
namespace artifact {

// Constants
constexpr size_t HASH_SIZE = 32;
constexpr size_t ATTESTATION_ID_SIZE = 16;
constexpr size_t PERCEPTUAL_HASH_SIZE = 16;

/**
 * IPFS configuration and connection settings
 */
struct IPFSConfig {
    std::string gateway_url = "https://ipfs.io/ipfs/";
    std::string local_api_url = "http://localhost:5001/api/v0";
    std::string pinata_api_key;
    std::string pinata_secret_key;
    bool use_local_ipfs = true;
    int timeout_seconds = 30;
    
    IPFSConfig() = default;
};

/**
 * Artifact analysis results
 */
struct ArtifactAnalysis {
    std::string artifact_type; // "audio" or "video"
    uint64_t size_bytes;
    std::array<uint8_t, HASH_SIZE> content_hash;
    std::string format;
    int64_t timestamp;
    
    ArtifactAnalysis() : size_bytes(0), timestamp(0) {
        content_hash.fill(0);
    }
};

/**
 * Provenance attestation for artifacts
 */
struct ArtifactAttestation {
    std::string attestation_id;
    std::string artifact_type;
    std::string ipfs_cid;
    std::array<uint8_t, HASH_SIZE> content_hash;
    std::array<uint8_t, PERCEPTUAL_HASH_SIZE> perceptual_hash;
    uint64_t size_bytes;
    std::string format;
    std::string creator_wallet;
    int64_t created_at;
    std::map<std::string, std::string> metadata;
    std::string verification_status;
    std::string oracle_signature;
    
    ArtifactAttestation() : size_bytes(0), created_at(0) {
        content_hash.fill(0);
        perceptual_hash.fill(0);
    }
};

/**
 * IPFS verifier for content addressing and retrieval
 */
class IPFSVerifier {
public:
    explicit IPFSVerifier(const IPFSConfig& config = IPFSConfig());
    ~IPFSVerifier() = default;
    
    // Upload content to IPFS
    std::string upload_to_ipfs(const std::vector<uint8_t>& content);
    std::string upload_to_ipfs(const std::string& file_path);
    
    // Retrieve content from IPFS
    std::vector<uint8_t> retrieve_from_ipfs(const std::string& cid);
    
    // Verify content integrity
    bool verify_content_integrity(const std::string& cid, 
                                  const std::array<uint8_t, HASH_SIZE>& expected_hash);
    
    // Generate mock CID for testing when IPFS is unavailable
    std::string generate_mock_cid(const std::vector<uint8_t>& content);
    
private:
    IPFSConfig config_;
    std::unique_ptr<Crypto> crypto_;
    
    // Upload to local IPFS node
    std::string upload_to_local_ipfs(const std::vector<uint8_t>& content);
    
    // Upload to Pinata service
    std::string upload_to_pinata(const std::vector<uint8_t>& content, 
                                 const std::string& filename);
    
    // HTTP request helper
    std::vector<uint8_t> http_get(const std::string& url);
    std::vector<uint8_t> http_post(const std::string& url, 
                                    const std::vector<uint8_t>& data,
                                    const std::map<std::string, std::string>& headers);
};

/**
 * Artifact analyzer for audio/video files
 */
class ArtifactAnalyzer {
public:
    ArtifactAnalyzer();
    ~ArtifactAnalyzer() = default;
    
    // Detect artifact type
    std::string detect_artifact_type(const std::string& file_path);
    std::string detect_artifact_type(const std::string& filename, 
                                     const std::vector<uint8_t>& content);
    
    // Analyze audio file
    ArtifactAnalysis analyze_audio(const std::string& file_path);
    ArtifactAnalysis analyze_audio(const std::vector<uint8_t>& content, 
                                   const std::string& format);
    
    // Analyze video file
    ArtifactAnalysis analyze_video(const std::string& file_path);
    ArtifactAnalysis analyze_video(const std::vector<uint8_t>& content, 
                                   const std::string& format);
    
    // Generate perceptual hash for duplicate detection
    std::array<uint8_t, PERCEPTUAL_HASH_SIZE> generate_perceptual_hash(
        const std::string& file_path);
    std::array<uint8_t, PERCEPTUAL_HASH_SIZE> generate_perceptual_hash(
        const std::vector<uint8_t>& content);
    
private:
    std::unique_ptr<Crypto> crypto_;
    
    // Read file content
    std::vector<uint8_t> read_file(const std::string& file_path);
    
    // Get file extension
    std::string get_file_extension(const std::string& filename);
    
public:
    std::vector<std::string> audio_extensions_;
    std::vector<std::string> video_extensions_;
};

/**
 * Provenance attestation builder
 */
class ProvenanceAttestationBuilder {
public:
    explicit ProvenanceAttestationBuilder(std::shared_ptr<IPFSVerifier> ipfs_verifier);
    ~ProvenanceAttestationBuilder() = default;
    
    // Create full attestation for an artifact
    ArtifactAttestation create_artifact_attestation(
        const std::string& file_path,
        const std::string& creator_wallet,
        const std::map<std::string, std::string>& metadata = {});
    
    ArtifactAttestation create_artifact_attestation(
        const std::vector<uint8_t>& content,
        const std::string& format,
        const std::string& creator_wallet,
        const std::map<std::string, std::string>& metadata = {});
    
    // Verify attestation validity
    bool verify_artifact_attestation(const ArtifactAttestation& attestation);
    
private:
    std::shared_ptr<IPFSVerifier> ipfs_verifier_;
    std::unique_ptr<ArtifactAnalyzer> analyzer_;
    std::unique_ptr<Crypto> crypto_;
    
    // Generate attestation ID
    std::string generate_attestation_id(const std::string& ipfs_cid,
                                        const std::string& creator_wallet);
};

/**
 * Artifact registry for managing attestations
 */
class ArtifactRegistry {
public:
    explicit ArtifactRegistry(const std::string& oracle_endpoint = "http://localhost:8000");
    ~ArtifactRegistry() = default;
    
    // Register artifact attestation
    std::string register_artifact(ArtifactAttestation& attestation);
    
    // Retrieve attestation
    std::shared_ptr<ArtifactAttestation> get_attestation(const std::string& attestation_id);
    
    // Verify artifact uniqueness
    bool verify_artifact_uniqueness(const std::string& file_path);
    bool verify_artifact_uniqueness(const std::vector<uint8_t>& content);
    
    // List attestations with filters
    std::vector<ArtifactAttestation> list_artifacts(
        const std::string& artifact_type = "",
        const std::string& creator_wallet = "",
        size_t limit = 50);
    
    // Get registry statistics
    struct RegistryStats {
        size_t total_attestations;
        size_t audio_count;
        size_t video_count;
        size_t unique_creators;
    };
    RegistryStats get_stats() const;
    
private:
    std::string oracle_endpoint_;
    std::map<std::string, ArtifactAttestation> attestations_;
    std::map<std::string, std::string> perceptual_hashes_; // perceptual_hash -> attestation_id
    std::unique_ptr<ArtifactAnalyzer> analyzer_;
    std::unique_ptr<Crypto> crypto_;
    
    // Submit to oracle for verification
    void submit_to_oracle(ArtifactAttestation& attestation);
    
    // Load/save registry state
    void load_state();
    void save_state();
};

/**
 * Factory function for easy instantiation
 */
struct ArtifactVerificationStack {
    std::shared_ptr<IPFSVerifier> ipfs_verifier;
    std::shared_ptr<ProvenanceAttestationBuilder> attestation_builder;
    std::shared_ptr<ArtifactRegistry> registry;
};

ArtifactVerificationStack create_artifact_verifier(
    const IPFSConfig& ipfs_config = IPFSConfig(),
    const std::string& oracle_endpoint = "http://localhost:8000");

} // namespace artifact
} // namespace membra

#endif // IPFS_VERIFIER_HPP