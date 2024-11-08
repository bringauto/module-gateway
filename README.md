# Module Gateway

Directory contains cpp implementation of module gateway that communicates with internal client and external server.
Module Gateway is one component of Fleet protocol.

Module Gateway is  composed of:

### Internal Server

The internal server handles the connection and incoming data from the Internal Clients (devices).
After verification the data are parsed into protobuf message that is sent to Module Handler.
Internal server then receives response to the message and sends it back to Internal Client.

### Module Handler

The module handler is responsible for receiving protobuf messages from internal clients via the internal server.
It processes these messages using the status aggregator and subsequently sends corresponding commands back to the internal client.
Each status aggregator holds information about the connected internal client, including aggregated statuses, commands, and the actual status.
The external client updates the command for each connected device and subsequently transmits the aggregated messages to the external server.

### External Client

The external client is responsible for initializing connection with external server, reconnecting, delivering protobuf messages from module
handler to external server and updating devices commands. It uses error aggregator to process messages which could not be delivered, when
connection is broken and as soon as the connection is up, then error aggregated message is sent.

## Requirements

- [cmlib](https://github.com/cmakelib/cmakelib)

- [protobuf](https://github.com/protocolbuffers/protobuf/tree/main/src) >= v3.21.12
- [cxxopts](https://github.com/jarro2783/cxxopts) >= v3.0.0
- [boost](https://github.com/boostorg/boost) >= v1.74.0
- [nlohmann-json](https://github.com/nlohmann/json) >= v3.2.0
- [ba-logger](https://github.com/bringauto/ba-logger) >= v1.2.0
- g++ >= 10 or other compiler with c++20 support

## Build
```
mkdir -p _build && cd _build
cmake ../ -DCMLIB_DIR=</absolute/path/cmakelib>
make
```

## Run

```
./module-gateway-app --config-path=../resources/config/default.json
```

### Arguments

* required arguments:
  * `-c <string> | --config-path=<string>`path to json configuration file ([Configs Readme](./configs/README.md))
* All arguments:
  * `-h | --help` print help
  * `--port=<unsigned short>` port on which Internal Server communicates

### CMAKE arguments

* CMLIB_DIR=\<PATH>
* BRINGAUTO_TESTS=ON/OFF
  - DEFAULT: OFF
  - if on enable build/configure of tests
  - if off disable build/configure of tests

* BRINGAUTO_INSTALL=ON/OFF
  - DEFAULT: OFF
  - if on enable install feature,
  - if off disable install feature,

* BRINGAUTO_PACKAGE=ON/OFF
  - DEFAULT: OFF
  - if on enable package creation - if the BRINGAURO_INSTALL is not ON then is switched ON with warning,
  - if off disable package creation

* BRINGAUTO_SYSTEM_DEP=ON/OFF
  - DEFAULT: OFF


* CURRENTLY UNUSED
  * BRINGAUTO_SAMPLES=ON/OFF
	  - DEFAULT: OFF
	  - if on enable build/configure of sample aplications
	  - if off disable build/configure of sample application



## Tests

[Tests Readme](./test/README.md)

## Memory leaks

It is possible, that valgrind will show you, that there are still reachable memory leaks. It is caused by dlopen function.
We are not sure, if bug is in dlopen or in valgrind, but we cannot do anything with that.
The bug is already [reported](https://bugs.kde.org/show_bug.cgi?id=358980).

## Dockerfile

Port 1636 is exposed in the dockerfile.
