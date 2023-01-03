import string
import json
import random
import queue

import module_gateway.protobuf.ExternalProtocol_pb2 as ExternalProtocolProtob
import module_gateway.protobuf.InternalProtocol_pb2 as InternalProtocolProtob
import paho.mqtt.client as mqtt

"""
TODO don't forget to check functionality: when client starts connecting
 to the server, error aggregator should be locked - messages can get lost.
"""


class Endpoint:
    def __init__(self, ip_addr: str, port: int) -> None:
        self._ip_addr = ip_addr
        self._port = port

    @property
    def ip_addr(self) -> str:
        return self._ip_addr

    @property
    def port(self) -> int:
        return self._port


    def __eq__(self, __o: object) -> bool:
        if not isinstance(__o, Endpoint):
            return False
        if self._ip_addr == __o._ip_addr and self._port == __o._port:
            return True
        return False

    def __str__(self) -> str:
        return self._ip_addr + "/" + str(self._port)


class ModuleConfig:
    def __init__(self, module_id: int, endpoint: Endpoint) -> None:
        self.module_id = module_id
        self.endpoint = endpoint

    def __repr__(self) -> str:
        return str(self.module_id)


class Device:
    def __init__(self, device_name: str, device_type: int, module: InternalProtocolProtob.Device.Module,
                 device_role: str) -> None:
        self.device_name = device_name
        self.device_type = device_type
        self.module = module
        self.device_role = device_role

    def get_protobuf(self) -> InternalProtocolProtob.Device:
        proto = InternalProtocolProtob.Device()
        proto.module = self.module
        proto.deviceType = self.device_type
        proto.deviceRole = self.device_role
        proto.deviceName = self.device_name
        return proto


class ConnectionChannel:
    KEEP_ALIVE = 60



    def __init__(self, broker: str, port: int, input_topic:str, output_topic: str) -> None:
        self.broker = broker
        self.port = port
        self._input_topic = input_topic
        self._output_topic = output_topic
        self.client = None
        self.rec_msgs_queue = queue.Queue()

    def _on_connect(self, client, userdata, flags, rc):
        print("Connected with result code "+str(rc))
        client.subscribe(self._input_topic)

    def _on_message(self, client, userdata, msg):
        print(msg.topic+" "+str(msg.payload))
        self.rec_msgs_queue.put(str(msg.payload), block=False, timeout=None)

    def start(self):
        self.client = mqtt.Client()
        self.client.on_connect = self._on_connect
        self.client.on_message = self._on_message
        self.client.connect(self.broker, self.port, self.KEEP_ALIVE)
        self.client.loop_start()

    def stop(self):
        self.client.loop_stop()


    def send_msg(self, msg) -> None:
        print("sending msg...")
        self.client.publish(self._output_topic, msg)

    def get_last_msg(self, timeout: int) -> None:
        self.rec_msgs_queue.get(block=True, timeout=timeout)


class ExternalConnection:
    """
    one ExternalConnection is for one module set
    """
    KEY_LENGHT = 8
    RESPONSE_TIMEOUT = 30

    def __init__(self, endpoint: Endpoint, associated_modules: list[ModuleConfig]) -> None:
        self.message_counter = 0
        self._session_id = None
        self._endpoint = endpoint
        self._associated_modules = associated_modules
        self.io_connection = ConnectionChannel("broker", 80, "input/", "output/")

    @property
    def endpoint(self) -> Endpoint:
        return self._endpoint

    def send_status(self, stat: InternalProtocolProtob.DeviceStatus) -> None:
        encapsulated_status = ExternalProtocolProtob.Status()
        encapsulated_status.sessionId = self.get_session_id()
        encapsulated_status.deviceState = ExternalProtocolProtob.Status.DeviceState.RUNNING
        encapsulated_status.messageCounter = self.get_next_message_counter()
        encapsulated_status.deviceStatus.CopyFrom(stat)
        try:
            self.io_connection.send_msg(encapsulated_status)  # TODO add .SerializeToString (just testing for now)
        except:  # TODO what error
            print("Error during send")

    def get_next_message_counter(self) -> int:
        self.message_counter += 1
        return self.message_counter

    def _connect_msg_handle(self, dev_list):
        connect_msg = ExternalProtocolProtob.Connect()
        self.set_session_id("CAR1")  # TODO add to attr
        connect_msg.sessionId = self.get_session_id()
        connect_msg.company = "Company"  # TODO add to attr
        connect_msg.vehicleName = "vehName"  # TODO add to attr
        for device in dev_list:
            connect_msg.devices.extend([device.get_protobuf()])

        self.io_connection.send_msg(connect_msg)  # TODO add .SerializeToString (just testing for now)
        connect_response_msg = self.io_connection.get_last_msg(self.RESPONSE_TIMEOUT) # TODO when timeout, reset connection procedure + if not already logged

    def _status_msgs_handle(self, dev_list):
        for device in dev_list:
            last_status = get_last_device_status(device)
            self.io_connection.send_msg(last_status)
        for i in range(len(dev_list)):
            status_response = self.io_connection.get_last_msg(self.RESPONSE_TIMEOUT)
            #TODO should check status responses
        
    def _command_msgs_handle(self, dev_list):
        for i in range(len(dev_list)):
            command_msg = self.io_connection.get_last_msg(self.RESPONSE_TIMEOUT)
            #TODO pass commands to devices
            command_response = ExternalProtocolProtob.CommandResponse()
            command_response.sessionId = self.get_session_id()
            command_response.type = ExternalProtocolProtob.CommandResponse.Type.OK;
            command_response.messageCounter = self.get_next_message_counter # should be same as arrived counter
            self.io_connection.send_msg(command_response)
        




    def init_connection(self) -> None:
        print("Initializing connection to endpoint " + str(self._endpoint) + " for modules: " + str(self._associated_modules))
        dev_list = []
        for module in self._associated_modules:
            dev_list.extend(get_all_devices(module.module_id))
        self._connect_msg_handle(dev_list)
        self._status_msgs_handle(dev_list)
        self._command_msgs_handle(dev_list)
        # TODO receive commands for all devices (rest of conn seq.)



    def get_session_id(self) -> str:
        return self._session_id

    def set_session_id(self, car_id: str) -> None:
        self._session_id = self.generate_session_id(car_id)

    def generate_session_id(self, car_id: str) -> str:
        letters = string.ascii_letters + string.digits
        result_str = ''.join(random.choice(letters) for _ in range(ExternalConnection.KEY_LENGHT))
        return car_id + result_str


