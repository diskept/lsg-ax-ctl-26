"""Serial service — port listing, open/close, and basic I/O via pyserial."""
import re
import threading
from typing import Optional

from PyQt6.QtCore import QObject

import serial
from serial.tools import list_ports as _list_ports_module


def _port_sort_key(port: str) -> tuple[int, float | str, str]:
    """Sort COM ports by numeric suffix (COM1, COM2, ..., COM10); others lexicographically."""
    if port.upper().startswith("COM"):
        m = re.search(r"(\d+)$", port)
        if m:
            return (0, float(int(m.group(1))), port)
    return (1, port, port)


class SerialService(QObject):
    """Serial port listing and a thin wrapper around pyserial Serial."""

    def __init__(self, parent: QObject | None = None):
        super().__init__(parent)
        self._ser: Optional[serial.Serial] = None
        self._lock = threading.RLock()
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
        """Open the serial port. Returns True on success."""
        port = (port or "").strip()
        if not port:
            return False
        with self._lock:
            self.close()
            self._port = port
            self._baud = int(baud)
            self._timeout_ms = int(timeout_ms)
            try:
                # pyserial timeout is in seconds (float). Use same value for read & write timeout.
                timeout_s = max(0.0, self._timeout_ms / 1000.0)
                self._ser = serial.Serial(
                    port=self._port,
                    baudrate=self._baud,
                    timeout=timeout_s,
                    write_timeout=timeout_s,
                )
                return True
            except Exception:
                self._ser = None
                return False

    def close(self) -> None:
        """Close the serial port if open."""
        with self._lock:
            if self._ser is not None:
                try:
                    self._ser.close()
                except Exception:
                    pass
            self._ser = None

    def is_connected(self) -> bool:
        with self._lock:
            return self._ser is not None and bool(getattr(self._ser, "is_open", False))

    def current(self) -> tuple[str | None, int, int]:
        """Return (port, baud, timeout_ms)."""
        return (self._port, self._baud, self._timeout_ms)

    def write(self, data: bytes) -> int:
        """Write bytes to the serial port. Returns number of bytes written (0 on failure)."""
        if not data:
            return 0
        with self._lock:
            if not self.is_connected() or self._ser is None:
                return 0
            try:
                return int(self._ser.write(data))
            except Exception:
                return 0

    def read(self, size: int = 1) -> bytes:
        """Read up to size bytes."""
        if size <= 0:
            return b""
        with self._lock:
            if not self.is_connected() or self._ser is None:
                return b""
            try:
                return bytes(self._ser.read(size))
            except Exception:
                return b""

    def read_all(self) -> bytes:
        """Read all available bytes (non-blocking with configured timeout)."""
        with self._lock:
            if not self.is_connected() or self._ser is None:
                return b""
            try:
                waiting = int(getattr(self._ser, "in_waiting", 0) or 0)
                if waiting <= 0:
                    return b""
                return bytes(self._ser.read(waiting))
            except Exception:
                return b""

    def readline(self, max_bytes: int = 4096) -> bytes:
        """Read a line (up to newline) or until timeout."""
        with self._lock:
            if not self.is_connected() or self._ser is None:
                return b""
            try:
                # pyserial handles timeout.
                return bytes(self._ser.readline(max_bytes))
            except Exception:
                return b""
