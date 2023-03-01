import module_gateway.protobuf.ExternalProtocol_pb2 as ExternalProtocolProtob
import module_gateway.protobuf.InternalProtocol_pb2 as InternalProtocolProtob
from module_gateway.external_client.device import Device


class ProtoHelpers:
    @staticmethod
    def proto_create_status(
        sessionId: str,
        deviceState: int,
        messageCounter: int,
        status,
        errorMessage: bytes | None = None,
    ):
        external_msg = ExternalProtocolProtob.ExternalClient()
        encapsulated_status = ExternalProtocolProtob.Status()
        encapsulated_status.sessionId = sessionId
        encapsulated_status.deviceState = deviceState
        encapsulated_status.messageCounter = messageCounter
        encapsulated_status.deviceStatus.CopyFrom(status)
        if errorMessage is not None:
            encapsulated_status.errorMessage = errorMessage
        external_msg.status.CopyFrom(encapsulated_status)
        return external_msg

    @staticmethod
    def proto_create_connect(
        sessionId: str, company: str, vehicleName: str, device_list: list[Device]
    ):
        external_msg = ExternalProtocolProtob.ExternalClient()
        connect_msg = ExternalProtocolProtob.Connect()
        connect_msg.sessionId = sessionId
        connect_msg.company = company
        connect_msg.vehicleName = vehicleName
        for device in device_list:
            connect_msg.devices.extend([device.get_protobuf()])
        external_msg.connect.CopyFrom(connect_msg)
        return external_msg

    @staticmethod
    def proto_create_command_response(
        session_id: str, command_response_type, messageCounter: int
    ):
        external_msg = ExternalProtocolProtob.ExternalClient()
        command_response = ExternalProtocolProtob.CommandResponse()
        command_response.sessionId = session_id
        command_response.type = command_response_type
        command_response.messageCounter = messageCounter
        external_msg.commandResponse.CopyFrom(command_response)
        return external_msg

    @staticmethod
    def get_device_from_status(status: InternalProtocolProtob.DeviceStatus) -> Device:
        return Device(status.device)
