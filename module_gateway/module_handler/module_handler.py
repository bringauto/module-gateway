import logging
import pickle

import paho.mqtt.client as mqtt

import module_gateway.protobuf.InternalProtocol_pb2 as ip
import module_gateway.protobuf.ExternalProtocol_pb2 as ep
from .modules_manager import ModulesManager


class ModuleMQTTHandler:
    """Handler of IPC communication using MQTT"""

    def __init__(self, mqtt_url, mqtt_port):
        self.mqtt_port = mqtt_port
        self.mqtt_url = mqtt_url

        self.module_manager = ModulesManager()

    def on_from_internal(self, client, payload):
        is_new_device, message = pickle.loads(payload)
        internal_client_msg = ip.InternalClient.FromString(message)
        status = internal_client_msg.deviceStatus
        dev = status.device

        DeviceCommandB, status_ready_to_sent = self.module_manager.handle_status(
            is_new_device, status)

        to_internal_topic = f"to-internal/{dev.module}/{dev.deviceType}/{dev.deviceRole}"
        device_str = f"(module={dev.module}, role={dev.deviceRole}, name={dev.deviceName})"

        if DeviceCommandB is None:
            logging.info(f"Disconnecting device: {device_str}")
            payload = pickle.dumps((True, b""))
        else:
            logging.info(
                f"Sending command for device {device_str} to topic: {to_internal_topic}")
            payload = pickle.dumps((False, DeviceCommandB))

        client.publish(
            topic=to_internal_topic,
            payload=payload
        )

        if DeviceCommandB is not None and status_ready_to_sent:
            self.send_all_device_statuses(client, dev)

    def send_all_device_statuses(self, client, device: ip.Device):
        topic = "send-status"
        statuses = self.module_manager.get_all_device_statuses(device)
        if statuses is not None:
            for status in statuses:
                logging.info(f"Sending status for device {device}")
                client.publish(
                    topic=topic,
                    payload=status
                )

    def on_get_all_devices(self, client, payload):
        module_id = pickle.loads(payload)
        logging.info(f"Got get-all-devices for module_id: {module_id}")

        all_devices = self.module_manager.get_all_devices(module_id)

        if all_devices is not None:
            logging.info(f"Sending all-devices for module_id: {module_id}")
        else:
            logging.debug(
                f"get-all-devices unsupported module_id: {module_id}. Sending None.")

        client.publish(
            topic="all-devices",
            payload=pickle.dumps(all_devices)
        )

    def on_get_last_device_status(self, client, payload):
        device = ip.Device.FromString(payload)
        last_device_status = self.module_manager.get_last_device_status(device)
        if last_device_status is None:
            device_status = ip.DeviceStatus(
                device=device,
                statusData=b""
            )
            internal_client_msg = ip.InternalClient(deviceStatus=device_status)
            last_device_status = internal_client_msg.SerializeToString()

        logging.info("Sending last-device-status")
        client.publish(
            topic="last-device-status",
            payload=last_device_status
        )

    def on_pass_command(self, _client, payload):
        external_command = ep.Command.FromString(payload)
        self.module_manager.update_command(external_command)

    def on_force_status_send(self, client, payload):
        device = ip.Device.FromString(payload)
        device_status = self.module_manager.get_last_device_status(device)
        if device_status is not None:
            logging.info("Sending periodic forced status.")
            client.publish(
                topic="send-status",
                payload=device_status
            )
        else:
            logging.error("Could not get status for forced periodic send.")

    def on_device_disconnect(self, client, payload):
        device = ip.Device.FromString(payload)
        last_device_status = self.module_manager.handle_device_disconnect(device)

        if last_device_status is not None:
            logging.info("Device disconnect.")
            client.publish(
                topic="exc-device-disconnect",
                payload=last_device_status
            )
        else:
            logging.error("Device disconnect on not registered device.")

    def on_message(self, client, userdata, msg):
        topic = msg.topic

        if topic == "from-internal":
            self.on_from_internal(client, msg.payload)
        elif topic == "get-all-devices":
            self.on_get_all_devices(client, msg.payload)
        elif topic == "get-last-device-status":
            self.on_get_last_device_status(client, msg.payload)
        elif topic == "pass-command":
            self.on_pass_command(client, msg.payload)
        elif topic == "force-status-send":
            self.on_force_status_send(client, msg.payload)
        elif topic == "device-disconnect":
            self.on_device_disconnect(client, msg.payload)

    def on_connect(self, client, *_):
        # internal
        client.subscribe("from-internal")
        client.subscribe("device-disconnect")
        # external
        client.subscribe("get-all-devices")
        client.subscribe("get-last-device-status")
        client.subscribe("pass-command")
        # period checker
        client.subscribe("force-status-send")

    def start_loop(self):
        mqtt_client = mqtt.Client()

        mqtt_client.on_connect = self.on_connect
        mqtt_client.on_message = self.on_message

        logging.info(
            f"Connecting to mqtt broker ({self.mqtt_url}:{self.mqtt_port})")
        mqtt_client.connect(host=self.mqtt_url, port=self.mqtt_port)
        logging.info("Module handler ready...")
        mqtt_client.loop_forever()
