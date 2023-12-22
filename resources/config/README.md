# Module Gateway - CONFIGURATION

Setting used in Module Gateway can be parsed out of .json file.

## JSON settings

### general-settings:
* log-path : 
  - string
  - logs will be saved to provided folder
  - path can be both relative and absolute
* verbose : 
  - bool
  - if true logs will be printed to console
### internal-server-settings: 
* port : 
    - unsigned short 
    - port on which internal server will communicate on
	- possible values 1 - 65535
### module-handler-settings:
* module-paths :
  - vector of strings
  - paths to shared module libraries
## Example

[Example](./example.json)


