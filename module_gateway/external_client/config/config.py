import logging

from pydantic import BaseModel, ValidationError

from .endpoint import Endpoint
from .module_config import ModuleConfig


class ModuleConfigModel(BaseModel):
    moduleName: str
    moduleId: int


class EndpointConfigModel(BaseModel):
    ip: str
    port: int
    modules: list[ModuleConfigModel]


class ConfigModel(BaseModel):
    carId: str
    vehicleName: str
    company: str
    endpoints: list[EndpointConfigModel]


class InvalidConfigError(Exception):
    """Invalid config."""
    pass


class Config:
    """Class representing json config file - which modules are connected to which endpoints"""

    def __init__(self, config_path: str) -> None:
        self._module_id_to_config: dict[int, ModuleConfig] = {}
        self.parse_config(config_path)

    def parse_config(self, config_path: str) -> None:
        try:
            with open(config_path) as f:
                data = f.read()
        except OSError as e:
            raise InvalidConfigError(f"Config could not be loaded: {e}")

        try:
            config = ConfigModel.parse_raw(data)
        except ValidationError as e:
            raise InvalidConfigError(f"Invalid config: {e}")

        self._car_id = config.carId
        self._vehicle_name = config.vehicleName
        self._company = config.company

        for endpoint_config in config.endpoints:
            endpoint = Endpoint(ip_addr=endpoint_config.ip, port=endpoint_config.port)

            for module_config in endpoint_config.modules:
                module_id = module_config.moduleId
                if module_id in self._module_id_to_config:
                    raise InvalidConfigError(
                        f"Module {module_config} with endpoint {endpoint} already defined with endpoint: {self._module_id_to_config[module_id].endpoint}."
                    )
                else:
                    new_module_config = ModuleConfig(
                        module_id=module_id,
                        module_name=module_config.moduleName,
                        endpoint=endpoint,
                    )
                    self._module_id_to_config[module_id] = new_module_config

        logging.info(
            f"Config loaded with {len(self._module_id_to_config)} modules: {list(self._module_id_to_config.values())}"
        )

    def get_endpoint_for_module_id(self, module_id: int) -> Endpoint | None:
        if module_id not in self._module_id_to_config:
            return None

        return self._module_id_to_config[module_id].endpoint

    def get_modules_with_endpoint(self, endpoint: Endpoint) -> list[ModuleConfig]:
        """returns, which modules should be connected to passed endpoint"""
        return [
            module
            for module in self._module_id_to_config.values()
            if module.endpoint == endpoint
        ]

    @property
    def car_id(self) -> str:
        return self._car_id

    @property
    def vehicle_name(self) -> str:
        return self._vehicle_name

    @property
    def company(self) -> str:
        return self._company
