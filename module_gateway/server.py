import asyncio
from collections import defaultdict
import logging

from module_gateway.modules import (
    creator_factory,
    check_supported_device_for_module,
    MessageCreator,
    MessageParser,
    ModuleType
)
from module_gateway.response_type import ResponseType
from module_gateway.utils import ClientException

import module_gateway.protobuf.InternalProtocol_pb2 as internal_protocol


class Server:

    MESSAGE_SIZE = 4
    DISCONNECTED = -1

    def __init__(self, client_exc: ClientException) -> None:
        self.client_exc = client_exc
        self.message_parser = MessageParser()
        self.connected_devices = defaultdict(
            lambda: defaultdict(lambda: defaultdict(lambda: Server.DISCONNECTED)))

    async def run_server(self, host: str, port: int) -> None:
        logging.info("Starting server")
        server = await asyncio.start_server(self._handle_client, host, port)
        async with server:
            await server.serve_forever()

    async def _handle_client(self, reader: asyncio.streams.StreamReader, writer: asyncio.streams.StreamWriter) -> None:
        data = await self._receive_message(reader)
        connect = self.message_parser.parse_connect(data)
        response_type = self._check_conditions(connect)
        if self.client_exc is ClientException.CommunicationError:
            response_type = ClientException.CommunicationError
        logging.info(f"New client connected, sending response {response_type}")
        connect_response = MessageCreator.create_connect_response(
            response_type
        )
        if self.client_exc is not ClientException.ServerTookTooLong:
            await self._send_message(writer, connect_response)
        if response_type is not ResponseType.OK:
            writer.close()
            await writer.wait_closed()
            logging.info(
                f"Closing connection with client because {response_type}")
            return

        device = connect.device
        device_name = device.deviceName
        device_module = device.module
        device_type = device.deviceType
        device_role = device.deviceRole
        priority = connect.priority
        message_creator = creator_factory(device_module)
        self.connected_devices[device_module][device_type][device_role] = priority

        while True:
            # check if same device with higher priority was connected
            if self.connected_devices[device_module][device_type][device_role] != priority:
                logging.info(
                    f"Device {device_name} will be disconnected, because device with higher priority was connected"
                )
                break
            data = await self._receive_message(reader)
            if not data:
                break
            logging.info(
                f"Received status from device {device_name}, sending command")
            status = self.message_parser.parse_status(data)
            command = message_creator.create_command(status)
            await self._send_message(writer, command)

        logging.info(f"Closing connection with client {device_name}")
        # check if device was disconnected on his own
        if self.connected_devices[device_module][device_type][device_role] == priority:
            self.connected_devices[device_module][device_type][device_role] = Server.DISCONNECTED
        writer.close()
        await writer.wait_closed()

    async def _receive_message(self, reader: asyncio.streams.StreamReader) -> bytes | None:
        data = (await reader.read(Server.MESSAGE_SIZE))
        if not data:
            return None
        while len(data) < Server.MESSAGE_SIZE:
            data += (await reader.read(Server.MESSAGE_SIZE - len(data)))
        actual_message_size = int.from_bytes(data, 'big')
        data = (await reader.read(actual_message_size))
        while len(data) < actual_message_size:
            data += (await reader.read(actual_message_size - len(data)))
        return data

    def _check_conditions(self, connect: internal_protocol.DeviceConnect) -> ResponseType:
        device = connect.device
        module = device.module
        device_type = device.deviceType
        device_role = device.deviceRole
        priority = connect.priority
        connected_device_priority = self.connected_devices[module][device_type][device_role]
        if connected_device_priority == priority:
            return ResponseType.ALREADY_CONNECTED
        if not ModuleType.has_value(module):
            return ResponseType.MODULE_NOT_SUPPORTED
        if not check_supported_device_for_module(module, device_type):
            return ResponseType.DEVICE_NOT_SUPPORTED
        if connected_device_priority != Server.DISCONNECTED and connected_device_priority < priority:
            return ResponseType.HIGHER_PRIORITY_ALREADY_CONNECTED
        return ResponseType.OK

    async def _send_message(self, writer: asyncio.streams.StreamWriter, message) -> None:
        writer.write(message.ByteSize().to_bytes(
            Server.MESSAGE_SIZE, byteorder='big'))
        writer.write(message.SerializeToString())
        await writer.drain()
