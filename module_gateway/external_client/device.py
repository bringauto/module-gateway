import module_gateway.protobuf.InternalProtocol_pb2 as InternalProtocolProtob


class Device:
    """Representation of device"""

    def __init__(self, device_proto: bytes | InternalProtocolProtob.Device) -> None:
        if isinstance(device_proto, InternalProtocolProtob.Device):
            proto = device_proto
        elif isinstance(device_proto, bytes):
            proto = InternalProtocolProtob.Device().FromString(device_proto)
        else:
            raise ValueError("device_proto is not bytes or InternalProtocolProto.Device")
        self.device_name = proto.deviceName
        self.device_type = proto.deviceType
        self.device_role = proto.deviceRole
        self.module = proto.module

    def get_protobuf(self) -> InternalProtocolProtob.Device:
        proto = InternalProtocolProtob.Device()
        proto.module = self.module
        proto.deviceType = self.device_type
        proto.deviceRole = self.device_role
        proto.deviceName = self.device_name
        return proto

    def __eq__(self, __o: object) -> bool:
        if not isinstance(__o, Device):
            return False
        if (
            self.device_name == __o.device_name
            and self.device_type == __o.device_type
            and self.module == __o.module
            and self.device_role == __o.device_role
        ):
            return True
        return False

    def __hash__(self):
        return hash((self.device_name, self.device_type, self.module, self.device_role))

    def __repr__(self) -> str:
        return f"Device({self.device_name}:{self.device_role} [{self.module}])"
