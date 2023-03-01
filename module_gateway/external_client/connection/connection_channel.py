import queue
import logging

import paho.mqtt.client as mqtt

from module_gateway.external_client.config import Endpoint
from module_gateway.external_client.event_queue import PriorityQueueSingleton


class ConnectionChannelTimeout(Exception):
    """Message was not received after specified timeout"""
    pass


class ConnectionChannel:
    """Class representing connection channel - MQTT in this case."""

    KEEP_ALIVE = 60

    def __init__(self, endpoint: Endpoint, sub_topic: str, pub_topic: str) -> None:
        self._endpoint = endpoint
        self._sub_topic = sub_topic
        self._pub_topic = pub_topic
        self.client = mqtt.Client(protocol=mqtt.MQTTv5, transport="tcp")
        self.rec_msgs_queue = queue.Queue()
        self._event_queue = PriorityQueueSingleton()

    def is_connected(self):
        return self.client.is_connected()

    def _on_disconnect(self, _client, _userdata, _rc, _properties):
        logging.info(f"ConnectionChannel ({self._endpoint}) disconnected from broker.")
        self._event_queue.add_broker_disconnect(self._endpoint)

    def _on_message(self, _client, _userdata, msg):
        self.rec_msgs_queue.put(msg.payload, block=False, timeout=None)

    def start(self):
        self.client.on_message = self._on_message
        self.client.on_disconnect = self._on_disconnect
        self.client.connect(self._endpoint.ip_addr, self._endpoint.port, self.KEEP_ALIVE)
        self.client.subscribe(self._sub_topic)
        self.client.loop_start()

    def stop(self):
        self.client.loop_stop()
        self.rec_msgs_queue.put(None)

    def send_msg(self, msg: str) -> None:
        self.client.publish(self._pub_topic, msg)

    def get_last_msg(self, timeout: int) -> str:
        try:
            return self.rec_msgs_queue.get(block=True, timeout=timeout)
        except queue.Empty:
            raise ConnectionChannelTimeout(
                f"No item available in queue after timeout ({timeout})"
            )
