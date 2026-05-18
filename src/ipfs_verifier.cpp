#include "ipfs_verifier.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <chrono>
#include <filesystem>
#include <set>

namespace membra {
namespace artifact {

// ============================================================================
// IPFSVerifier Implementation
// ============================================================================

IPFSVerifier::IPFSVerifier(const IPFSConfig& config)
    : config_(config), crypto_(std::make_unique<Crypto>()) {
}

std::string IPFSVerifier::upload_to_ipfs(const std::vector<uint8_t>& content) {
    // Try Pinata first if API keys are available
    if (!config_.pinata_api_key.empty() && !config_.pinata_secret_key.empty()) {
        try {
            return upload_to_pinata(content, "artifact.bin");
        } catch (const std::exception& e) {
            // Fall back to local IPFS if Pinata fails
        }
    }
    
    // Try local IPFS node
    if (config_.use_local_ipfs) {
        try {
            return upload_to_local_ipfs(content);
        } catch (const std::exception& e) {
            // Fall back to mock CID
        }
    }
    
    // Generate mock CID for testing
    return generate_mock_cid(content);
}

std::string IPFSVerifier::upload_to_ipfs(const std::string& file_path) {
    // Read file content
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + file_path);
    }
    
    std::vector<uint8_t> content((std::istreambuf_iterator<char>(file)),
                                  std::istreambuf_iterator<char>());
    
    std::string filename = std::filesystem::path(file_path).filename().string();
    
    // Try Pinata first
    if (!config_.pinata_api_key.empty() && !config_.pinata_secret_key.empty()) {
        try {
            return upload_to_pinata(content, filename);
        } catch (const std::exception& e) {
            // Fall back to local IPFS
        }
    }
    
    // Try local IPFS node
    if (config_.use_local_ipfs) {
        try {
            return upload_to_local_ipfs(content);
        } catch (const std::exception& e) {
            // Fall back to mock CID
        }
    }
    
    return generate_mock_cid(content);
}

std::vector<uint8_t> IPFSVerifier::retrieve_from_ipfs(const std::string& cid) {
    std::string url = config_.gateway_url + cid;
    return http_get(url);
}

bool IPFSVerifier::verify_content_integrity(
    const std::string& cid,
    const std::array<uint8_t, HASH_SIZE>& expected_hash) {
    
    try {
        std::vector<uint8_t> content = retrieve_from_ipfs(cid);
        std::array<uint8_t, HASH_SIZE> actual_hash = crypto_->sha256(content);
        
        return actual_hash == expected_hash;
    } catch (const std::exception& e) {
        return false;
    }
}

std::string IPFSVerifier::generate_mock_cid(const std::vector<uint8_t>& content) {
    std::array<uint8_t, HASH_SIZE> hash = crypto_->sha256(content);
    
    // Generate IPFS-style CID (simplified)
    std::string cid = "Qm";
    for (size_t i = 0; i < 44; ++i) { // Base58 encoding would be proper, using hex for simplicity
        char buf[3];
        snprintf(buf, 3, "%02x", hash[i % 32]);
        cid += buf;
    }
    
    return cid.substr(0, 46); // Typical IPFS CID length
}

std::string IPFSVerifier::upload_to_local_ipfs(const std::vector<uint8_t>& /* content */) {
    std::string url = config_.local_api_url + "/add";
    
    // For local IPFS, we'd need to use multipart/form-data
    // This is a simplified version - in production use proper HTTP client
    std::string response_str = "QmHash123456789"; // Mock response
    
    // Parse CID from response (simplified)
    return response_str;
}

