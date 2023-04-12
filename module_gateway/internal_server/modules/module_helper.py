from .module_type import (
    CarAccessoryType,
    MissionType,
    ModuleType
)
import module_gateway.protobuf.InternalProtocol_pb2 as internal_protocol


def create_connect_response(type) -> internal_protocol.DeviceConnectResponse:
    dconnectresponse_msg = internal_protocol.DeviceConnectResponse()
    dconnectresponse_msg.responseType = type.value
    message = internal_protocol.InternalServer()
    message.deviceConnectResponse.CopyFrom(dconnectresponse_msg)
    return message.SerializeToString()


def check_supported_device_for_module(module: int, device_type: int) -> bool:
    match module:
        case ModuleType.CAR_ACCESSORY_MODULE.value:
            return CarAccessoryType.has_value(device_type)
        case ModuleType.MISSION_MODULE.value:
            return MissionType.has_value(device_type)
        case _:
            return False
