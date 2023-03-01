import json

import module_gateway.protobuf.InternalProtocol_pb2 as InternalProtocolProtob
from . import ModuleAggregatorBase


class CarAccessoryErrorAggregator(ModuleAggregatorBase):
    @staticmethod
    def create_device_error(statuses: list[InternalProtocolProtob.DeviceStatus]) -> bytes:
        pressed_count = 0
        for status in statuses:
            try:
                if json.loads(status.statusData)["pressed"]:
                    pressed_count += 1
            except KeyError:
                return json.dumps(
                    {"error": "CarAccessoryErrorAggregator, found not supported status."}
                ).encode()

        return json.dumps(
            {
                "pressed_count": pressed_count,
                "not_pressed_count": len(statuses) - pressed_count,
                "total_statuses": len(statuses),
            }
        ).encode()
