from abc import ABC, abstractmethod

import module_gateway.protobuf.InternalProtocol_pb2 as InternalProtocolProtob


class ModuleAggregatorBase(ABC):
    """Error aggregator api implemented by module maintainer"""

    @staticmethod
    @abstractmethod
    def create_device_error(statuses: list[InternalProtocolProtob.DeviceStatus]) -> bytes:
        """Create module specific error message from list of statuses.

        Args:
            statuses (list[InternalProtocolProtob.DeviceStatus]): device statuses in error aggregator

        Returns:
            bytes: module specific generated error message
        """
        pass
