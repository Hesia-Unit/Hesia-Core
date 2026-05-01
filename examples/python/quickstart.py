from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "bindings" / "python"))

from hesia_core import HesiaCore, Watchdog


def main() -> None:
    core = HesiaCore()
    print(f"HESIA Core {core.version}")
    core.validate_inference("demo-model", b"\x01\x02\x03", output_capacity=32, deadline_ms=10)
    watchdog = Watchdog(core, timeout_ms=1000)
    watchdog.arm()
    watchdog.heartbeat()
    watchdog.check()


if __name__ == "__main__":
    main()
