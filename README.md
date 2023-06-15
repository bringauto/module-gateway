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

## Requirements

- [protobuf](https://github.com/protocolbuffers/protobuf/tree/main/src) >= v3.21.12
- [cxxopts](https://github.com/jarro2783/cxxopts) >= v3.0.0
- [boost](https://github.com/boostorg/boost) >= v1.74.0
- [nlohmann-json](https://github.com/nlohmann/json) >= v3.2.0
- [ba-logger](https://github.com/bringauto/ba-logger) >= v1.2.0

- [cmlib](https://github.com/cmakelib/cmakelib)

## Build
```
mkdir -p _build && cd _build
cmake ../ -DCMLIB_DIR=<absolute path cmakelib>
make
```

## Run

```
./ModuleGateway --config-path=../configs/default.json
```

### Arguments

* required arguments:
	* `-c | --config-path <string>`path to json configuration file ([Configs Readme](./configs/README.md))
* `-v | --verbose` logs will be printed to console
* `-l | --log-path <string>` logs will be saved to provided path
* `-h | --help` print help

* `--port ` unsigned short, port on which Internal Server communicates

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
