cmake_minimum_required(VERSION 3.15)
project(membra_l3_cpp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3 -march=native")
elseif(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /O2")
endif()

# Find OpenSSL
find_package(OpenSSL REQUIRED)

# Include directories
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/main.cpp
    src/merkle_provenance.cpp
    src/crypto.cpp
    src/ipfs_verifier.cpp
    src/llm_mempool_governor.cpp
    src/zk_compute.cpp
)

# Create executable
add_executable(membra_l3 ${SOURCES})

# Link libraries
target_link_libraries(membra_l3 ${OPENSSL_LIBRARIES})

# Threads
find_package(Threads REQUIRED)
target_link_libraries(membra_l3 Threads::Threads)

# Enable testing
enable_testing()

# Test sources - separate test executables
add_executable(test_compute_staking tests/test_compute_staking.cpp)
target_link_libraries(test_compute_staking ${OPENSSL_LIBRARIES} Threads::Threads)
target_include_directories(test_compute_staking PRIVATE ${PROJECT_SOURCE_DIR}/include)
add_test(NAME test_compute_staking COMMAND test_compute_staking)

add_executable(test_reward_vault tests/test_reward_vault.cpp)
target_link_libraries(test_reward_vault ${OPENSSL_LIBRARIES} Threads::Threads)
target_include_directories(test_reward_vault PRIVATE ${PROJECT_SOURCE_DIR}/include)
add_test(NAME test_reward_vault COMMAND test_reward_vault)

add_executable(test_gas_vault tests/test_gas_vault.cpp)
target_link_libraries(test_gas_vault ${OPENSSL_LIBRARIES} Threads::Threads)
target_include_directories(test_gas_vault PRIVATE ${PROJECT_SOURCE_DIR}/include)
add_test(NAME test_gas_vault COMMAND test_gas_vault)

add_executable(test_merkle_provenance tests/test_merkle_provenance.cpp src/merkle_provenance.cpp src/crypto.cpp)
target_link_libraries(test_merkle_provenance ${OPENSSL_LIBRARIES} Threads::Threads)
target_include_directories(test_merkle_provenance PRIVATE ${PROJECT_SOURCE_DIR}/include)
add_test(NAME test_merkle_provenance COMMAND test_merkle_provenance)

add_executable(test_ipfs_verifier tests/test_ipfs_verifier.cpp src/ipfs_verifier.cpp src/crypto.cpp)
target_link_libraries(test_ipfs_verifier ${OPENSSL_LIBRARIES} Threads::Threads)
target_include_directories(test_ipfs_verifier PRIVATE ${PROJECT_SOURCE_DIR}/include)
add_test(NAME test_ipfs_verifier COMMAND test_ipfs_verifier)

add_executable(test_llm_mempool_governor tests/test_llm_mempool_governor.cpp src/llm_mempool_governor.cpp src/merkle_provenance.cpp src/crypto.cpp)
target_link_libraries(test_llm_mempool_governor ${OPENSSL_LIBRARIES} Threads::Threads)
target_include_directories(test_llm_mempool_governor PRIVATE ${PROJECT_SOURCE_DIR}/include)
add_test(NAME test_llm_mempool_governor COMMAND test_llm_mempool_governor)

add_executable(test_zk_compute tests/test_zk_compute.cpp src/zk_compute.cpp src/merkle_provenance.cpp src/crypto.cpp)
target_link_libraries(test_zk_compute ${OPENSSL_LIBRARIES} Threads::Threads)
target_include_directories(test_zk_compute PRIVATE ${PROJECT_SOURCE_DIR}/include)
add_test(NAME test_zk_compute COMMAND test_zk_compute)

# Install
install(TARGETS membra_l3 DESTINATION bin)
