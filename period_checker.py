import logging
import textwrap
import argparse
import os
import asyncio
from typing import Callable

from rich.logging import RichHandler
import asyncio_mqtt as aiomqtt

import module_gateway.protobuf.InternalProtocol_pb2 as ip


class Timer:
    def __init__(self, timeout, callback):
        self._timeout = timeout
        self._callback = callback
        self._task = asyncio.ensure_future(self._job())

    async def _job(self) -> None:
        await asyncio.sleep(self._timeout)
        await self._callback()
        self.restart()

    def cancel(self) -> None:
        self._task.cancel()

    def restart(self) -> None:
        self._task.cancel()
        self._task = asyncio.ensure_future(self._job())


def callaback_factory(mqtt_client, msg_payload) -> Callable:
    async def callback() -> None:
        logging.info(f"Status timeout {msg_payload.decode()}")
        await mqtt_client.publish("force-status-send", msg_payload)

    return callback


class PeriodChecker:
    def __init__(self, mqtt_url: str, mqtt_port: int, period_s: float = 5.0):
        self.mqtt_url = mqtt_url
        self.mqtt_port = mqtt_port
        self.period_s = period_s

        self.timers: dict[str, Timer] = {}

    @classmethod
    def parse_device(cls, payload: bytes, message_is_deviceStatus: bool = False) -> str:
        if message_is_deviceStatus:
            status = ip.DeviceStatus.FromString(payload)
            device = status.device
        else:
            device = ip.Device.FromString(payload)
        return f"{device.module}:{device.deviceType}:{device.deviceRole}"

    def handle_send_status(self, client: aiomqtt.Client, payload: bytes) -> None:
        device_str = self.parse_device(payload, message_is_deviceStatus=True)
        if device_str in self.timers:
            logging.info(f"Restarting {device_str} timer.")
            self.timers[device_str].restart()
        else:
            logging.info(f"Adding {device_str} timer.")
            status = ip.DeviceStatus.FromString(payload)
            device_payload = status.device.SerializeToString()
            self.timers[device_str] = Timer(
                self.period_s, callaback_factory(client, device_payload)
            )

    def handle_device_disconnect(self, _client: aiomqtt.Client, payload: bytes) -> None:
        device_str = self.parse_device(payload, message_is_deviceStatus=False)
        if device_str in self.timers:
            logging.info(f"Device {device_str} disconnected, removing timer.")
            self.timers[device_str].cancel()
            del self.timers[device_str]

    async def start(self):
        async with aiomqtt.Client(
            hostname=self.mqtt_url, port=self.mqtt_port
        ) as client:
            logging.info("Client for period checker ready.")
            async with client.messages() as messages:
                await client.subscribe("send-status")
                await client.subscribe("device-disconnect")
                async for message in messages:
                    if message.topic.matches("send-status"):
                        self.handle_send_status(client, message.payload)
                    elif message.topic.matches("device-disconnect"):
                        self.handle_device_disconnect(client, message.payload)


if __name__ == "__main__":
    logging.basicConfig(level=logging.DEBUG, format="%(message)s", handlers=(RichHandler(),))
    mqtt_host = os.getenv("IPC_MQTT_HOST", "localhost")
    try:
        mqtt_port = int(os.getenv("IPC_MQTT_PORT", 1883))
    except ValueError:
        logging.error(f"Invalid IPC_MQTT_PORT, expected int, got {os.getenv('IPC_MQTT_PORT')}")

    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=textwrap.dedent(
            """
        Periodic status checker.
        If device does not send status in specified period, signal for force status is sent.
        Ipc MQTT can be configured by environment variables:
            - IPC_MQTT_HOST (default localhost)
            - IPC_MQTT_PORT (default 1883)
        """
        ),
    )
    parser.add_argument(
        "-p",
        "--period",
        type=float,
        default=5.0,
        help="Minimum period of status sending for devices",
    )
    args = parser.parse_args()

    period_checker = PeriodChecker(mqtt_host, mqtt_port, args.period)
    try:
        asyncio.run(period_checker.start())
    except KeyboardInterrupt:
        logging.info("Bye bye")
