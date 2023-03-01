from abc import ABC, abstractmethod


class ModuleManagerBase(ABC):
    """Module aggregator interface used by module maintainer"""

    @abstractmethod
    def send_status_condition(self, current_status_data: bytes, new_status_data: bytes, device_type: int) -> bool:
        pass

    @abstractmethod
    def generate_command(self, current_status: bytes, new_status: bytes, current_command: bytes, device_type: int) -> bytes:
        pass

    @abstractmethod
    def aggregate_status(self, current_status: bytes, new_status: bytes, device_type: int) -> bytes:
        pass

    @abstractmethod
    def generate_default_command(self, device_type: int) -> bytes:
        pass

    @abstractmethod
    def status_data_valid(self, status_data: bytes, device_type: int) -> bool:
        pass

    @abstractmethod
    def command_data_valid(self, command_data: bytes, device_type: int) -> bool:
        pass