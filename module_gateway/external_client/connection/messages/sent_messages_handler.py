import logging
from threading import Event, Lock

import module_gateway.protobuf.ExternalProtocol_pb2 as ExternalProtocolProtob
import module_gateway.protobuf.InternalProtocol_pb2 as InternalProtocolProtob

from module_gateway.external_client.config import Endpoint
from .not_acked_status import NotAckedStatus


class SentMessagesHandler:
    """
    This class stores all sent statuses, that were not acknowledged.
    Status is acknowledged, when status response arrives.
    """

    def __init__(self, endpoint: Endpoint) -> None:
        self._not_acked_statuses: list[NotAckedStatus] = []
        self._endpoint = endpoint

        # set of devices which are already connected (used for determining device_state)
        self._connected_devices: set[bytes] = set()

        # response timeout guard
        self._response_handled = Event()
        self._response_handled_lock = Lock()

    @property
    def not_acked_statuses(self):
        return self._not_acked_statuses

    def add_not_acked_status(self, status: ExternalProtocolProtob.ExternalClient):
        """call this method for each sent status - will add status as not acknowledged"""
        not_acked_status = NotAckedStatus(status, self._endpoint)
        not_acked_status.start_timer(
            response_handled=self._response_handled,
            response_handled_lock=self._response_handled_lock
        )
        self._not_acked_statuses.append(not_acked_status)

    def acknowledge_status(self, status_response: ExternalProtocolProtob.ExternalServer):
        """This is called, when status response arrives"""
        stat_resp_counter = self.get_status_response_counter(status_response)
        for i in range(len(self._not_acked_statuses)):
            stat_counter = self.get_status_counter(self._not_acked_statuses[i].status)
            if stat_counter == stat_resp_counter:
                not_acked_status = self._not_acked_statuses.pop(i)
                not_acked_status.cancel_timer()
                return
        raise Exception("Server acknowledged status, that was not sent!")

    def all_statuses_acked(self):
        if len(self._not_acked_statuses) == 0:
            return True
        return False

    def get_status_counter(self, status: ExternalProtocolProtob.ExternalClient):
        return status.status.messageCounter

    def get_status_response_counter(self, status_response: ExternalProtocolProtob.ExternalServer):
        return status_response.statusResponse.messageCounter

    def clear_all(self):
        self.clear_all_timers()
        self._not_acked_statuses.clear()

    def clear_all_timers(self):
        for not_acked_status in self.not_acked_statuses:
            not_acked_status.cancel_timer()
        self._response_handled.clear()

    def add_device_as_connected(self, device: InternalProtocolProtob.Device) -> None:
        self._connected_devices.add(device.SerializeToString())

    def delete_device_connected(self, device: InternalProtocolProtob.Device) -> None:
        try:
            self._connected_devices.remove(device.SerializeToString())
        except KeyError:
            logging.warning(
                "SentMessageHandler: tried to delete connected device which does not exist, ignoring."
            )

    def is_device_connected(self, device: InternalProtocolProtob.Device) -> bool:
        return device.SerializeToString() in self._connected_devices

    def is_any_device_connected(self) -> bool:
        return bool(self._connected_devices)
