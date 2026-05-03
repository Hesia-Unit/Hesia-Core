from __future__ import annotations

import ctypes
import os
from ctypes import c_char_p, c_int, c_size_t, c_uint32, c_uint64, c_uint8, c_void_p
from pathlib import Path


HESIA_STATUS_OK = 0
HESIA_STATUS_UNSUPPORTED = 3

HESIA_FEATURE_OS_RANDOM = 1
HESIA_FEATURE_OPENSSL_AEAD = 2
HESIA_FEATURE_LIBOQS_PQC = 3
HESIA_FEATURE_WATCHDOG = 4
HESIA_FEATURE_INFERENCE_GUARD = 5


class HesiaErrorStruct(ctypes.Structure):
    _fields_ = [
        ("status", c_int),
        ("subsystem", ctypes.c_char * 32),
        ("message", ctypes.c_char * 192),
    ]


class WatchdogStruct(ctypes.Structure):
    _fields_ = [
        ("timeout_ms", c_uint64),
        ("last_heartbeat_ms", c_uint64),
        ("max_missed", c_uint32),
        ("missed", c_uint32),
        ("armed", c_int),
    ]


class ModelContractStruct(ctypes.Structure):
    _fields_ = [
        ("model_id", c_char_p),
        ("model_version", c_uint32),
        ("max_input_bytes", c_size_t),
        ("max_output_bytes", c_size_t),
        ("max_latency_ms", c_uint32),
        ("require_signed_model", c_int),
        ("model_digest", ctypes.POINTER(c_uint8)),
        ("model_digest_len", c_size_t),
    ]


class InferenceRequestStruct(ctypes.Structure):
    _fields_ = [
        ("input", ctypes.POINTER(c_uint8)),
        ("input_len", c_size_t),
        ("output", ctypes.POINTER(c_uint8)),
        ("output_capacity", c_size_t),
        ("deadline_ms", c_uint32),
    ]


class HesiaError(RuntimeError):
    def __init__(self, status: int, message: str):
        super().__init__(message)
        self.status = status


def _default_library_names() -> list[str]:
    if os.name == "nt":
        return ["hesia_core.dll", "libhesia_core.dll"]
    if os.uname().sysname == "Darwin":
        return ["libhesia_core.dylib"]
    return ["libhesia_core.so"]


def load(path: str | os.PathLike[str] | None = None) -> ctypes.CDLL:
    candidates: list[Path] = []
    if path is not None:
        candidates.append(Path(path))
    env_path = os.environ.get("HESIA_CORE_LIBRARY")
    if env_path:
        candidates.append(Path(env_path))
    candidates.extend(Path(name) for name in _default_library_names())

    last_error: OSError | None = None
    for candidate in candidates:
        try:
            lib = ctypes.CDLL(str(candidate))
            _configure(lib)
            return lib
        except OSError as exc:
            last_error = exc
    raise RuntimeError(f"cannot load HESIA Core library: {last_error}")


def _configure(lib: ctypes.CDLL) -> None:
    lib.hesia_version.restype = c_char_p
    lib.hesia_status_string.argtypes = [c_int]
    lib.hesia_status_string.restype = c_char_p
    lib.hesia_feature_available.argtypes = [c_int]
    lib.hesia_feature_available.restype = c_int
    lib.hesia_feature_provider_version.argtypes = [c_int]
    lib.hesia_feature_provider_version.restype = c_char_p
    lib.hesia_error_init.argtypes = [ctypes.POINTER(HesiaErrorStruct)]
    lib.hesia_error_message.argtypes = [ctypes.POINTER(HesiaErrorStruct)]
    lib.hesia_error_message.restype = c_char_p
    lib.hesia_random.argtypes = [ctypes.POINTER(c_uint8), c_size_t, ctypes.POINTER(HesiaErrorStruct)]
    lib.hesia_random.restype = c_int
    lib.hesia_watchdog_init.argtypes = [
        ctypes.POINTER(WatchdogStruct),
        c_uint64,
        c_uint32,
        ctypes.POINTER(HesiaErrorStruct),
    ]
    lib.hesia_watchdog_init.restype = c_int
    lib.hesia_watchdog_arm.argtypes = [ctypes.POINTER(WatchdogStruct), ctypes.POINTER(HesiaErrorStruct)]
    lib.hesia_watchdog_arm.restype = c_int
    lib.hesia_watchdog_heartbeat.argtypes = [ctypes.POINTER(WatchdogStruct), ctypes.POINTER(HesiaErrorStruct)]
    lib.hesia_watchdog_heartbeat.restype = c_int
    lib.hesia_watchdog_check.argtypes = [ctypes.POINTER(WatchdogStruct), ctypes.POINTER(HesiaErrorStruct)]
    lib.hesia_watchdog_check.restype = c_int
    lib.hesia_watchdog_disarm.argtypes = [ctypes.POINTER(WatchdogStruct)]
    lib.hesia_inference_validate.argtypes = [
        ctypes.POINTER(ModelContractStruct),
        ctypes.POINTER(InferenceRequestStruct),
        ctypes.POINTER(HesiaErrorStruct),
    ]
    lib.hesia_inference_validate.restype = c_int


