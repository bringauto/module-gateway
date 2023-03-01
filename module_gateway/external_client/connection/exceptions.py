class BadSessionId(Exception):
    def __init__(self, message: str) -> None:
        super().__init__(f"Bad Session ID: {message}")


class AlreadyLogged(Exception):
    def __init__(self, message: str) -> None:
        super().__init__(f"Already logged: {message}")


class ConnectionSequenceInvalid(Exception):
    """Connection sequence between external client <-> server unsuccessful"""
    pass


class CommandOutOfOrder(Exception):
    """Command was received out of order."""
    pass


class ConnectionSequenceNoDevices(Exception):
    pass