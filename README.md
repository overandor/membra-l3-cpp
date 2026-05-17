# MEMBRA Layer-3 C++ Implementation

High-performance C++ implementation of MEMBRA L3 protocol with protocol-subsidized gas model.

## Key Feature: Protocol-Subsidized Gas

**Neither sender nor receiver pays gas fees.** The protocol pays all gas from its own reserves.

- Sender pays: 0 SOL
- Receiver pays: 0 SOL
- Protocol pays: All gas fees

This is economically unsustainable long-term (protocol drains reserves), but demonstrates the requested model.

## Architecture

- **ProofBook**: Immutable hash-chained proof log (ring-buffer optimized)
- **GasVault**: Protocol reserves, pays all gas, no user reimbursement
- **IntentNetwork**: Gasless payment intents (7-day claim window)
- **VolatilityOracle**: High-performance TWAP with ring buffers
- **C++17**: Modern features, thread-safe, minimal allocations

## Build

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

## Important Note

This implementation assumes the protocol has unlimited SOL reserves to subsidize all gas. In production:
- Protocol would eventually drain reserves
- Would need continuous SOL injection
- Economically unsustainable without external funding

The Python version uses a more sustainable model (fee credits + relayer reimbursement).
