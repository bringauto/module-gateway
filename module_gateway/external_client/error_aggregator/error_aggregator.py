import logging

import module_gateway.protobuf.InternalProtocol_pb2 as InternalProtocolProtob

from module_gateway.external_client.device import Device
from module_gateway.external_client.config import ModuleConfig
from module_gateway.external_client.utils import ProtoHelpers
from .device_error_aggregator import DeviceErrorAggregator
from . import modules


class ErrorAggregator:

    # module specific error aggregator api implementation
    MODULE_ID_TO_AGGREGATOR = {
        1: modules.MissionErrorAggregator,
        2: modules.CarAccessoryErrorAggregator,
    }

    def __init__(self, wanted_modules: list[ModuleConfig]) -> None:
        self._device_error_aggregators: dict[Device, DeviceErrorAggregator] = {}
        self._loaded_aggregators: dict[int, modules.ModuleAggregatorBase] = {
            mc.module_id: self.MODULE_ID_TO_AGGREGATOR[mc.module_id]()
            for mc in wanted_modules
            if mc.module_id in self.MODULE_ID_TO_AGGREGATOR
        }
        logging.info(
            f"ErrorAggregator initialized: loaded modules: {tuple(self._loaded_aggregators.keys())}"
        )

    def add_status_to_error_aggregator(self, status: InternalProtocolProtob.DeviceStatus):
        device = ProtoHelpers.get_device_from_status(status)
        logging.info(f"Adding status to error aggregator for {device}")

        if device in self._device_error_aggregators:
            self._device_error_aggregators[device].add_status(status)
        else:
            new_device_err_aggregator = DeviceErrorAggregator(device)
            new_device_err_aggregator.add_status(status)
            self._device_error_aggregators[device] = new_device_err_aggregator

    def is_empty(self, device: Device):
        if device not in self._device_error_aggregators:
            return True
        return self._device_error_aggregators[device].status_count() == 0

    def get_all_registered_devices_with_not_empty_aggregators(self) -> set[Device]:
        out_set = set()
        for device in self._device_error_aggregators.keys():
            if not self.is_empty(device):
                out_set.add(device)
        return out_set

    def get_last_error_status(self, device: Device) -> InternalProtocolProtob.DeviceStatus | None:
        if not self.is_device_registered(device):
            return None
        return self._device_error_aggregators[device].get_last_status()

    def is_device_registered(self, device: Device) -> bool:
        if device.module not in self._loaded_aggregators:
            return False
        if device not in self._device_error_aggregators:
            return False
        return True

    def get_error(self, device: Device) -> bytes | None:
        if not self.is_device_registered(device):
            logging.error(
                "Called get_error on device not registered in error aggregator, this should not be possible."
            )
            return None

        device_statuses = self._device_error_aggregators[device].get_statuses()
        if not device_statuses:
            logging.error(
                "Called get_error on device with no statuses in error aggregator, this should not be possible."
            )
            return None

        return self._loaded_aggregators[device.module].create_device_error(device_statuses)

    def clear_error_aggregator(self):
        self._device_error_aggregators.clear()
