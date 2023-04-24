import signal
import logging
import sys
import threading

import module_gateway.protobuf.InternalProtocol_pb2 as InternalProtocolProtob
from module_gateway.external_client.ipc_communication import IPCCommunication

from module_gateway.external_client.config import Config, Endpoint

from .connection import ExternalConnection

from .connection.exceptions import (
    BadSessionId,
    AlreadyLogged,
    ConnectionSequenceInvalid,
    ConnectionSequenceNoDevices,
)

from module_gateway.external_client.event_queue import Event, PriorityQueueSingleton
from .utils import rabbit


class ExternalClient:
    """
    This class represents external client. It manages ExternalConnections to specified endpoints and handles global event queue.
    One connection == one endpoint.
    """

    RECONNECT_TIME = 3

    def __init__(self, ipc_host: str, ipc_port: int) -> None:
        self.connection_list: list[ExternalConnection] = []
        self._reconnect_timers: dict[Endpoint, threading.Timer | None] = {}
        self.config = Config("./config/config.json")
        self._event_queue = PriorityQueueSingleton()
        self.ipc_communication = IPCCommunication(Endpoint(ipc_host, ipc_port))

    def stop_client(self) -> None:
        self.ipc_communication.stop()
        for connection in self.connection_list:
            connection.end_connection()

    def signal_handler(self, _sig, _frame) -> None:
        logging.info("Trying to exit...")
        logging.info(rabbit)
        self.stop_client()
        sys.exit()

    def start(self):
        """Main infinite client loop, takes events from global queue and processes them"""
        signal.signal(signal.SIGINT, self.signal_handler)
        self.ipc_communication.start()
        while True:
            new_event: Event = self._event_queue.get()[-1]
            if new_event is None:
                return
            logging.info(f"Handling event: {new_event.event_type.name}")
            if new_event.event_type == Event.EventType.EV_SEND_STATUS:
                internal_client_msg = InternalProtocolProtob.InternalClient.FromString(new_event.values["status-to-send"])
                status_proto = internal_client_msg.deviceStatus
                self.handle_device_status(status_proto)
            elif (
                new_event.event_type == Event.EventType.EV_STATUS_RESPONSE_TIMEOUT
                or new_event.event_type == Event.EventType.EV_BROKER_DISCONNECT
                or new_event.event_type == Event.EventType.EV_RECONNECT
            ):
                connection = self.get_connection(new_event.values["endpoint"])
                if connection is None:
                    logging.warning(
                        "Attempted to reconnect on destroyed connection, skipping."
                    )
                    continue
                connection.end_connection()
                self._connect_to_endpoint(new_event.values["endpoint"])
            elif new_event.event_type == Event.EventType.EV_DEVICE_DISCONNECT:
                internal_client_msg = InternalProtocolProtob.InternalClient.FromString(new_event.values["status-to-send"])
                status_proto = internal_client_msg.deviceStatus
                self.handle_device_status(status_proto)
            elif new_event.event_type == Event.EventType.EV_DISCONNECT:
                connection = self.get_connection(new_event.values["endpoint"])
                if connection is None:
                    logging.error(
                        "[bold red]Internal error[/]: attempted to disconnect on destroyed connection, skipping.",
                        extra={"markup": True},
                    )
                    continue
                self._remove_connection(connection.endpoint)
            else:
                logging.warning(f"Event on event queue not known, skipping.")

    def get_connection(self, endpoint: Endpoint) -> ExternalConnection | None:
        for connection in self.connection_list:
            if connection.endpoint == endpoint:
                return connection
        return None

    def _get_connection_from_status(
        self, stat: InternalProtocolProtob.DeviceStatus
    ) -> ExternalConnection:
        module_id = self._get_module_id_from_status(stat)
        endpoint = self.config.get_endpoint_for_module_id(module_id)
        if endpoint is None:
            raise Exception(f"No endpoint defined for module with id {module_id}")
        return self.get_connection(endpoint)

    def _get_module_id_from_status(self, stat: InternalProtocolProtob.DeviceStatus):
        return stat.device.module

    def handle_device_status(self, stat: InternalProtocolProtob.DeviceStatus) -> None:
        """
        Send status to endpoint, which is defined in json config.
        It is sending via ExternalConnection (one endpoint == one ExternalConnection).
        Device is extracted from status. If there is no connection to desired endpoint, connect sequence is started.

        If was ExternalConnection disconnects, status message is passed to error aggregator.
        """
        module_id = self._get_module_id_from_status(stat)
        endpoint = self.config.get_endpoint_for_module_id(module_id)
        if endpoint is None:
            raise Exception(f"No endpoint defined for module with id {module_id}")
        connection = self.get_connection(endpoint)
        first_connecting = False
        if connection is None:
            connection = self._add_connection(endpoint)
            first_connecting = True
        if not connection.is_connected:
            if first_connecting:
                self._connect_to_endpoint(endpoint)
            else:
                connection.fill_error_aggregator(stat)
        else:
            connection.send_status(stat)

    def _add_connection(self, endpoint: Endpoint) -> ExternalConnection:
        new_connection = ExternalConnection(
            endpoint,
            self.config.get_modules_with_endpoint(endpoint),
            self.config.car_id,
            self.config.vehicle_name,
            self.config.company,
            self.ipc_communication,
        )
        self.connection_list.append(new_connection)
        return new_connection

    def _remove_connection(self, endpoint: Endpoint) -> None:
        if endpoint in self._reconnect_timers:
            timer = self._reconnect_timers[endpoint]
            if timer is not None:
                timer.cancel()
            del self._reconnect_timers[endpoint]

        connection = self.get_connection(endpoint)
        if connection is not None:
            connection.end_connection(complete_disconnect=True)
            self.connection_list.remove(connection)

    def _connect_to_endpoint(self, endpoint: Endpoint) -> None:
        """
        this method executes connect sequence defined in protocol.
        If fails, event EV_RECONNECT is added to global event queue after some time
        """
        connection = self.get_connection(endpoint)
        if endpoint in self._reconnect_timers:
            timer = self._reconnect_timers[endpoint]
            if timer is not None:
                timer.cancel()
                self._reconnect_timers[endpoint] = None
        try:
            connection.init_connection()
        except (
            ConnectionRefusedError,
            ConnectionSequenceInvalid,
            BadSessionId,
        ) as conn_refused:
            logging.error(
                f"[bold yellow]Connect sequence[/] [bold red]failed[/] to endpoint: {endpoint} error: {conn_refused}",
                extra={"markup": True},
            )
            self._add_reconnect_timer(endpoint, self.RECONNECT_TIME)
        except AlreadyLogged as e:
            logging.info(
                f"[bold red]Connection to endpoint {endpoint} failed [/]({e})",
                extra={"markup": True},
            )
            self._add_reconnect_timer(endpoint, self.RECONNECT_TIME * 5)
        except ConnectionSequenceNoDevices:
            logging.debug(
                f"[bold red]Connection sequence no devices[/], completely destroying connection",
                extra={"markup": True},
            )
            self._event_queue.add_disconnect(endpoint)

    def _add_reconnect_timer(self, endpoint: Endpoint, reconnect_time: float) -> None:
        new_timer = threading.Timer(reconnect_time, self._reconnect_callback, (endpoint,))
        logging.info(f"Trying again in {reconnect_time} seconds...")
        self._reconnect_timers[endpoint] = new_timer
        self._reconnect_timers[endpoint].start()

    def _reconnect_callback(self, endpoint):
        self._event_queue.add_reconnect(endpoint)
