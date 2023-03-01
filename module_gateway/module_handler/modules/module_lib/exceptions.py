
class DeviceNotSupported(Exception):
    """Device type not supported in module"""
    pass

class DeviceNotRegistered(Exception):
    """Device is not registered in this module"""
    pass


class CantAggregate(Exception):
    """Device statuses can not be aggregated"""
    pass


class InvalidCommandData(Exception):
    """Module manager got invalid commandData"""
    pass

class InvalidStatusData(Exception):
    """Module manager got invalid statusData"""
    pass