import time
import logging
import threading

from module_gateway.external_client.config import Endpoint
from module_gateway.external_client.event_queue import PriorityQueueSingleton

import module_gateway.protobuf.ExternalProtocol_pb2 as ExternalProtocolProtob
import module_gateway.protobuf.InternalProtocol_pb2 as InternalProtocolProtob

class NotAckedStatus:
    """
    For each sent status instance of this class is created - has its own timer, which indicates,
    after how long error should be raised, if status response does not arrive.
    Raising error means creating event EV_STATUS_RESPONSE_TIMEOUT and adding it to global event queue.
    """

    STATUS_RESPONSE_TIMEOUT = 5  # TODO set to 30

    def __init__(
        self, status: ExternalProtocolProtob.ExternalClient, endpoint: Endpoint
    ) -> None:
        self.timeout_time = time.time() + self.STATUS_RESPONSE_TIMEOUT
        self._status = status
        self._event_queue = PriorityQueueSingleton()
        self._endpoint = endpoint

    @property
    def status(self) -> ExternalProtocolProtob.ExternalClient:
        return self._status

    def get_internal_status(self) -> InternalProtocolProtob.DeviceStatus:
        return self._status.status.deviceStatus

    def start_timer(
        self, response_handled: threading.Event, response_handled_lock: threading.Lock
    ):
        self.timer = threading.Timer(self.STATUS_RESPONSE_TIMEOUT, self.timeout_handler)
        self._response_handled = response_handled
        self._response_handled_lock = response_handled_lock
        self.timer.start()

    def cancel_timer(self):
        self.timer.cancel()
        self.timer.join()

    def timeout_handler(self):
        logging_str = f"Status Response Timeout ({self._status.status.messageCounter}):"
        with self._response_handled_lock:
            # guard against multiple EV_STATUS_RESPONSE_TIMEOUT events.
            if self._response_handled.is_set():
                logging.error(f"{logging_str} already handled, skipping.")
                return
            self._response_handled.set()
        logging.error(f"{logging_str} putting event onto queue.")
        self._event_queue.add_status_response_timeout(self._endpoint)
