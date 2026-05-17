# MEMBRA Layer-3 C++ Implementation

High-performance C++ implementation of MEMBRA L3 protocol with **compute-collateralized gas model with MEMBRA gas credits and futures market**.

## ⚠️ CRITICAL DISCLAIMER

**MEMBRA GAS CREDITS ARE NOT SOL. COMPUTE DOES NOT CREATE MONEY.**

- Compute staking issues **MEMBRA Gas Credits** (internal accounting units)
- Gas Credits make MEMBRA intents sender-gasless
- **Real Solana fees still require funded GasVault, relayer, or receiver-paid settlement**
- **DO NOT** expect guaranteed passive income from staking compute
- Hardware depreciation and electricity costs may exceed earnings
- This is a **simulation/prototype** for development and testing
- **Mainnet is disabled** by default
- Audit required before any production deployment

## Key Features

### 1. Stake CPU/RAM to Receive MEMBRA Gas Credits

**Users lock CPU/RAM to receive MEMBRA Gas Credits.** These credits make MEMBRA intents sender-gasless.

- User locks CPU/RAM → Gets MEMBRA Gas Credits
- User sends gasless MEMBRA intents using gas credits
- Sender pays: 0 SOL (gas accounted against credits)
- Receiver pays: 0 SOL
- **Real Solana fees require funded GasVault, relayer, or receiver-paid model**
- **Gas Credits are NOT redeemable for SOL - internal accounting only**

### 2. Compute Futures Market

**Trade futures on CPU/RAM prices.** Compute prices are appraised and volatile, similar to gas.

- **Forward Contracts**: Lock compute for future use (hedging)
- **Speculative Long**: Bet compute price will rise (with leverage)
- **Speculative Short**: Bet compute price will fall (with leverage)
- **Compute Index**: Combined CPU + RAM price (60% CPU, 40% RAM)
- **Liquidation**: Automatic liquidation when collateral ratio < 50%
- **Futures trading involves significant risk of loss**

### 3. Phase-Two Infrastructure (Development)

- **M5 Benchmarks**: Performance profiling for Apple Silicon
- **Proof-of-Inference**: Receipts for compute work verification
- **Reward Vault**: Accounting with strict caps and funding checks
- **Receiver-Paid Gas**: Gas compensation simulation (devnet only)
- **Token Compensation Caps**: Prevent unlimited token minting

## How It Works

### Staking Flow
1. **User locks CPU cores + RAM** (e.g., 4 cores + 16 GB RAM)
2. **MEMBRA issues gas credits** (10 SOL-equivalent credits per core)
3. **User gets gas credit allowance** (10 SOL-equivalent credits per core)
4. **User sends gasless MEMBRA intents** using gas credits
5. Gas credits are debited when intents settle
6. **Real Solana fees** require funded GasVault, relayer, or receiver-paid model
7. Gas credits are **internal MEMBRA accounting**, not redeemable for SOL

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

### Sustainability Considerations

This model can be sustainable long-term only if:
- Users lock real compute resources (CPU/RAM) to receive gas credits
- MEMBRA issues gas credits for internal fee accounting (not money creation)
- Gas credits make MEMBRA intents sender-gasless
- **Real Solana fees require funded GasVault, relayer, or receiver-paid settlement**
- Protocol reserves only used as backup
- Futures market provides price discovery and hedging
- No unlimited money creation - compute is a real resource
- **Gas Credits are NOT redeemable for SOL**

### ⚠️ Earnings Reality Check

For an M5 MacBook Pro ($3000):
- Locking 12 cores issues **MEMBRA Gas Credits** (not SOL)
- Gas Credits are for internal MEMBRA fee accounting only
- **Gas Credits are NOT redeemable for SOL**
- To use MEMBRA, you need funded GasVault, relayer, or receiver-paid model
- Hardware depreciation (~$300-500/year) may exceed benefits
- Electricity costs ($20-50/month depending on usage) may exceed benefits
- **DO NOT** lock compute expecting to earn SOL
- Gas Credits subsidize your own MEMBRA usage, not generate income

### Correct Model

```
User locks CPU/RAM
↓
MEMBRA measures and records compute collateral
↓
User receives MEMBRA Gas Credits
↓
User can submit "gasless" MEMBRA intents
↓
A relayer/GasVault pays real Solana SOL fees if settlement touches Solana
↓
Gas Credits are burned/accounted against the user's allowance
↓
ProofBook records the transaction
```

### Compensation Caps

To prevent abuse:
- Max reward per receipt: 1 SOL
- Max reward per wallet per day: 10 SOL
- Max reward per epoch: 100 SOL
- Max pool balance: 1000 SOL
- Oracle stale check: 5 minutes
- Replay detection: Each receipt can only be used once

## Comparison to Python Version

- Python: Fee credits + relayer reimbursement (ZK compute)
- C++: Direct CPU/RAM staking with instant rewards + gas allowance + futures market (no ZK required)
- Both: Neither sender nor receiver pays gas