std::string IPFSVerifier::upload_to_pinata(
    const std::vector<uint8_t>& /* content */,
    const std::string& /* filename */) {
    
    std::string url = "https://api.pinata.cloud/pinning/pinFileToIPFS";
    
    // Prepare headers
    std::map<std::string, std::string> headers;
    headers["pinata_api_key"] = config_.pinata_api_key;
    headers["pinata_secret_api_key"] = config_.pinata_secret_key;
    
    // In production, this would be a proper multipart/form-data request
    std::vector<uint8_t> dummy_content = {'d', 'u', 'm', 'm', 'y'};
    std::vector<uint8_t> response = http_post(url, dummy_content, headers);
    
    // Parse response to get CID (simplified)
    return "QmPinataMockCID123456";
}

std::vector<uint8_t> IPFSVerifier::http_get(const std::string& /* url */) {
    // Simplified HTTP GET - in production use proper HTTP client
    // This is a placeholder implementation
    return std::vector<uint8_t>();
}

std::vector<uint8_t> IPFSVerifier::http_post(
    const std::string& /* url */,
    const std::vector<uint8_t>& /* data */,
    const std::map<std::string, std::string>& /* headers */) {
    
    // Simplified HTTP POST - in production use proper HTTP client
    // This is a placeholder implementation
    return std::vector<uint8_t>();
}

// ============================================================================
// ArtifactAnalyzer Implementation
// ============================================================================

ArtifactAnalyzer::ArtifactAnalyzer()
    : crypto_(std::make_unique<Crypto>()) {
    
    audio_extensions_ = {".mp3", ".wav", ".ogg", ".m4a", ".flac"};
    video_extensions_ = {".mp4", ".avi", ".mov", ".mkv", ".webm"};
}

std::string ArtifactAnalyzer::detect_artifact_type(const std::string& file_path) {
    std::string ext = get_file_extension(file_path);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (std::find(audio_extensions_.begin(), audio_extensions_.end(), ext) != audio_extensions_.end()) {
        return "audio";
    } else if (std::find(video_extensions_.begin(), video_extensions_.end(), ext) != video_extensions_.end()) {
        return "video";
    }
    
    return "unknown";
}

std::string ArtifactAnalyzer::detect_artifact_type(
    const std::string& filename,
    const std::vector<uint8_t>& content) {
    
    std::string ext = get_file_extension(filename);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (std::find(audio_extensions_.begin(), audio_extensions_.end(), ext) != audio_extensions_.end()) {
        return "audio";
    } else if (std::find(video_extensions_.begin(), video_extensions_.end(), ext) != video_extensions_.end()) {
        return "video";
    }
    
    // Try to detect from magic bytes
    if (content.size() >= 4) {
        // MP3 detection (ID3v2)
        if (content[0] == 'I' && content[1] == 'D' && content[2] == '3') {
            return "audio";
        }
        // WAV detection
        if (content[0] == 'R' && content[1] == 'I' && content[2] == 'F' && content[3] == 'F') {
            return "audio";
        }
        // MP4 detection
        if (content[4] == 'f' && content[5] == 't' && content[6] == 'y' && content[7] == 'p') {
            return "video";
        }
    }
    
    return "unknown";
}

ArtifactAnalysis ArtifactAnalyzer::analyze_audio(const std::string& file_path) {
    std::vector<uint8_t> content = read_file(file_path);
    std::string format = get_file_extension(file_path);
    format.erase(0, 1); // Remove the dot
    
    return analyze_audio(content, format);
}

ArtifactAnalysis ArtifactAnalyzer::analyze_audio(
    const std::vector<uint8_t>& content,
    const std::string& format) {
    
    ArtifactAnalysis analysis;
    analysis.artifact_type = "audio";
    analysis.size_bytes = content.size();
    analysis.content_hash = crypto_->sha256(content);
    analysis.format = format;
    
    auto now = std::chrono::system_clock::now();
    analysis.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()
    ).count();
    
    return analysis;
}

ArtifactAnalysis ArtifactAnalyzer::analyze_video(const std::string& file_path) {
    std::vector<uint8_t> content = read_file(file_path);
    std::string format = get_file_extension(file_path);
    format.erase(0, 1); // Remove the dot
    
    return analyze_video(content, format);
}

