from abc import ABC, abstractmethod

import module_gateway.protobuf.InternalProtocol_pb2 as ip

from . import exceptions
from .device import Device
from .module_manager_base import ModuleManagerBase


class ModuleBase:
    """Base implementation of module logic."""
    
    MODULE_ID = 0
    SUPPORTED_DEVICE_IDS = {
        0,
        1
    }
    MODULE_MANAGER: ModuleManagerBase = None

    def __init__(self):
        self._devices: dict[int, dict[str, Device]] = {}
        self.module_manager: ModuleManagerBase = self.MODULE_MANAGER()

    def _is_device_registered(self, device_type: int, device_role: str) -> bool:
        if not device_type in self._devices:
            return False

        if not device_role in self._devices[device_type]:
            return False

        return True

    def destroy_status_aggregator(self):
        raise NotImplementedError

    def clear_all_devices(self) -> None:
        for device_type, roles in self._devices.items():
            for device_role in roles.keys():
                self._devices[device_type][device_role].clear_device()

    def clear_device(self, device_type: int, device_role: str) -> None:
        try:
            self._devices[device_type][device_role].clear_device()
        except KeyError:
            raise exceptions.DeviceNotRegistered(
                "This device is not registered.")

    def remove_device(self, device_name: str, device_type: int, device_role: str) -> bool:
        if self._is_device_registered(device_type, device_role):
            del self._devices[device_type][device_role]
            if not self._devices[device_type]:
                del self._devices[device_type]
            return True
        else:
            return False

    def _register_device(self, device_name, device_type, device_role, first_status):
        new_device = Device(device_name, device_type,
                            device_role, module_id=self.MODULE_ID)
        self._handle_first_status(new_device, first_status)

        if device_type not in self._devices:
            self._devices[device_type] = {}

        self._devices[device_type][device_role] = new_device

    def _handle_first_status(self, device: Device, first_status: bytes) -> None:
        device.aggregated_message = first_status
        device.ready_to_sent.append(first_status)
        device.current_command = self.module_manager.generate_default_command(
            device.device_type)

    def get_all_registered_devices(self) -> list[bytes]:
        all_devices = []
        for device_type, roles in self._devices.items():
            for device_role in roles.keys():
                all_devices.append(
                    self._devices[device_type][device_role].get_device_protobuf())

        return all_devices

    def get_aggregated_status(self, device_type: int, device_role: str) -> bytes | None:
        if not self._is_device_registered(device_type, device_role):
            raise exceptions.DeviceNotRegistered(
                "This device is not registered.")

        return self._devices[device_type][device_role].pop_oldest_ready_status()

    def force_aggregation_on_device(self, device_type: int, device_role: str) -> int:
        if not self._is_device_registered(device_type, device_role):
            raise exceptions.DeviceNotRegistered(
                "This device is not registered.")
        device = self._devices[device_type][device_role]
        if device.aggregated_message is None:
            raise exceptions.CantAggregate(
                "This device does not have any previous statuses because it was cleared.")

        device.ready_to_sent.append(device.aggregated_message)
        return len(device.ready_to_sent)

    def update_command(self, command_data: bytes, device_type: int, device_role: str) -> None:
        if not self._is_device_registered(device_type, device_role):
            raise exceptions.DeviceNotRegistered(
                "This device is not registered.")

        if not self.module_manager.command_data_valid(command_data, device_type):
            raise exceptions.InvalidCommandData("Command data is invalid.")

        self._devices[device_type][device_role].current_command = command_data

    def get_command(self, status_data: bytes, device_name: str, device_type: int, device_role: str) -> tuple[bytes, int]:
        if device_type not in self.SUPPORTED_DEVICE_IDS:
            raise exceptions.DeviceNotSupported(
                f"This device is not supported. Supported devices are: {self.SUPPORTED_DEVICE_IDS}")

        if not self.module_manager.status_data_valid(status_data, device_type):
            raise exceptions.InvalidStatusData("Status data is invalid.")

        if not self._is_device_registered(device_type, device_role):
            self._register_device(device_name, device_type,
                                  device_role, status_data)
            return (
                self._devices[device_type][device_role].current_command,
                len(self._devices[device_type][device_role].ready_to_sent)
            )

        device = self._devices[device_type][device_role]

        if device.aggregated_message is None:
            # there is no previous message in buffer, force aggregate, probably after device clear
            self._handle_first_status(device, status_data)
            return (
                device.current_command,
                len(device.ready_to_sent)
            )

        # retrieve new command
        current_status = device.aggregated_message
        new_command = self.module_manager.generate_command(
            current_status, status_data, device.current_command, device.device_type)
        device.current_command = new_command

        # check send status condition for status
        if self.module_manager.send_status_condition(current_status, status_data, device_type):
            device.aggregated_message = status_data
            device.ready_to_sent.append(status_data)
        else:
            device.aggregated_message = self.module_manager.aggregate_status(
                current_status, status_data, device_type
            )

        return (
            device.current_command,
            len(device.ready_to_sent)
        )
