from .endpoint import Endpoint


class ModuleConfig:
    """This class represents, to which endpoint specific module is connected"""

    def __init__(self, module_id: int, module_name: str, endpoint: Endpoint) -> None:
        self.module_id = module_id
        self.module_name = module_name
        self.endpoint = endpoint

    def __repr__(self) -> str:
        return f"{self.module_id} ({self.module_name}, endpoint={self.endpoint})"
