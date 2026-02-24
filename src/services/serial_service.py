"""Minimal serial service — port listing and connection state (no real I/O yet)."""
import re

from PyQt6.QtCore import QObject

from serial.tools import list_ports as _list_ports_module


def _port_sort_key(port: str) -> tuple[int, float | str, str]:
    """Sort COM ports by numeric suffix (COM1, COM2, ..., COM10); others lexicographically."""
    if port.upper().startswith("COM"):
        m = re.search(r"(\d+)$", port)
        if m:
            return (0, float(int(m.group(1))), port)
    return (1, port, port)


class SerialService(QObject):
    """Serial port listing and connection state. open/close are stubs (no real I/O)."""

    def __init__(self, parent: QObject | None = None):
        super().__init__(parent)
        self._connected = False
        self._port: str | None = None
        self._baud = 115200
        self._timeout_ms = 1000

    def list_ports(self) -> list[str]:
        """Return list of port device names from pyserial comports(), sorted ascending (COM1, COM2, COM10)."""
        try:
            ports = [p.device for p in _list_ports_module.comports()]
            ports.sort(key=_port_sort_key)
            return ports
        except Exception:
            return []

    def open(
        self,
        port: str,
        baud: int = 115200,
        timeout_ms: int = 1000,
    ) -> bool:
        """Store params and set connected=True. No real open."""
        self._port = port
        self._baud = baud
        self._timeout_ms = timeout_ms
        self._connected = True
        return True

    def close(self) -> None:
        """Set connected=False."""
        self._connected = False

    def is_connected(self) -> bool:
        return self._connected

    def current(self) -> tuple[str | None, int, int]:
        """Return (port, baud, timeout_ms)."""
        return (self._port, self._baud, self._timeout_ms)
