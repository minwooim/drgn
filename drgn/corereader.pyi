from os import PathLike
from typing import Union


class CoreReader:
    def __init__(self, path: Union[str, bytes, PathLike]) -> None: ...
    def read(self, address: int, size: int) -> bytes: ...
    def read_u8(self, address: int) -> int: ...
    def read_u16(self, address: int) -> int: ...
    def read_u32(self, address: int) -> int: ...
    def read_u64(self, address: int) -> int: ...
    def read_s8(self, address: int) -> int: ...
    def read_s16(self, address: int) -> int: ...
    def read_s32(self, address: int) -> int: ...
    def read_s64(self, address: int) -> int: ...
    def read_bool(self, address: int) -> bool: ...
    def read_bool16(self, address: int) -> bool: ...
    def read_bool32(self, address: int) -> bool: ...
    def read_bool64(self, address: int) -> bool: ...
    def read_float(self, address: int) -> float: ...
    def read_double(self, address: int) -> float: ...
    def read_long_double(self, address: int) -> float: ...
