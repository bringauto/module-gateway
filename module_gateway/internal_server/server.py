import asyncio
from collections import defaultdict
from dataclasses import dataclass
import logging
import pickle

import asyncio_mqtt as aiomqtt
import paho.mqtt.client as mqtt

from .modules import (
    create_connect_response,
    check_supported_device_for_module,
    ModuleType
)
from .response_type import ResponseType
from .utils import ClientException
import module_gateway.protobuf.InternalProtocol_pb2 as internal_protocol


class Server:

    MESSAGE_SIZE = 4
    DISCONNECTED = -1

    def __init__(self, client_exc: ClientException) -> None:
        self.client_exc = client_exc
        self.mqtt_client = mqtt.Client(protocol=mqtt.MQTTv5, transport='tcp')
        self.connected_devices = defaultdict(
            lambda: defaultdict(lambda: defaultdict(
                lambda: DeviceItem())
            )
        )

    async def listen(self, mqtt_host: str, mqtt_port: int) -> None:
        async with aiomqtt.Client(mqtt_host, mqtt_port) as client:
            async with client.messages() as messages:
                await client.subscribe("to-internal/#")
                async for message in messages:
                    _, d_module, d_type, d_role = message.topic.value.split('/')
                    d_module = int(d_module)
                    d_type = int(d_type)

                    if self.connected_devices[d_module][d_type][d_role].priority == Server.DISCONNECTED:
                        logging.warning(f'Message for not connected device: {d_module}/{d_type}/{d_role}\
                                          has been received')
                        continue
                    await self.connected_devices[d_module][d_type][d_role].queue.put(message.payload)
                    logging.info(f"Message for unique device: {d_module}/{d_type}/{d_role} has been received")

    async def run_server(self, host: str, port: int, mqtt_host: str, mqtt_port: int) -> None:
        logging.info("Starting server")
        self.mqtt_client.connect(mqtt_host, mqtt_port)
        self.mqtt_client.loop_start()
        server = await asyncio.start_server(self._handle_client, host, port)
        async with server:
            await server.serve_forever()

    async def _handle_client(self, reader: asyncio.streams.StreamReader, writer: asyncio.streams.StreamWriter) -> None:
        data = await self._receive_message(reader)
        connect = internal_protocol.DeviceConnect.FromString(data)
        try:
            await self._init_sequence(connect, writer)
        except Exception:
            return

        device = connect.device
        d_name = device.deviceName
        d_module = device.module
        d_type = device.deviceType
        d_role = device.deviceRole
        priority = device.priority
        self.connected_devices[d_module][d_type][d_role].priority = priority
        queue = self.connected_devices[d_module][d_type][d_role].queue
        new_device = True

        while True:
            # check if same device with higher priority was connected
            if self.connected_devices[d_module][d_type][d_role].priority != priority:
                logging.info(
                    f"Device {d_name} will be disconnected, because device with higher priority was connected"
                )
                break
            data = await self._receive_message(reader)
            if not data:
                break
            logging.info(f"Received status from device {d_name}, sending command")
            self.mqtt_client.publish('from-internal', pickle.dumps((new_device, data)))
            new_device = False
            command = await queue.get()
            disconnect, command = pickle.loads(command)
            if disconnect:
                logging.error(f"Device {d_name} will be disconnected, because device has sent corupted data")
                break
            await self._send_message(writer, command)
            logging.info(f"Message for device {d_name} has been sent")

        logging.info(f"Closing connection with client {d_name}")
        # check if device was disconnected on his own
        if self.connected_devices[d_module][d_type][d_role].priority == priority:
            self.connected_devices[d_module][d_type][d_role].priority = Server.DISCONNECTED
            self.mqtt_client.publish('device-disconnect', device.SerializeToString())
        writer.close()
        await writer.wait_closed()

    async def _receive_message(self, reader: asyncio.streams.StreamReader) -> bytes | None:
        data = (await reader.read(Server.MESSAGE_SIZE))
        if not data:
            return None
        while len(data) < Server.MESSAGE_SIZE:
            data += (await reader.read(Server.MESSAGE_SIZE - len(data)))
        actual_message_size = int.from_bytes(data, 'little')
        data = (await reader.read(actual_message_size))
        while len(data) < actual_message_size:
            data += (await reader.read(actual_message_size - len(data)))
        return data

    async def _init_sequence(self, connect: internal_protocol.DeviceConnect,
                             writer: asyncio.streams.StreamWriter) -> None:
        response_type = self._check_conditions(connect)
        if self.client_exc is ClientException.CommunicationError:
            response_type = ClientException.CommunicationError
        logging.info(f"New client connected, sending response {response_type}")

        if self.client_exc is not ClientException.ServerTookTooLong:
            await self._send_message(writer, create_connect_response(response_type).SerializeToString())
        if response_type is not ResponseType.OK:
            writer.close()
            await writer.wait_closed()
            logging.warning(f"Closing connection with client because {response_type}")
            raise Exception()

    def _check_conditions(self, connect: internal_protocol.DeviceConnect) -> ResponseType:
        device = connect.device
        module = device.module
        d_type = device.deviceType
        d_role = device.deviceRole
        priority = device.priority
        connected_device_priority = self.connected_devices[module][d_type][d_role].priority
        if connected_device_priority == priority:
            return ResponseType.ALREADY_CONNECTED
        if not ModuleType.has_value(module):
            return ResponseType.MODULE_NOT_SUPPORTED
        if not check_supported_device_for_module(module, d_type):
            return ResponseType.DEVICE_NOT_SUPPORTED
        if connected_device_priority != Server.DISCONNECTED and connected_device_priority < priority:
            return ResponseType.HIGHER_PRIORITY_ALREADY_CONNECTED
        return ResponseType.OK

    async def _send_message(self, writer: asyncio.streams.StreamWriter, message) -> None:
        writer.write(len(message).to_bytes(Server.MESSAGE_SIZE, byteorder='little'))
        writer.write(message)
        await writer.drain()


@dataclass
class DeviceItem:
    priority: int = Server.DISCONNECTED
    queue: asyncio.Queue = asyncio.Queue()
