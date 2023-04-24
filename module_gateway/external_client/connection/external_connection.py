import threading
import logging
import random
import string

from module_gateway.external_client.utils import ProtoHelpers
from .connection_channel import (
    ConnectionChannel,
    ConnectionChannelTimeout,
)
from module_gateway.external_client.device import Device
from .messages import SentMessagesHandler
from module_gateway.external_client.error_aggregator import ErrorAggregator
from module_gateway.external_client.ipc_communication import IPCCommunication
from module_gateway.external_client.event_queue import PriorityQueueSingleton

from module_gateway.external_client.config import Endpoint, ModuleConfig

from .exceptions import (
    AlreadyLogged,
    BadSessionId,
    ConnectionSequenceInvalid,
    CommandOutOfOrder,
    ConnectionSequenceNoDevices,
)

import module_gateway.protobuf.ExternalProtocol_pb2 as ExternalProtocolProtob
import module_gateway.protobuf.InternalProtocol_pb2 as InternalProtocolProtob


class ExternalConnection:
    """
    Class representing connection to one external server endpoint (see config.json)
    """

    KEY_LENGHT = 8
    RESPONSE_TIMEOUT = 8

    def __init__(
        self,
        endpoint: Endpoint,
        associated_modules: list[ModuleConfig],
        car_id: str,
        vehicle_name: str,
        company: str,
        ipc_communication: IPCCommunication,
    ) -> None:
        self.message_counter = 0
        self.last_command_counter = None
        self._session_id = ""

        self._car_id = car_id
        self._company = company
        self._vehicle_name = vehicle_name

        self._endpoint = endpoint
        self._associated_modules = associated_modules
        self.io_connection = ConnectionChannel(
            endpoint, f"to-client/{car_id}", f"to-server/{car_id}"
        )
        self._receive_loop_thread = None
        self._sent_messages_handler = SentMessagesHandler(endpoint)
        self.should_stop_event = threading.Event()
        self._is_connected: bool = False
        self._error_aggregator = ErrorAggregator(wanted_modules=self._associated_modules)
        self._ipc_communication = ipc_communication

        self._event_queue = PriorityQueueSingleton()

    @property
    def is_connected(self):
        return self._is_connected

    @is_connected.setter
    def is_connected(self, value: bool):
        self._is_connected = value

    @property
    def sent_messages_handler(self):
        return self._sent_messages_handler

    @property
    def endpoint(self) -> Endpoint:
        return self._endpoint

    def fill_error_aggregator(self, status: InternalProtocolProtob.DeviceStatus):
        """
        Add not acked statuses and status to error aggregator.
        """
        for not_acked_status in self._sent_messages_handler.not_acked_statuses:
            self._error_aggregator.add_status_to_error_aggregator(
                not_acked_status.get_internal_status()
            )
        self._sent_messages_handler.clear_all()
        if status is not None:
            self._error_aggregator.add_status_to_error_aggregator(status)

    def end_connection(
        self, complete_disconnect: bool = False, wait_receive_thread: bool = True
    ) -> None:
        self.sent_messages_handler.clear_all_timers()
        self.sent_messages_handler
        self._is_connected = False
        self.io_connection.stop()
        self.should_stop_event.set()
        if not complete_disconnect:
            self.fill_error_aggregator(None)
        if self._receive_loop_thread is not None and wait_receive_thread:
            self._receive_loop_thread.join()
        self.message_counter = 0
        self.last_command_counter = None

    def send_status(
        self,
        stat: InternalProtocolProtob.DeviceStatus,
        device_state: ExternalProtocolProtob.Status.DeviceState = ExternalProtocolProtob.Status.DeviceState.RUNNING,
        errorMessage: bytes = None,
    ) -> None:
        # check if this is the first status of device to update device_state to CONNECTING
        if device_state == ExternalProtocolProtob.Status.DeviceState.CONNECTING:
            self._sent_messages_handler.add_device_as_connected(stat.device)
        elif device_state == ExternalProtocolProtob.Status.DeviceState.RUNNING:
            if not self._sent_messages_handler.is_device_connected(stat.device):
                device_state = ExternalProtocolProtob.Status.DeviceState.CONNECTING
                self._sent_messages_handler.add_device_as_connected(stat.device)
        elif device_state == ExternalProtocolProtob.Status.DeviceState.DISCONNECT:
            self._sent_messages_handler.delete_device_connected(stat.device)

        encapsulated_status = ProtoHelpers.proto_create_status(
            self._session_id,
            device_state,
            self.get_next_status_counter(),
            stat,
            errorMessage=errorMessage,
        )
        self._sent_messages_handler.add_not_acked_status(encapsulated_status)
        self.io_connection.send_msg(encapsulated_status.SerializeToString())
        logging.debug(
            f"Sending status with messageCounter={encapsulated_status.status.messageCounter}{' with aggregated errorMessage' if errorMessage is not None else ''}"
        )

    def get_next_status_counter(self) -> int:
        """
        Generate next count for status (used for acking)
        """
        self.message_counter += 1
        return self.message_counter

    def _get_command_counter(self, command: ExternalProtocolProtob.ExternalServer) -> int:
        return command.command.messageCounter

    def _connect_msg_handle(self, device_list: list[Device]) -> None:
        """Takes care of first etape of connect sequence - send connect message containing list of devices"""
        self.set_session_id(self._car_id)
        connect_msg = ProtoHelpers.proto_create_connect(
            self._session_id, self._company, self._vehicle_name, device_list
        )

        self.io_connection.send_msg(connect_msg.SerializeToString())
        connect_response_msg_raw = self.io_connection.get_last_msg(self.RESPONSE_TIMEOUT)

        if connect_response_msg_raw is None:
            # see ConectionChannel.stop
            connect_response_msg_raw = self.io_connection.get_last_msg(self.RESPONSE_TIMEOUT)
            while connect_response_msg_raw is None:
                connect_response_msg_raw = self.io_connection.get_last_msg(
                    self.RESPONSE_TIMEOUT
                )

        connect_response = ExternalProtocolProtob.ExternalServer().FromString(
            connect_response_msg_raw
        )
        if connect_response.connectResponse.sessionId != self._session_id:
            raise BadSessionId(self._car_id)
        if (
            connect_response.connectResponse.type
            == ExternalProtocolProtob.ConnectResponse.Type.ALREADY_LOGGED
        ):
            raise AlreadyLogged(self._car_id)

    def _status_msgs_handle(self, device_list: list[Device]) -> None:
        """Takes care of second etape of connect sequence - for all devices send their last status"""
        for device in device_list:
            logging.debug(f"Status for {device}:")
            last_status = self._error_aggregator.get_last_error_status(device)

            if last_status is None:
                logging.debug(
                    f"Device does not have any statuses in error aggregator, adding latest status."
                )
                last_status = self._ipc_communication.get_last_device_status(device)
                last_status = last_status.deviceStatus
                self._error_aggregator.add_status_to_error_aggregator(last_status)

            # error aggregation
            dev_error = self._error_aggregator.get_error(device)

            self.send_status(
                last_status,
                device_state=ExternalProtocolProtob.Status.DeviceState.CONNECTING,
                errorMessage=dev_error,
            )

        for _ in range(len(device_list)):
            raw_status_response = self.io_connection.get_last_msg(self.RESPONSE_TIMEOUT)
            status_response = ExternalProtocolProtob.ExternalServer().FromString(
                raw_status_response
            )
            self._sent_messages_handler.acknowledge_status(status_response)
            logging.debug(
                f"Acknowledged status response with messasgeCounter={status_response.statusResponse.messageCounter}"
            )

        if not self._sent_messages_handler.all_statuses_acked():
            # this will happen just if there is some extra message in connect sequence
            raise ConnectionChannelTimeout("Not all statuses from connect sequence acked!")

    def _command_msgs_handle(self, dev_list) -> None:
        """Takes care of third etape of connect sequence - receive commands for all devices and pass them to devices."""
        for _ in range(len(dev_list)):
            external_msg_raw = self.io_connection.get_last_msg(self.RESPONSE_TIMEOUT)
            external_msg = ExternalProtocolProtob.ExternalServer().FromString(external_msg_raw)
            self._handle_command(external_msg)

    def _handle_command(self, external_command: ExternalProtocolProtob.ExternalServer) -> None:
        command = external_command.command
        logging.debug(f"Handling COMMAND messageCounter={command.messageCounter}")

        if self.last_command_counter is not None:
            if self.last_command_counter + 1 != command.messageCounter:
                # out of order command received
                raise CommandOutOfOrder(
                    f"expected {self.last_command_counter}, got {command.messageCounter}"
                )

        self.last_command_counter = command.messageCounter
        if self.sent_messages_handler.is_device_connected(command.deviceCommand.device):
            response_type = ExternalProtocolProtob.CommandResponse.Type.OK
            self._ipc_communication.pass_command(command)
        else:
            response_type = ExternalProtocolProtob.CommandResponse.Type.DEVICE_NOT_CONNECTED

        command_response = ProtoHelpers.proto_create_command_response(
            self._session_id, response_type, self.last_command_counter
        )

        self.io_connection.send_msg(command_response.SerializeToString())
        logging.debug(
            f"Sending command response with messageCounter={command_response.commandResponse.messageCounter}"
        )

    def _receiving_handler_loop(self):
        """
        This loop is started after successfull connect sequence in own thread.
        Loop is receiving messages from external server and processes them.
        """
        while not self.should_stop_event.is_set():
            raw_message = self.io_connection.get_last_msg(None)
            if raw_message is None:
                return
            external_server_message = ExternalProtocolProtob.ExternalServer().FromString(
                raw_message
            )
            if external_server_message.HasField("command"):
                try:
                    self._handle_command(external_server_message)
                except CommandOutOfOrder as e:
                    logging.error(
                        f"[bold red]Out of order command received[/]({e}), will reconnect.",
                        extra={"markup": True},
                    )
                    self.end_connection(wait_receive_thread=False)
                    self._event_queue.add_reconnect(self._endpoint)

            elif external_server_message.HasField("statusResponse"):
                logging.debug(
                    f"Handling STATUS_RESPONSE messageCounter={external_server_message.statusResponse.messageCounter}"
                )
                self._sent_messages_handler.acknowledge_status(external_server_message)

    def _start_infinite_receive_loop(self) -> None:
        self._receive_loop_thread = threading.Thread(
            target=self._receiving_handler_loop, daemon=False
        )
        self._receive_loop_thread.start()

    def init_connection(self) -> None:
        """
        Handles all etapes of connect sequence. If connect sequence is successful,
        infinite receive loop is started in new thread.
        """
        logging.info(
            f"Initializing connection to endpoint {self._endpoint} for modules: {self._associated_modules}"
        )
        self.io_connection.start()
        self.should_stop_event.clear()

        # create device list (connected devices plus devices in error aggregator)
        dev_list: set[Device] = set()
        for module in self._associated_modules:
            module_devices = self._ipc_communication.get_all_devices(module.module_id)
            for device in module_devices:
                dev_list.add(device)
        dev_list |= self._error_aggregator.get_all_registered_devices_with_not_empty_aggregators()
        dev_list = list(dev_list)

        if not dev_list:
            logging.warning(
                f"Init connection when no device is connected nor available in error aggregator, skipping."
            )
            raise ConnectionSequenceNoDevices("No devices connected or in error aggregator.")

        try:
            logging.info(
                "[bold yellow]Connect sequence[/]: 1st step (sending list of devices)",
                extra={"markup": True},
            )
            self._connect_msg_handle(dev_list)
            logging.info("[bold]1st step[/] [green]done[/]", extra={"markup": True})

            logging.info(
                "[bold yellow]Connect sequence[/]: 2nd step (sending statuses of all connected devices)",
                extra={"markup": True},
            )
            self._status_msgs_handle(dev_list)
            logging.info("[bold]2nd step[/] [green]done[/]", extra={"markup": True})

            logging.info(
                "[bold yellow]Connect sequence[/]: 3rd step (receiving commands for devices in previous step)",
                extra={"markup": True},
            )
            self._command_msgs_handle(dev_list)
            logging.info("[bold]3rd step[/] [green]done[/]", extra={"markup": True})
        except ConnectionChannelTimeout as e:
            raise ConnectionSequenceInvalid("Connection sequence timed out")
        except CommandOutOfOrder as e:
            logging.error(
                f"[bold red]Out of order command received[/] ({e})", extra={"markup": True}
            )
            raise ConnectionSequenceInvalid("Out of order command received")

        self._start_infinite_receive_loop()
        self._is_connected = True

        # clear error aggregator after successful connection
        self._error_aggregator.clear_error_aggregator()
        logging.info(
            "[bold yellow]Connection sequence[/] [bold green]successful[/]",
            extra={"markup": True},
        )

    def set_session_id(self, car_id: str) -> None:
        self._session_id = self._generate_session_id(car_id)

    def _generate_session_id(self, car_id: str) -> str:
        letters = string.ascii_letters + string.digits
        result_str = "".join(
            random.choice(letters) for _ in range(ExternalConnection.KEY_LENGHT)
        )
        return car_id + result_str

    def has_any_device_connected(self) -> bool:
        return self._sent_messages_handler.is_any_device_connected()
