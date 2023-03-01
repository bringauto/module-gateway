import time
from queue import PriorityQueue
from threading import Lock

from . import Event
from module_gateway.external_client.config import Endpoint


class SingletonMeta(type):
    """
    This is a thread-safe implementation of Singleton.
    """

    _instances = {}
    _lock: Lock = Lock()

    def __call__(cls, *args, **kwargs):
        """
        Possible changes to the value of the `__init__` argument do not affect
        the returned instance.
        """
        with cls._lock:
            if cls not in cls._instances:
                instance = super().__call__(*args, **kwargs)
                cls._instances[cls] = instance
        return cls._instances[cls]


class PriorityQueueSingleton(metaclass=SingletonMeta):
    """Singleton priority queue used as global event queue of external client."""

    def __init__(self) -> None:
        self._priority_queue = PriorityQueue()

    def get(self):
        return self._priority_queue.get(block=True, timeout=None)

    def add_status(self, status) -> None:
        self._priority_queue.put(
            (
                Event.Priority.NORMAL,
                time.time(),
                Event(Event.EventType.EV_SEND_STATUS, {"status-to-send": status}),
            )
        )

    def add_disconnect_device(self, status) -> None:
        self._priority_queue.put(
            (
                Event.Priority.NORMAL,
                time.time(),
                Event(Event.EventType.EV_DEVICE_DISCONNECT, {"status-to-send": status}),
            )
        )

    def add_status_response_timeout(self, endpoint: Endpoint) -> None:
        self._priority_queue.put(
            (
                Event.Priority.NORMAL,
                time.time(),
                Event(Event.EventType.EV_STATUS_RESPONSE_TIMEOUT, {"endpoint": endpoint}),
            )
        )

    def add_broker_disconnect(self, endpoint: Endpoint) -> None:
        self._priority_queue.put(
            (
                Event.Priority.NORMAL,
                time.time(),
                Event(Event.EventType.EV_BROKER_DISCONNECT, {"endpoint": endpoint}),
            )
        )

    def add_disconnect(self, endpoint: Endpoint) -> None:
        self._priority_queue.put(
            (
                Event.Priority.HIGH,
                time.time(),
                Event(Event.EventType.EV_DISCONNECT, {"endpoint": endpoint}),
            )
        )

    def add_reconnect(self, endpoint: Endpoint) -> None:
        self._priority_queue.put(
            (
                Event.Priority.NORMAL,
                time.time(),
                Event(Event.EventType.EV_RECONNECT, {"endpoint": endpoint}),
            )
        )

    def add_unexpected_msg(self):
        self._priority_queue.put(
            (Event.Priority.HIGH, time.time(), Event(Event.EventType.EV_UNEXPECTED_MSG, None))
        )

    def add_none(self):
        self._priority_queue.put((Event.Priority.HIGHEST, time.time(), None))
