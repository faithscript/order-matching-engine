# order-matching-engine

![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)
![CMake](https://img.shields.io/badge/CMake-%3E%3D3.15-orange)
![License: MIT](https://img.shields.io/badge/License-MIT-brightgreen)

An **event‑sourced, single‑threaded matching engine** with **FIX 4.4** connectivity, built in progressive layers toward ultra‑low‑latency trading systems.

## Table of Contents
- [Architecture](#architecture)
- [What We've Completed](#what-weve-completed)
- [Quick Start (Build & Run)](#quick-start)
- [Demo: Send a FIX Message](#demo)
- [Testing](#testing)
- [Next Steps](#next-steps)
- [Contributing](#contributing)
- [License](#license)

## Architecture

| Layer | Description |
| --- | --- |
| 0 — Core | Event‑sourced, single‑threaded matching engine |
| 1 — FIX Protocol | FIX tag‑value connectivity over sockets |
| 2 — Memory & Speed | Intrusive data structures & custom memory pool |
| 3 — Concurrency | Lock‑free pipelines (LMAX‑Disruptor) |
| 4 — Market Data | UDP multicast market‑data feed |
| Stretch — Recovery | Snapshot + log replay, FIX session recovery |

## What We've Completed

- **Core Engine (Layer 0)** – Event‑sourced design with `OrderAdded`, `OrderCancelled`, `OrderModified`, `Trade` events, a `Sequencer` for monotonic IDs, and full replay capability.
- **FIX Protocol Integration (Layer 1)** – Hand‑rolled parser/encoder supporting `NewOrderSingle`, `OrderCancelRequest`, `ExecutionReport`, and `Reject`. Added `FixParser`, `FixEncoder`, and socket wrapper (`TcpServer`).
- **`Book::apply_fix`** – Bridges parsed FIX messages to core events; now supports `Reject` messages without generating engine events.
- **Socket Server** – `TcpServer` runs on port 9876, receives raw FIX strings, parses them, feeds them to the book, and prints encoded responses.
- **Unit Tests** – `tests/test_fix.cpp` validates parser, encoder, and end‑to‑end socket handling.
- **Documentation Cleanup** – Updated README, added `.gitignore` entry for `build/`.

## Quick Start (Build & Run)

```bash
# Clone the repo
git clone https://github.com/faithanyanwu/order-matching-engine.git
cd order-matching-engine

# Build (requires CMake ≥3.15 and a C++20 compiler)
cmake -S . -B build && cmake --build build

# Run the server (listens on TCP 9876)
./build/main
```

The server will output encoded FIX messages to the console.

## Demo: Send a FIX Message

In another terminal, send a *NewOrderSingle* using `netcat` (or any TCP client):

```bash
printf "8=FIX.4.4\x01""9=...\x01""35=D\x01""11=123\x01""54=1\x01""44=100\x01""38=10\x01""10=...\x01" | nc localhost 9876
```

You should see an `ExecutionReport` printed by the server, e.g.:

```
8=FIX.4.4|9=...|35=8|11=123|54=1|44=100|38=10|151=0|14=10|10=...
```

(Length and checksum fields are calculated automatically by the engine.)

## Testing

```bash
# Build the tests (already part of the CMake configuration)
cmake -S . -B build && cmake --build build
# Run all tests
ctest --test-dir build
```

The test suite covers core matching logic, FIX parsing/encoding, and the socket integration.

## Next Steps

- **Layer 2 – Memory & Speed**: Introduce intrusive containers and a custom memory pool to eliminate per‑order heap allocations and further reduce latency.
- **Layer 3 – Concurrency**: Refactor the sequencer and matching loop into a lock‑free SPSC/MPSC pipeline based on the LMAX Disruptor pattern.
- **Layer 4 – Market Data**: Add a UDP multicast publisher for top‑of‑book and depth updates, and a corresponding subscriber library.
- **Stretch – Recovery**: Implement periodic snapshots and log replay, plus FIX session‑layer recovery (gap detection, resend requests).

## Contributing

Contributions are welcome! Please:
1. Fork the repository and create a feature branch.
2. Follow the existing coding style (`clang-format` is configured).
3. Run the full test suite locally (`ctest`).
4. Open a Pull Request with a clear description of the change.

See [CONTRIBUTING.md](CONTRIBUTING.md) for more details.

## License

This project is licensed under the MIT License – see the [LICENSE](LICENSE) file for details.

---

## Layer 0 — Core

Single-threaded matching engine with an append-only event log (`OrderAdded`, `OrderCancelled`, `OrderModified`, `Trade`) as the source of truth. A sequencer assigns a strict monotonic sequence number to each event. Orders follow a `New → PartiallyFilled → Filled / Cancelled / Rejected` state machine, matched using price-time priority. Book state is derived entirely by replaying the event log, verified by wiping in-memory state and rebuilding it from scratch.
 
## Layer 1 — FIX Protocol
 
Hand-rolled parser and encoder for core FIX message types: `NewOrderSingle`, `OrderCancelRequest`, `ExecutionReport`, and `Reject`, targeting FIX 4.2/4.4. Replaces internal test structs with real FIX messages over a socket.
 
## Layer 2 — Memory & Speed
 
Intrusive data structures for price levels and order queues to eliminate per-order heap allocation, backed by a custom memory pool. Optimization is profiler-driven, measured with latency histograms (p50/p99/p99.9) rather than throughput alone.
 
## Layer 3 — Concurrency
 
Moves the sequencer to a lock-free, single-writer pipeline using a ring buffer (SPSC, extended to MPSC) based on the LMAX Disruptor pattern, with the sequencer and matching engine running on separate threads.
 
## Layer 4 — Market Data
 
Separates order entry from market data distribution. Top-of-book and depth updates are published over UDP multicast, reconstructed independently by a separate subscriber process.
 
## Stretch — Recovery
 
Periodic snapshotting to bound replay time, crash recovery from snapshot plus log replay, and FIX session-layer recovery via sequence gap detection and resend requests.