class Config:
    def __init__(self, config_path: str) -> None:
        self.modules_configs: list[ModuleConfig] = []
        self.parse_config(config_path)

    def get_endpoint_for_module_id(self, module_id) -> Endpoint | None:
        for module_config in self.modules_configs:
            if (module_config.module_id == module_id):
                return module_config.endpoint
        return None

    def parse_config(self, config_path: str) -> None:
        with open(config_path) as f:
            data = json.load(f)
            for module in data["moduleToEndpoint"]:
                new_mod_conf = ModuleConfig(module["moduleId"], Endpoint(module["endpointIp"], module["endpointPort"]))
                self.modules_configs.append(new_mod_conf)

    def get_modules_with_endpoint(self, endpoint: Endpoint) -> list[ModuleConfig]:
        modules_to_ret = []
        for module in self.modules_configs:
            if module.endpoint == endpoint:
                modules_to_ret.append(module)
        return modules_to_ret


class ExternalClient:
    def __init__(self) -> None:
        self.connection_list: list[ExternalConnection] = []
        self.config = Config('config/config.json')

    def get_connection(self, module_id) -> ExternalConnection:
        endpoint = self.config.get_endpoint_for_module_id(module_id)
        for connection in self.connection_list:
            if (connection.endpoint == endpoint):
                return connection
        return None

    def add_connection(self, endpoint: Endpoint) -> ExternalConnection:
        new_connection = ExternalConnection(endpoint, self.config.get_modules_with_endpoint(endpoint))
        self.connection_list.append(new_connection)
        return new_connection

    def _get_module_id_from_status(self, stat: InternalProtocolProtob.DeviceStatus):
        return stat.device.module

    def send_status(self, stat: InternalProtocolProtob.DeviceStatus) -> None:
        module_id = self._get_module_id_from_status(stat)
        connection = self.get_connection(module_id)
        if connection is None:
            endpoint_for_module = self.config.get_endpoint_for_module_id(module_id)
            if endpoint_for_module is None:
                raise Exception("No endpoint defined for module with id " + str(module_id))
            connection = self.add_connection(endpoint_for_module)
            connection.init_connection()
        connection.send_status(stat)

    def get_command(self, device):
        pass


def get_all_devices(module_id):  # TODO replace with real getting
    dev_list = []
    dev_list.append(Device("name1", 0, InternalProtocolProtob.Device.Module.EXAMPLE_MODULE, "role1"))
    dev_list.append(Device("name2", 0, InternalProtocolProtob.Device.Module.EXAMPLE_MODULE, "role2"))
    return dev_list


def get_last_device_status(device: Device) -> InternalProtocolProtob.DeviceStatus:  # TODO replace with real last status
    last_status = InternalProtocolProtob.DeviceStatus()
    last_status.statusData = b'last status data'
    device = InternalProtocolProtob.Device()
    device.module = 10
    device.deviceType = 3
    device.deviceRole = "left"
    device.deviceName = "button"
    last_status.device.CopyFrom(device)
    return last_status