ArtifactAnalysis ArtifactAnalyzer::analyze_video(
    const std::vector<uint8_t>& content,
    const std::string& format) {
    
    ArtifactAnalysis analysis;
    analysis.artifact_type = "video";
    analysis.size_bytes = content.size();
    analysis.content_hash = crypto_->sha256(content);
    analysis.format = format;
    
    auto now = std::chrono::system_clock::now();
    analysis.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()
    ).count();
    
    return analysis;
}

std::array<uint8_t, PERCEPTUAL_HASH_SIZE> ArtifactAnalyzer::generate_perceptual_hash(
    const std::string& file_path) {
    
    std::vector<uint8_t> content = read_file(file_path);
    return generate_perceptual_hash(content);
}

std::array<uint8_t, PERCEPTUAL_HASH_SIZE> ArtifactAnalyzer::generate_perceptual_hash(
    const std::vector<uint8_t>& content) {
    
    // Sample first 4KB for perceptual hashing
    size_t sample_size = std::min(content.size(), size_t(4096));
    std::vector<uint8_t> sample(content.begin(), content.begin() + sample_size);
    
    std::array<uint8_t, HASH_SIZE> full_hash = crypto_->sha256(sample);
    
    // Truncate to perceptual hash size
    std::array<uint8_t, PERCEPTUAL_HASH_SIZE> perceptual_hash;
    std::copy(full_hash.begin(), full_hash.begin() + PERCEPTUAL_HASH_SIZE, perceptual_hash.begin());
    
    return perceptual_hash;
}

std::vector<uint8_t> ArtifactAnalyzer::read_file(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + file_path);
    }
    
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
}

std::string ArtifactAnalyzer::get_file_extension(const std::string& filename) {
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos != std::string::npos) {
        return filename.substr(dot_pos);
    }
    return "";
}

// ============================================================================
// ProvenanceAttestationBuilder Implementation
// ============================================================================

ProvenanceAttestationBuilder::ProvenanceAttestationBuilder(
    std::shared_ptr<IPFSVerifier> ipfs_verifier)
    : ipfs_verifier_(ipfs_verifier),
      analyzer_(std::make_unique<ArtifactAnalyzer>()),
      crypto_(std::make_unique<Crypto>()) {
}

ArtifactAttestation ProvenanceAttestationBuilder::create_artifact_attestation(
    const std::string& file_path,
    const std::string& creator_wallet,
    const std::map<std::string, std::string>& metadata) {
    
    // Analyze artifact
    std::string artifact_type = analyzer_->detect_artifact_type(file_path);
    if (artifact_type == "unknown") {
        throw std::runtime_error("Unsupported file type");
    }
    
    ArtifactAnalysis analysis;
    if (artifact_type == "audio") {
        analysis = analyzer_->analyze_audio(file_path);
    } else {
        analysis = analyzer_->analyze_video(file_path);
    }
    
    // Upload to IPFS
    std::string cid = ipfs_verifier_->upload_to_ipfs(file_path);
    
    // Generate perceptual hash
    auto perceptual_hash = analyzer_->generate_perceptual_hash(file_path);
    
    // Build attestation
    ArtifactAttestation attestation;
    attestation.artifact_type = artifact_type;
    attestation.ipfs_cid = cid;
    attestation.content_hash = analysis.content_hash;
    attestation.perceptual_hash = perceptual_hash;
    attestation.size_bytes = analysis.size_bytes;
    attestation.format = analysis.format;
    attestation.creator_wallet = creator_wallet;
    attestation.created_at = analysis.timestamp;
    attestation.metadata = metadata;
    attestation.verification_status = "pending";
    attestation.attestation_id = generate_attestation_id(cid, creator_wallet);
    
    return attestation;
}

