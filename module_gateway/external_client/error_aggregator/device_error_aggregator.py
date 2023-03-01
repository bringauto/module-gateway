import module_gateway.protobuf.InternalProtocol_pb2 as InternalProtocolProtob
from module_gateway.external_client.device import Device


class DeviceErrorAggregator:
    """
    This class represents one concrete device in ErrorAggregator
    """

    def __init__(self, device: Device) -> None:
        self._device = device
        self._statuses: list[InternalProtocolProtob.DeviceStatus] = []

    @property
    def device(self):
        return self._device

    def add_status(self, status: InternalProtocolProtob.DeviceStatus):
        self._statuses.append(status)

    def status_count(self) -> int:
        return len(self._statuses)

    def get_statuses(self) -> list[InternalProtocolProtob.DeviceStatus]:
        return self._statuses

    def get_last_status(self) -> InternalProtocolProtob.DeviceStatus | None:
        try:
            return self._statuses[-1]
        except IndexError:
            return None
