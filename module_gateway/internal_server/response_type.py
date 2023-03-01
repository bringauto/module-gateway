from enum import Enum


class ResponseType(Enum):
    OK = 0
    ALREADY_CONNECTED = 1
    MODULE_NOT_SUPPORTED = 2
    DEVICE_NOT_SUPPORTED = 3
    HIGHER_PRIORITY_ALREADY_CONNECTED = 4
