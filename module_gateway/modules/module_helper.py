from module_gateway.modules.module_type import ModuleType
from module_gateway.modules.message_creator import MessageCreator
from module_gateway.modules.car_accessory_module.car_accessory_creator import CarAccessoryCreator
from module_gateway.modules.car_accessory_module.car_accessory_type import CarAccessoryType
from module_gateway.modules.mission_module.mission_creator import MissionCreator
from module_gateway.modules.mission_module.mission_type import MissionType


def check_supported_device_for_module(module: int, device_type: int) -> bool:
    match module:
        case ModuleType.CAR_ACCESSORY_MODULE.value:
            return CarAccessoryType.has_value(device_type)
        case ModuleType.MISSION_MODULE.value:
            return MissionType.has_value(device_type)
        case _:
            return False


def creator_factory(module: int) -> MessageCreator | None:
    match module:
        case ModuleType.CAR_ACCESSORY_MODULE.value:
            return CarAccessoryCreator()
        case ModuleType.MISSION_MODULE.value:
            return MissionCreator()
        case _:
            return None
