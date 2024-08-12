# Module Gateway - TESTS

!!! NOTICE !!!
Tests should be compiled with a different compiler than **gcc-11**, which is used by default on Ubuntu 22.04.
Otherwise, the build or run may fail with error: `AddressSanitizer:DEADLYSIGNAL`

The only tested working compiler is **clang++-17**

Google Tests

### InternalServerTests suite:

Handles testing of Internal Server component

Uses port 8888 for communication. As such requires the port to be unused.

Each test is composed of main three different components:
* Each test case constructs TestHandler(using vector of devices and data) object which will hold the testing data, and handles threads on which the tests run and communications between Internal Server and  other components happen.
* Testhandler will on one thread starts up Dummy "Module Handler" that will answer to each received connect and status with command or response.
* For each received device TestHandler will constructs Dummy "Internal Client" which will offer TestHandler with operations (connect, disconnect, and versions of send and receive message). These operations test for correct responses(behavior of Internal Server).

Tests can be split into 5 different groups:
* Testing for multiplicity of different Clients creating multiple Connections.
  - Is done by parallel running threads each handling the entire communication between each "Internal Client" and Internal Server.
* Testing repeated responses to connect from the "same" device with different priority.
  - main thread runs serially all communication between client and server. Starting with try to connect all devices, then running communication for period of time. Testing for correct responses to connects, and disconnection of overridden lower priority deice.
* Testing response to invalid messages.
  - main thread runs serially all communication between client and server. Tests for disconnection when invalid message is sent.
* Testing for correct behavior of timeouts when module handler does not respond to received message.
  - "module handler" intentionally skips response to certain amount of messages
  - main thread runs serially all communication between client and server. Tests for disconnection when message is intentionally skipped.
  - NOTICE: tests should be rewritten and client server communication run parallel to each other.
* Testing for correct behavior of timeouts when Client does not send whole message.
  - NOTICE: both the behavior and tests for it are not implemented

### ModuleHandlerTests suite:

### ExternalClientTests suite:

## Requirements

[gtest](https://github.com/google/googletest)

## Build
```
mkdir -p _build_tests && cd _build_tests
cmake ../ -DCMLIB_DIR=<absolute path cmakelib> -DBRINGAUTO_TESTS=ON -DCMAKE_CXX_COMPILER=clang++-17
make
```

## Run
```
./ctest
```

