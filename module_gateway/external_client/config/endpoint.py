class Endpoint:
    def __init__(self, ip_addr: str, port: int) -> None:
        self._ip_addr = ip_addr
        self._port = port

    @property
    def ip_addr(self) -> str:
        return self._ip_addr

    @property
    def port(self) -> int:
        return self._port

    def __eq__(self, __o: object) -> bool:
        if not isinstance(__o, Endpoint):
            return False
        if self._ip_addr == __o._ip_addr and self._port == __o._port:
            return True
        return False

    def __hash__(self):
        return hash((self._ip_addr, self._port))

    def __str__(self) -> str:
        return f"{self._ip_addr}:{self._port}"