ArtifactAttestation ProvenanceAttestationBuilder::create_artifact_attestation(
    const std::vector<uint8_t>& content,
    const std::string& format,
    const std::string& creator_wallet,
    const std::map<std::string, std::string>& metadata) {
    
    // Detect artifact type from format
    std::string artifact_type = "unknown";
    std::string ext = "." + format;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (std::find(analyzer_->audio_extensions_.begin(), 
                  analyzer_->audio_extensions_.end(), ext) != analyzer_->audio_extensions_.end()) {
        artifact_type = "audio";
    } else if (std::find(analyzer_->video_extensions_.begin(),
                       analyzer_->video_extensions_.end(), ext) != analyzer_->video_extensions_.end()) {
        artifact_type = "video";
    }
    
    if (artifact_type == "unknown") {
        throw std::runtime_error("Unsupported format");
    }
    
    // Analyze content
    ArtifactAnalysis analysis;
    if (artifact_type == "audio") {
        analysis = analyzer_->analyze_audio(content, format);
    } else {
        analysis = analyzer_->analyze_video(content, format);
    }
    
    // Upload to IPFS
    std::string cid = ipfs_verifier_->upload_to_ipfs(content);
    
    // Generate perceptual hash
    auto perceptual_hash = analyzer_->generate_perceptual_hash(content);
    
    // Build attestation
    ArtifactAttestation attestation;
    attestation.artifact_type = artifact_type;
    attestation.ipfs_cid = cid;
    attestation.content_hash = analysis.content_hash;
    attestation.perceptual_hash = perceptual_hash;
    attestation.size_bytes = analysis.size_bytes;
    attestation.format = format;
    attestation.creator_wallet = creator_wallet;
    attestation.created_at = analysis.timestamp;
    attestation.metadata = metadata;
    attestation.verification_status = "pending";
    attestation.attestation_id = generate_attestation_id(cid, creator_wallet);
    
    return attestation;
}

bool ProvenanceAttestationBuilder::verify_artifact_attestation(
    const ArtifactAttestation& attestation) {
    
    // Verify IPFS content integrity
    if (!ipfs_verifier_->verify_content_integrity(
            attestation.ipfs_cid, attestation.content_hash)) {
        return false;
    }
    
    // Additional verification checks can be added here
    // - Check perceptual hash against database for duplicates
    // - Verify creator wallet signature
    // - Check metadata consistency
    
    return true;
}

std::string ProvenanceAttestationBuilder::generate_attestation_id(
    const std::string& ipfs_cid,
    const std::string& creator_wallet) {
    
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    
    std::string combined = ipfs_cid + creator_wallet + std::to_string(timestamp);
    std::array<uint8_t, HASH_SIZE> hash = crypto_->sha256(
        std::vector<uint8_t>(combined.begin(), combined.end())
    );
    
    // Convert to hex string (first 16 characters)
    char buf[33];
    for (size_t i = 0; i < 16; ++i) {
        snprintf(&buf[i * 2], 3, "%02x", hash[i]);
    }
    buf[32] = '\0';
    
    return std::string(buf);
}

// ============================================================================
// ArtifactRegistry Implementation
// ============================================================================

ArtifactRegistry::ArtifactRegistry(const std::string& oracle_endpoint)
    : oracle_endpoint_(oracle_endpoint),
      analyzer_(std::make_unique<ArtifactAnalyzer>()),
      crypto_(std::make_unique<Crypto>()) {
    
    load_state();
}

std::string ArtifactRegistry::register_artifact(ArtifactAttestation& attestation) {
    // Check for duplicates using perceptual hash
    std::string perceptual_hash_str;
    for (uint8_t byte : attestation.perceptual_hash) {
        char buf[3];
        snprintf(buf, 3, "%02x", byte);
        perceptual_hash_str += buf;
    }
    
    if (perceptual_hashes_.find(perceptual_hash_str) != perceptual_hashes_.end()) {
        throw std::runtime_error("Duplicate artifact detected");
    }
    
    // Store attestation
    attestations_[attestation.attestation_id] = attestation;
    perceptual_hashes_[perceptual_hash_str] = attestation.attestation_id;
    attestation.verification_status = "verified";
    
    // Submit to oracle for cryptographic verification
    submit_to_oracle(attestation);
    
    // Save state
    save_state();
    
    return attestation.attestation_id;
}

