from abc import ABCMeta, abstractmethod

import module_gateway.protobuf.InternalProtocol_pb2 as internal_protocol


class MessageCreator(metaclass=ABCMeta):

    @staticmethod
    def create_connect_response(type) -> internal_protocol.DeviceConnectResponse:
        message = internal_protocol.DeviceConnectResponse()
        message.responseType = type.value
        return message

    @abstractmethod
    def create_command(self, status_bytes: bytes) -> internal_protocol.DeviceCommand:
        pass
