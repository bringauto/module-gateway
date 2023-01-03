import json

from module_gateway.modules.message_creator import MessageCreator
import module_gateway.protobuf.InternalProtocol_pb2 as internal_protocol


class CarAccessoryCreator(MessageCreator):

    def create_command(self, status_bytes: bytes) -> internal_protocol.DeviceCommand:
        status = json.loads(status_bytes)
        message = internal_protocol.DeviceCommand()
        led_on = True if status["pressed"] else False
        message.commandData = json.dumps({"lit_up": led_on}).encode()
        return message