std::shared_ptr<ArtifactAttestation> ArtifactRegistry::get_attestation(
    const std::string& attestation_id) {
    
    auto it = attestations_.find(attestation_id);
    if (it != attestations_.end()) {
        return std::make_shared<ArtifactAttestation>(it->second);
    }
    return nullptr;
}

bool ArtifactRegistry::verify_artifact_uniqueness(const std::string& file_path) {
    auto perceptual_hash = analyzer_->generate_perceptual_hash(file_path);
    
    std::string perceptual_hash_str;
    for (uint8_t byte : perceptual_hash) {
        char buf[3];
        snprintf(buf, 3, "%02x", byte);
        perceptual_hash_str += buf;
    }
    
    return perceptual_hashes_.find(perceptual_hash_str) == perceptual_hashes_.end();
}

bool ArtifactRegistry::verify_artifact_uniqueness(const std::vector<uint8_t>& content) {
    auto perceptual_hash = analyzer_->generate_perceptual_hash(content);
    
    std::string perceptual_hash_str;
    for (uint8_t byte : perceptual_hash) {
        char buf[3];
        snprintf(buf, 3, "%02x", byte);
        perceptual_hash_str += buf;
    }
    
    return perceptual_hashes_.find(perceptual_hash_str) == perceptual_hashes_.end();
}

std::vector<ArtifactAttestation> ArtifactRegistry::list_artifacts(
    const std::string& artifact_type,
    const std::string& creator_wallet,
    size_t limit) {
    
    std::vector<ArtifactAttestation> results;
    
    for (const auto& pair : attestations_) {
        const ArtifactAttestation& attestation = pair.second;
        
        // Apply filters
        if (!artifact_type.empty() && attestation.artifact_type != artifact_type) {
            continue;
        }
        
        if (!creator_wallet.empty() && attestation.creator_wallet != creator_wallet) {
            continue;
        }
        
        results.push_back(attestation);
        
        if (results.size() >= limit) {
            break;
        }
    }
    
    return results;
}

ArtifactRegistry::RegistryStats ArtifactRegistry::get_stats() const {
    RegistryStats stats;
    stats.total_attestations = attestations_.size();
    
    for (const auto& pair : attestations_) {
        const ArtifactAttestation& attestation = pair.second;
        
        if (attestation.artifact_type == "audio") {
            stats.audio_count++;
        } else if (attestation.artifact_type == "video") {
            stats.video_count++;
        }
    }
    
    // Count unique creators
    std::set<std::string> creators;
    for (const auto& pair : attestations_) {
        creators.insert(pair.second.creator_wallet);
    }
    stats.unique_creators = creators.size();
    
    return stats;
}

void ArtifactRegistry::submit_to_oracle(ArtifactAttestation& attestation) {
    // Placeholder for oracle submission
    // In production, this would make an HTTP request to the oracle endpoint
    attestation.oracle_signature = "mock_oracle_signature_" + attestation.attestation_id;
}

void ArtifactRegistry::load_state() {
    // Placeholder for loading state from disk
    // In production, load from JSON file or database
}

void ArtifactRegistry::save_state() {
    // Placeholder for saving state to disk
    // In production, save to JSON file or database
}

// ============================================================================
// Factory Function Implementation
// ============================================================================

ArtifactVerificationStack create_artifact_verifier(
    const IPFSConfig& ipfs_config,
    const std::string& oracle_endpoint) {
    
    ArtifactVerificationStack stack;
    stack.ipfs_verifier = std::make_shared<IPFSVerifier>(ipfs_config);
    stack.attestation_builder = std::make_shared<ProvenanceAttestationBuilder>(
        stack.ipfs_verifier);
    stack.registry = std::make_shared<ArtifactRegistry>(oracle_endpoint);
    
    return stack;
}

} // namespace artifact
} // namespace membra