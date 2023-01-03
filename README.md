# Module Gateway

Directory contains fake module gateway that communicates with internal client and external server.

## Requirements

- Python (version >= 3.10)
- Other requirements can be installed by `pip3 install -r requirements.txt`

## Arguments

- `-i or --ip-address <string>` = ip address of the module gateway
- `-p or --port <int>` = port of the module gateway

### Testing argumets

can be used only with one of these arguments

- `--comm-error` = client should raise CommunicationError
- `--serv-too-long` = client should raise ServerTookTooLong

## Run
```
python3 main.py
```
