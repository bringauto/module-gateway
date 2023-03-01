# Module Gateway

Directory contains fake module gateway that communicates with internal client and external server.

## Requirements

- Requirements can be installed by `pip3 install -r requirements.txt`
- Mqtt broker for internal process communication, default ip address is 127.0.0.1 and default port is 1883
    - default ip address can be set by enviroment variable `IPC_MQTT_HOST`
    - default port can be set by enviroment variable `IPC_MQTT_PORT`

## Run all

Run Internal client, Module handler and External client by one script.\
When you use this script there is one disadvantage.\
Logs from all scripts will be printed to the one terminal session.

```
python3 run_all.py
```

## Docker

Prefered way to start the module gateway.\
Uses docker-compose.yml and mosquitto.conf files.
```
sudo docker compose up
```

## Internal Server

Accepts communication from internal clients and communicates with them through tcp connection.\
As soon as some client is connected, internal server also connects to the [module handler](#Module-handler).\
Internal server sends status messages to the module handler and module handler sends back\
command messages, which are sent to the internal clients.\
Internal server validates internal client that is trying to connect.

### Requirements

- Python (version >= 3.11)

### Arguments

- `-i or --ip-address <str>` = ip address of the module gateway, default = `127.0.0.1`
- `-p or --port <int>` = port of the module gateway, default = `8888`

#### Testing argumets

can be used only with one of these arguments

- `--comm-error` = client should raise CommunicationError
- `--serv-too-long` = client should raise ServerTookTooLong

### Run
```
python3 run_internal_server.py
```

Docker build:
```
docker build -t internal_server -f module_gateway/internal_server/Dockerfile .
```
Docker run:
```
docker run -t internal_server
```


## Module handler

Module handler aggregates status messages recieved from the internal server and sends them to\
the external client. External client sends back command messages, which are forwarded to the internal server.

### Requirements

- Python (version >= 3.11)

### Run
```
python3 run_modules.py
```

Docker build:
```
docker build -t module_handler -f module_gateway/module_handler/Dockerfile .
```
Docker run:
```
docker run -t module_handler
```

## External client

External client receives aggregated status messages from the module handler and sends it to the external server.\
If communication between external client and external server is broker, then external client aggregates status messages\
in error aggregator. As soon as the connection is restored, external client sends aggregated error message to the external\
server. External server receives and process aggregated status messages and sends back command messages. External client\
forwards them to the module handler.

### Requirements

- Python (version >= 3.11)

### Run
```
python3 run_external_client.py
```

Docker build:
```
docker build -t external_client -f module_gateway/external_client/Dockerfile .
```
Docker run:
```
docker run -t external_client
```

## Adding support for new module
This sections briefly describes what needs to be implemeted to add new module. Each module has specific `module_type` and supports devices of specific `device_type` (see fleet protocol specification). Because this POC implementation is split into three distinctive parts, module implementation is composed of multiple steps.
### 1. Adding module to internal server
Modifications in [module_type.py](module_gateway/internal_server/modules/module_type.py):
1. Add module name and `module_type` into `ModuleType` Enum. 
2. Create Module specific Enum containing all `device_type`s of supported devices for this module. (see `MissionType` for mission module)

Modifications in [module_helper.py](module_gateway/internal_server/modules/module_helper.py):
1. Module specific enum needs to be imported.
2. Add this enum into `match` inside of `check_supported_device_for_module` .

### 2. Adding module into module handler
In this step the actual module status aggreagation will be implemented. 
Modifications in [modules](module_gateway/module_handler/modules) folder:
1. Create new module file. (see `car_accessory_module.py` as example)
2. In this file, implement all abstract functions of `ModuleManagerBase`.
3. Implement the actual module by subclassing `ModuleBase`, setting `MODULE_ID` to `module_type`, `SUPPORTED_DEVICE_IDS` to set of all `device_type` values supported by this module, `MODULE_MANAGER` as manager implemented in previous step. 

Module implementation is now done, only thing remaining to do is instruct the module manager to load this new module:
- in [modules_manager.py](module_gateway/module_handler/modules_manager.py), import this new module implementation and add it to `SUPPORTED_MODULES` class attribute list.

### 3. Adding module into external client
This requires implementation of module specific error aggregation in [modules](module_gateway/external_client/error_aggregator/modules) folder:
1. Create new file for specific module error aggregator implementation (see `car_accessory_error_aggregator.py` as example).
2. Subclass `ModuleAggregatorBase` and implement abstract method `create_device_error`.


To instruct the [error aggregator](module_gateway/external_client/error_aggregator/error_aggregator.py) to load this module:
1. Import this module.
2. Add this module to `MODULE_ID_TO_AGGREGATOR` class attribute dictionary, where key is `module_type` and value is implemented module.


After all these steps are done, module is implemented and should be loaded.
