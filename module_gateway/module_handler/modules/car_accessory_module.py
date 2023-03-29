import json

from .module_lib import ModuleBase, ModuleManagerBase


class CarAccessoryModuleManager(ModuleManagerBase):
    """Implementation of module manager for car accessory module (done by module maintainer)"""

    def send_status_condition(
        self, current_status_data: bytes, new_status_data: bytes, device_type: int
    ) -> bool:
        return current_status_data != new_status_data

    def generate_command(
        self,
        current_status: bytes,
        new_status: bytes,
        current_command: bytes,
        device_type: int,
    ) -> bytes:
        data = json.loads(new_status)
        if data["pressed"]:
            out = json.dumps({"lit_up": True})
        else:
            out = json.dumps({"lit_up": False})

        return out.encode()

    def aggregate_status(
        self, current_status: bytes, new_status: bytes, device_type: int
    ) -> bytes:
        return new_status

    def generate_default_command(self, device_type: int) -> bytes:
        return json.dumps({"lit_up": False}).encode()

    def status_data_valid(self, status_data: bytes, device_type: int) -> bool:
        try:
            data = json.loads(status_data)
        except json.JSONDecodeError:
            return False
        else:
            return "pressed" in data

    def command_data_valid(self, command_data: bytes, device_type: int) -> bool:
        try:
            data = json.loads(command_data)
        except json.JSONDecodeError:
            return False
        else:
            return "lit_up" in data


class CarAccessoryModule(ModuleBase):
    """Implementation of car accessory module"""
    MODULE_ID = 2
    SUPPORTED_DEVICE_IDS = {0, 1}

    MODULE_MANAGER = CarAccessoryModuleManager
