# Module Gateway - CONFIGURATION

Setting used in Module Gateway can be parsed out of .json file.

## JSON settings

### logging:
* console :
  - level : console logging level (DEBUG, INFO, WARNING, ERROR, CRITICAL)
  - use : if logs will be printed to console (bool)
* file :
  - level : file logging level (DEBUG, INFO, WARNING, ERROR, CRITICAL)
  - use : if logs will be printed to files (bool)
  - path : logs will be saved to provided folder, path can be both relative and absolute (string)

Note: at least one logging sink needs to be used
### internal-server-settings: 
* port : 
    - unsigned short 
    - port on which internal server will communicate on
	- possible values 1 - 65535
### module-paths:
* key : number that corresponds to the module being loaded
* value : path to the module shared library file
### module-binary-path:
  - path to the module binary for async function execution over shared memory. If none is provided, the module will be loaded as a shared library
### external-connection:
* company : company name used as identification in external connection (string)
* vehicle-name : vehicle name used as identification in external connection (string)
* endpoints : array of objects listing possible ways to connect to external server
  - protocol-type : string (only mqtt and quic are supported)
  - server-ip : ip of the external connection (string)
  - port : port of the external connection (int)
  - modules : array of integers that represent module numbers to be used on this connection

#### mqtt-settings (only for MQTT)
* ssl : if connection requires ssl, bool
* ca-file : public trusted certificate file name (string)
* client-cert : public certificate chain file name (string)
* client-key : private key file name (string)

#### quic-settings (only for QUIC)
* ca-file : path to the trusted CA certificate file (string)
* client-cert : path to the client certificate file (string)
* client-key : path to the client private key file (string)
* alpn : Application-Layer Protocol Negotiation identifier (string), must match the ALPN configured on the server
  
Note: QUIC uses TLS 1.3 internally. All certificate files must be provided in a format supported by MsQuic/OpenSSL.

## Examples

[MQTT Example](./example.json)
[QUIC Example](./quic_example.json)

