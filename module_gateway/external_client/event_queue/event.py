from enum import Enum, auto

from module_gateway.external_client.utils import OrderedEnum


class Event:
    """Some type of event handled by external client from global event queue."""

    class EventType(Enum):
        EV_BROKER_DISCONNECT = auto()  # when disconnected from broker
        EV_SEND_STATUS = auto()  # when internal server passes status from device to send
        EV_UNEXPECTED_MSG = auto()
        EV_STATUS_RESPONSE_TIMEOUT = auto()  # when status response times out
        EV_RECONNECT = auto()  # reconnect to endpoint
        EV_DEVICE_DISCONNECT = auto()  # device has disconnected
        EV_DISCONNECT = (
            auto()
        )  # disconnect and destroy endpoint connection (after all devices are disconnected)

    class Priority(OrderedEnum):
        HIGHEST = 1
        HIGH = 2
        NORMAL = 3
        LOW = 4

    def __init__(self, event_type: EventType, values: dict) -> None:
        self._event_type = event_type
        self._values = values

    @property
    def event_type(self) -> EventType:
        return self._event_type

    @property
    def values(self):
        return self._values
