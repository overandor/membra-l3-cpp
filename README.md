# MEMBRA Layer-3 C++ Implementation

High-performance C++ implementation of MEMBRA L3 protocol with **compute-collateralized gas model with instant rewards and futures market**.

## Key Features

### 1. Stake CPU/RAM to Send Transactions for Free

**Users stake their own CPU and RAM to send transactions for free.** MEMBRA pays stakers instantly when they lock compute.

- User stakes CPU/RAM → Gets instant SOL payment + gas allowance
- User sends transactions for free using gas allowance
- Sender pays: 0 SOL (uses gas allowance from staked compute)
- Receiver pays: 0 SOL
- Reserves: Backup only (used when gas allowance exhausted)

### 2. Compute Futures Market

**Trade futures on CPU/RAM prices.** Compute prices are appraised and volatile, similar to gas.

- **Forward Contracts**: Lock compute for future use (hedging)
- **Speculative Long**: Bet compute price will rise (with leverage)
- **Speculative Short**: Bet compute price will fall (with leverage)
- **Compute Index**: Combined CPU + RAM price (60% CPU, 40% RAM)
- **Liquidation**: Automatic liquidation when collateral ratio < 50%

## How It Works

### Staking Flow
1. **User stakes CPU cores + RAM** (e.g., 4 cores + 16 GB RAM)
2. **MEMBRA pays instantly** (10 SOL per core) → User gets 40 SOL
3. **User gets gas allowance** (10 SOL per core) → 40 SOL for free transactions
4. **User sends transactions for free** using gas allowance
5. Gas allowance decreases with each transaction
6. When exhausted, user can stake more or wait for time-based rewards
7. After 1 hour minimum, user can unstake and claim additional time-based rewards

### Futures Flow
1. **Compute Price Oracle** tracks CPU and RAM prices in real-time
2. **Compute Index** = (CPU price × 0.6) + (RAM price × 0.4)
3. **Forward Contract**: Lock 8 cores + 32 GB RAM for 24 hours at current price
4. **Speculative Long**: Bet price rises with 5x leverage
5. **Speculative Short**: Bet price falls with 3x leverage
6. **Liquidation Check**: Auto-liquidate if collateral ratio < 50%

## Architecture

- **ProofBook**: Immutable hash-chained proof log (ring-buffer optimized)
- **ComputeStaking**: CPU/RAM staking with instant rewards + gas allowance
- **ComputePriceOracle**: Real-time CPU/RAM price tracking with compute index
- **ComputeFuturesMarket**: Forward contracts + speculative trading with leverage
- **GasVault**: Uses user's gas allowance first, reserves as backup
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
- Users stake real compute resources (CPU/RAM) to get free gas
- MEMBRA pays instant incentive (10 SOL per core) to encourage staking
- Gas allowance is tied to amount staked (10 SOL gas allowance per core)
- Protocol reserves only used as backup
- Futures market provides price discovery and hedging
- No unlimited money creation - compute is a real resource

## Comparison to Python Version

- Python: Fee credits + relayer reimbursement (ZK compute)
- C++: Direct CPU/RAM staking with instant rewards + gas allowance + futures market (no ZK required)
- Both: Neither sender nor receiver pays gas
