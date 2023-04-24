import logging

import module_gateway.protobuf.InternalProtocol_pb2 as ip
import module_gateway.protobuf.ExternalProtocol_pb2 as ep

from .modules.module_lib import exceptions as module_exc
from .modules.module_lib import ModuleBase

from .modules import CarAccessoryModule, MissionModule


class ModulesManager:
    """Manager of loaded modules"""
    SUPPORTED_MODULES: tuple[ModuleBase] = (
        CarAccessoryModule,
        MissionModule
    )

    def __init__(self):
        self.modules: dict[int, ModuleBase] = {
            module.MODULE_ID: module() for module in self.SUPPORTED_MODULES
        }
        nl = ",\n"
        logging.info(
            f"Module handler loaded modules:\n{nl.join([f'   - {mod.__name__} (id: {str(mod.MODULE_ID)})' for mod in self.SUPPORTED_MODULES])}")

    def handle_status(self, is_new_device: bool, status: ip.DeviceStatus) -> tuple[bytes | None, int]:
        device = status.device

        if device.module not in self.modules:
            logging.error(
                f"Device ({device}) tried to connect to module which is not supported {device.module}")
            return None, 0

        if is_new_device:
            logging.info(f"Registering new device: {device}")
            self.modules[device.module].remove_device(
                device.deviceName, device.deviceType, device.deviceRole)

        try:
            command_data, ready_to_sent_count = self.modules[device.module].get_command(
                status.statusData,
                device.deviceName,
                device.deviceType,
                device.deviceRole
            )
        except (module_exc.DeviceNotSupported, module_exc.InvalidStatusData) as e:
            logging.error(f"Couldn't get command for {device}: {e}")
            return None, 0

        # prepare deviceCommand message
        device_command = ip.DeviceCommand(
            commandData=command_data
        )
        msg = ip.InternalServer(deviceCommand=device_command)

        return msg.SerializeToString(), ready_to_sent_count

    def get_all_device_statuses(self, device: ip.Device) -> list[bytes] | None:
        if device.module not in self.modules:
            logging.error(
                f"Tried to get all device statuses from non supported module: {device.module}")
            return None

        statuses = []
        while True:
            try:
                status_data = self.modules[device.module].get_aggregated_status(
                    device.deviceType, device.deviceRole)
            except module_exc.DeviceNotRegistered:
                logging.error("Tried to get status from non registered device")
                return None
            if status_data is None:
                break
            status = ip.DeviceStatus(device=device, statusData=status_data)
            statuses.append(
                ip.InternalClient(deviceStatus=status).SerializeToString()
            )

        return statuses

    def get_all_devices(self, module_id: int) -> list[bytes] | None:
        if module_id not in self.modules:
            logging.error(
                f"Tried to get all devices from non supported module: {module_id}")
            return None

        return self.modules[module_id].get_all_registered_devices()

    def get_last_device_status(self, device: ip.Device) -> bytes | None:
        if device.module not in self.modules:
            logging.error(
                "Tried to get last device status for unsupported module")
            return None

        try:
            possible_status_data = self.modules[device.module].get_aggregated_status(
                device_type=device.deviceType,
                device_role=device.deviceRole
            )
        except module_exc.DeviceNotRegistered:
            logging.error(
                "Tried to get last device status for not registered device")
            return None

        if possible_status_data is None:
            # need to force aggregate and try again
            # dont need to check for module_exc.DeviceNotRegister because this function is blocking
            try:
                aggr_ready_count = self.modules[device.module].force_aggregation_on_device(
                    device_type=device.deviceType,
                    device_role=device.deviceRole
                )
            except module_exc.CantAggregate:
                logging.error(
                    "Tried to force aggregate device which was cleared and cant be.")
                return None
            if not aggr_ready_count:
                logging.error(
                    "Something is really broken, no statuses are ready after force aggregation.")
                return None

            possible_status_data = self.modules[device.module].get_aggregated_status(
                device_type=device.deviceType,
                device_role=device.deviceRole
            )
            if possible_status_data is None:
                # this should not be possible but never say never
                return None

        device_status = ip.DeviceStatus(
            device=device,
            statusData=possible_status_data
        )
        internal_client_msg = ip.InternalClient(deviceStatus=device_status)
        return internal_client_msg.SerializeToString()

    def update_command(self, external_command: ep.Command) -> None:
        device = external_command.deviceCommand.device
        module = device.module
        if module not in self.modules:
            logging.error(
                f"Update command on not supported module: {module}")
            return

        try:
            self.modules[module].update_command(
                command_data=external_command.deviceCommand.commandData,
                device_type=device.deviceType,
                device_role=device.deviceRole
            )
        except (module_exc.DeviceNotRegistered, module_exc.InvalidCommandData) as e:
            logging.error(f"Update command unsuccessful: {e}")

    def handle_device_disconnect(self, device: ip.Device) -> bytes:
        if device.module not in self.modules:
            logging.error("Device disconnect on not supported module")
            return None

        last_device_status = self.get_last_device_status(device)

        device_removed = self.modules[device.module].remove_device(
            device_name=device.deviceName,
            device_type=device.deviceType,
            device_role=device.deviceRole
        )

        if not device_removed:
            return None

        if last_device_status is None:
            # generate deviceStatus message with not statusData
            device_status = ip.DeviceStatus(
                device=device,
                statusData=b""
            )
            internal_client_msg = ip.InternalClient(deviceStatus=device_status)
            return internal_client_msg.SerializeToString()
        else:
            return last_device_status