def _raise_if_failed(lib: ctypes.CDLL, status: int, error: HesiaErrorStruct) -> None:
    if status == HESIA_STATUS_OK:
        return
    raw = lib.hesia_error_message(ctypes.byref(error))
    message = raw.decode("utf-8", errors="replace") if raw else lib.hesia_status_string(status).decode()
    raise HesiaError(status, message)


class HesiaCore:
    def __init__(self, path: str | os.PathLike[str] | None = None):
        self.lib = load(path)

    @property
    def version(self) -> str:
        return self.lib.hesia_version().decode("ascii")

    def feature_available(self, feature: int) -> bool:
        return bool(self.lib.hesia_feature_available(feature))

    def provider_version(self, feature: int) -> str:
        raw = self.lib.hesia_feature_provider_version(feature)
        return raw.decode("utf-8", errors="replace") if raw else ""

    def random(self, length: int) -> bytes:
        out = (c_uint8 * length)()
        error = HesiaErrorStruct()
        self.lib.hesia_error_init(ctypes.byref(error))
        status = self.lib.hesia_random(out, length, ctypes.byref(error))
        _raise_if_failed(self.lib, status, error)
        return bytes(out)

    def validate_inference(self, model_id: str, data: bytes, output_capacity: int, deadline_ms: int = 0) -> None:
        input_buf = (c_uint8 * len(data)).from_buffer_copy(data)
        output_buf = (c_uint8 * output_capacity)()
        contract = ModelContractStruct(
            model_id.encode("utf-8"),
            1,
            len(data),
            output_capacity,
            max(deadline_ms, 1),
            0,
            None,
            0,
        )
        request = InferenceRequestStruct(input_buf, len(data), output_buf, output_capacity, deadline_ms)
        error = HesiaErrorStruct()
        self.lib.hesia_error_init(ctypes.byref(error))
        status = self.lib.hesia_inference_validate(
            ctypes.byref(contract),
            ctypes.byref(request),
            ctypes.byref(error),
        )
        _raise_if_failed(self.lib, status, error)


class Watchdog:
    def __init__(self, core: HesiaCore, timeout_ms: int, max_missed: int = 1):
        self._core = core
        self._watchdog = WatchdogStruct()
        error = HesiaErrorStruct()
        self._core.lib.hesia_error_init(ctypes.byref(error))
        status = self._core.lib.hesia_watchdog_init(
            ctypes.byref(self._watchdog),
            timeout_ms,
            max_missed,
            ctypes.byref(error),
        )
        _raise_if_failed(self._core.lib, status, error)

    def arm(self) -> None:
        self._call(self._core.lib.hesia_watchdog_arm)

    def heartbeat(self) -> None:
        self._call(self._core.lib.hesia_watchdog_heartbeat)

    def check(self) -> None:
        self._call(self._core.lib.hesia_watchdog_check)

    def disarm(self) -> None:
        self._core.lib.hesia_watchdog_disarm(ctypes.byref(self._watchdog))

    def _call(self, fn) -> None:
        error = HesiaErrorStruct()
        self._core.lib.hesia_error_init(ctypes.byref(error))
        status = fn(ctypes.byref(self._watchdog), ctypes.byref(error))
        _raise_if_failed(self._core.lib, status, error)
