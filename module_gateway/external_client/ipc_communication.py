from queue import Queue
import time
import pickle
import logging

import paho.mqtt.client as mqtt

from module_gateway.external_client.config import Endpoint
from module_gateway.external_client.device import Device
from module_gateway.external_client.event_queue import PriorityQueueSingleton

import module_gateway.protobuf.InternalProtocol_pb2 as InternalProtocolProtob
import module_gateway.protobuf.ExternalProtocol_pb2 as ExternalProtocolProtob


class IPCCommunication:
    """This class takes care of inter process communication with module handler via mqtt"""

    ALL_DEVICES_ASK_TOPIC = "get-all-devices"
    ALL_DEVICES_ANSWER_TOPIC = "all-devices"
    LAST_DEVICE_STATUS_ASK_TOPIC = "get-last-device-status"
    LAST_DEVICE_STATUS_ANSWER_TOPIC = "last-device-status"
    PASS_COMMAND_TOPIC = "pass-command"
    SEND_STATUS_TOPIC = "send-status"
    DISCONNECT_TOPIC = "exc-device-disconnect"

    def __init__(self, endpoint: Endpoint) -> None:
        self._endpoint = endpoint
        self._all_devices_queue = Queue()
        self._last_status_queue = Queue()
        self._event_queue = PriorityQueueSingleton()
        self.client = mqtt.Client(protocol=mqtt.MQTTv5, transport="tcp")

    def _on_connect(self, client, _userdata, _flags, _rc, _properties) -> None:
        client.subscribe(self.SEND_STATUS_TOPIC)
        client.subscribe(self.ALL_DEVICES_ANSWER_TOPIC)
        client.subscribe(self.LAST_DEVICE_STATUS_ANSWER_TOPIC)
        client.subscribe(self.DISCONNECT_TOPIC)

    def _on_message(self, _client, _userdata, msg) -> None:
        if msg.topic == self.SEND_STATUS_TOPIC:
            self._event_queue.add_status(msg.payload)
        elif msg.topic == self.ALL_DEVICES_ANSWER_TOPIC:
            self._all_devices_queue.put(pickle.loads(msg.payload))
        elif msg.topic == self.LAST_DEVICE_STATUS_ANSWER_TOPIC:
            self._last_status_queue.put(msg.payload)
        elif msg.topic == self.DISCONNECT_TOPIC:
            self._event_queue.add_disconnect_device(msg.payload)
        else:
            self._event_queue.add_unexpected_msg()

    def start(self) -> None:
        self.client.on_connect = self._on_connect
        self.client.on_message = self._on_message
        while not self.client.is_connected():
            try:
                self.client.connect(self._endpoint.ip_addr, self._endpoint.port, 30)
                break
            except ConnectionRefusedError:
                logging.error("IPC mqtt unable to connect, trying again")
                time.sleep(0.5)
        logging.info(
            "[bold green]Connection to IPC MQTT broker was succesfull[/]",
            extra={"markup": True},
        )
        self.client.loop_start()

    def stop(self) -> None:
        self.client.loop_stop()
        self._event_queue.add_none()  # to wake up waiting for queue

    def send_msg(self, topic: str, msg: str) -> None:
        self.client.publish(topic, msg)

    def get_all_devices(self, module_id: int) -> list[Device]:
        self.send_msg(self.ALL_DEVICES_ASK_TOPIC, pickle.dumps(module_id))
        raw_devices = self._all_devices_queue.get()
        dev_list = []
        if raw_devices is None:
            return dev_list
        for raw_device in raw_devices:
            new_device = Device(raw_device)
            dev_list.append(new_device)
        return dev_list

    def get_last_device_status(self, device: Device):
        self.send_msg(
            self.LAST_DEVICE_STATUS_ASK_TOPIC, device.get_protobuf().SerializeToString()
        )
        raw_status = self._last_status_queue.get()
        status = InternalProtocolProtob.InternalClient().FromString(raw_status)
        return status

    def pass_command(self, command: ExternalProtocolProtob.Command):
        self.send_msg(self.PASS_COMMAND_TOPIC, command.SerializeToString())
