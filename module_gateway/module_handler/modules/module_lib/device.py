import module_gateway.protobuf.InternalProtocol_pb2 as ip


class Device:
    def __init__(self, device_name: str, device_type: int, device_role: str, module_id: int = 0):
        self.device_name = device_name
        self.device_type = device_type
        self.device_role = device_role
        self.module_id = module_id

        self.aggregated_message = None
        self.ready_to_sent = []

        self.current_command = None

    def clear_device(self):
        self.aggregated_message = None
        self.ready_to_sent.clear()
        self.current_command = None

    def pop_oldest_ready_status(self):
        try:
            oldest_status = self.ready_to_sent.pop(0)
        except IndexError:
            return None
        return oldest_status

    def get_device_protobuf(self) -> bytes:
        device_msg = ip.Device(
            deviceName=self.device_name,
            deviceType=self.device_type,
            deviceRole=self.device_role,
            module=self.module_id
        )
        return device_msg.SerializeToString()
