import json

import module_gateway.protobuf.InternalProtocol_pb2 as InternalProtocolProtob
from . import ModuleAggregatorBase


class MissionErrorAggregator(ModuleAggregatorBase):
    @staticmethod
    def create_device_error(statuses: list[InternalProtocolProtob.DeviceStatus]) -> bytes:
        count = len(statuses)
        return json.dumps({"status_count": count}).encode()
