from enum import Enum

from pydantic import BaseModel, ValidationError
from pydantic.dataclasses import dataclass

from .module_lib import ModuleBase, ModuleManagerBase, exceptions


@dataclass
class Position:
    latitude: float
    longitude: float
    altitude: float


class Station(BaseModel):
    name: str
    position: Position


class Telemetry(BaseModel):
    speed: float
    fuel: float
    position: Position


class State(Enum):
    IDLE = 0
    DRIVE = 1
    IN_STOP = 2
    OBSTACLE = 3
    ERROR = 4


class Action(Enum):
    NO_ACTION = 0
    STOP = 1
    START = 2


class AutonomyStatus(BaseModel):
    telemetry: Telemetry
    state: State
    nextStop: Station | None = None


class AutonomyCommand(BaseModel):
    stops: list[Station]
    route: str
    action: Action


class MissionModuleManager(ModuleManagerBase):
    """Implementation of module manager for mission module (done by module maintainer)"""

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
        return current_command

    def aggregate_status(
        self, current_status: bytes, new_status: bytes, device_type: int
    ) -> bytes:
        return new_status

    def generate_default_command(self, device_type: int) -> bytes:
        return AutonomyCommand(
            stops=[],
            route="",
            action=Action.NO_ACTION
        ).json().encode()

    def status_data_valid(self, status_data: bytes, device_type: int) -> bool:
        try:
            _ = AutonomyStatus.parse_raw(status_data)
        except ValidationError:
            return False
        else:
            return True

    def command_data_valid(self, command_data: bytes, device_type: int) -> bool:
        try:
            _ = AutonomyCommand.parse_raw(command_data)
        except ValidationError:
            return False
        else:
            return True


class MissionModule(ModuleBase):
    """Implementation of mission module"""
    MODULE_ID = 1
    SUPPORTED_DEVICE_IDS = {0}

    MODULE_MANAGER = MissionModuleManager