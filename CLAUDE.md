# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

Requires an external CMake library (`CMLIB_DIR`):

```bash
# Standard build
mkdir -p _build && cd _build
cmake ../ -DCMLIB_DIR=</absolute/path/to/cmakelib>
make

# Build with tests — must use clang++-17 (gcc causes AddressSanitizer crashes)
mkdir -p _build_tests && cd _build_tests
cmake ../ -DCMLIB_DIR=</absolute/path/to/cmakelib> -DBRINGAUTO_TESTS=ON -DCMAKE_CXX_COMPILER=clang++-17
make
```

Key CMake options:
- `BRINGAUTO_TESTS` — enable tests (requires clang++-17)
- `BRINGAUTO_INSTALL` — enable install target
- `BRINGAUTO_PACKAGE` — enable packaging (forces INSTALL=ON)
- `BRINGAUTO_SYSTEM_DEP` — use system deps instead of fetching
- `BRINGAUTO_MODULE_GATEWAY_MINIMUM_LOGGER_VERBOSITY` — compile-time log level (DEBUG/INFO/WARNING/ERROR/CRITICAL)

## Tests

```bash
# Run all tests
cd _build_tests && ctest

# Run a specific test binary directly (e.g. for verbose output)
cd _build_tests && ./test/ModuleHandlerTests
```

Tests use port 8888 for the internal server — it must be free when running `InternalServerTests`.

## Run

```bash
./module-gateway-app --config-path=../resources/config/default.json
# Optional: --port=<unsigned short>  overrides the internal server port
```

## Architecture

The gateway bridges internal device clients with external servers. Three main components cooperate via `AtomicQueue`-based message passing:

```
Internal Client ──► Internal Server ──► [fromInternalQueue] ──► Module Handler
                                                                       │
                                                              Module Libraries &
                                                              Status Aggregators
                                                                       │
                ◄── Internal Client ◄── [toInternalQueue] ◄───────────┤
                                                                       │
                External Client ◄────── [toExternalQueue] ◄───────────┘
                      │
              Error Aggregator
                      │
              External Server (MQTT / QUIC / Dummy)
```

### Internal Server (`internal_server/`)
Accepts TCP connections from internal clients (devices). Messages are length-prefixed (4-byte little-endian). Forwards device messages to Module Handler; returns responses to the originating client.

### Module Handler (`modules/`)
The processing core. Loads module shared libraries via `library_loader.hpp`, manages a `StatusAggregator` per device, and routes messages between Internal Server and External Client. Supports local execution (`ModuleManagerLibraryHandlerLocal`) and async shared-memory execution (`ModuleManagerLibraryHandlerAsync`).

### External Client (`external_client/`)
Manages connections to one or more external server endpoints. Handles reconnection with backoff via `ReconnectQueueItem`. Uses `ErrorAggregator` to buffer messages that failed to deliver. Protocol is abstracted behind `ICommunicationChannel`; concrete implementations: `MqttCommunication`, `QuicCommunication`, `DummyCommunication`.

### Settings (`settings/`)
JSON config is parsed by `SettingsParser` (and `QuicSettingsParser` for QUIC-specific fields) into `Settings`. Configuration examples are in `resources/config/`.

### Key shared structures (`structures/`)
- `GlobalContext` — holds `boost::asio::io_context` and `shared_ptr<Settings>`, passed everywhere
- `AtomicQueue<T>` — thread-safe queue used for all inter-component communication
- `DeviceIdentification` — unique key for a device (module + device type + device role + device name + priority)

## Language & Conventions

- C++23, compiled with `-Wall -Wextra -Wpedantic`
- AddressSanitizer + UBSan + LeakSanitizer are enabled in Debug/Test builds
- Headers live in `include/bringauto/`, implementations mirror that path under `source/bringauto/`
- Protobuf for serialization; Boost ASIO for async I/O; nlohmann-json for config parsing
