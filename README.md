# MEMBRA Layer-3 C++ Implementation

High-performance C++ implementation of MEMBRA L3 protocol with **compute-collateralized gas model with instant rewards**.

## Key Feature: Compute-Collateralized Gas with Instant Rewards

**Neither sender nor receiver pays gas fees.** Gas is subsidized by staked CPU/compute resources, and MEMBRA pays stakers instantly when they lock compute.

- Sender pays: 0 SOL
- Receiver pays: 0 SOL
- Gas paid by: Staked CPU/compute resources
- Reserves: Backup only (used when no staked compute)
- **Stakers receive: Instant SOL payment when locking compute + time-based rewards**

## How It Works

1. Users stake CPU cores/compute units
2. **MEMBRA pays stakers instantly (10 SOL per core) when they lock compute**
3. Staked compute earns gas credits continuously
4. Gas fees are paid from staked compute first
5. Protocol reserves are only used as backup
6. Stakers can unstake after minimum duration (1 hour) and claim additional time-based rewards

## Architecture

- **ProofBook**: Immutable hash-chained proof log (ring-buffer optimized)
- **ComputeStaking**: CPU/compute staking with instant rewards + time-based rewards
- **GasVault**: Uses staked compute for gas, reserves as backup
- **IntentNetwork**: Gasless payment intents (7-day claim window)
- **VolatilityOracle**: High-performance TWAP with ring buffers
- **C++17**: Modern features, thread-safe, minimal allocations

## Build

```bash
g++ -std=c++17 -O3 -I./include -o membra_l3 src/main.cpp -lcrypto -lpthread
./membra_l3
```

Or with CMake:
```bash
mkdir build && cd build
cmake ..
make
./membra_l3
```

## Dependencies

- OpenSSL (for SHA256)
- C++17 compiler
- Threads library

## Performance Optimizations

- Ring buffers for price history (no allocations)
- Atomic operations for counters
- Mutex-protected critical sections
- Cache-friendly data structures
- `-O3 -march=native` compilation flags

## Economic Model

This is sustainable long-term because:
- Gas is subsidized by real compute resource staking
- Protocol reserves are only backup
- **Stakers receive instant SOL incentive (10 SOL per core) to lock compute**
- Stakers earn additional time-based rewards for longer stakes
- No unlimited money creation - compute is a real resource

## Comparison to Python Version

- Python: Fee credits + relayer reimbursement (ZK compute)
- C++: Direct CPU/compute staking with instant rewards (no ZK required)
- Both: Neither sender nor receiver pays gas
