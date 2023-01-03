from enum import Enum, EnumMeta


class EnumHasValue(Enum):
    @classmethod
    def has_value(cls: EnumMeta, value: int) -> bool:
        return value in cls._value2member_map_
