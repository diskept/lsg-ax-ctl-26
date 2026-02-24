"""NodeManager service — scan/connect/test using SerialService and AppSettings."""
import logging

from PyQt6.QtCore import QObject, pyqtSignal

from src.core.settings import AppSettings
from src.services.serial_service import SerialService

logger = logging.getLogger("node_manager")


class NodeManager(QObject):
    """Manages Axiom node discovery, connection, and basic operations."""

    scanned = pyqtSignal(list)   # list[str]
    connected = pyqtSignal(str)
    disconnected = pyqtSignal()
    info = pyqtSignal(str)
    error = pyqtSignal(str)
    serial_params_changed = pyqtSignal(bool, dict)  # (connected, params)

    def __init__(self, parent: QObject | None = None):
        super().__init__(parent)
        self._serial = SerialService(self)
        self._settings = AppSettings()

    def scan(self) -> None:
        """Emit scanned(list_ports()); info message if list empty/non-empty."""
        ports = self._serial.list_ports()
        self.scanned.emit(ports)
        if ports:
            logger.info("Found %d port(s): %s", len(ports), ", ".join(ports))
            self.info.emit(f"Found {len(ports)} port(s): {', '.join(ports)}")
        else:
            logger.info("No serial ports found.")
            self.info.emit("No serial ports found.")

    def connect(self) -> None:
        """Read from AppSettings and call connect_with(...)."""
        port = self._settings.get_last_port()
        if not port or not port.strip():
            logger.error("connect failed")
            self.error.emit("connect failed")
            return
        self.connect_with(
            port.strip(),
            self._settings.get_baud(),
            self._settings.get_parity(),
            self._settings.get_data_bits(),
            self._settings.get_stop_bits(),
            self._settings.get_timeout_ms(),
        )

    def disconnect(self) -> None:
        """Close the service and emit disconnected, info, serial_params_changed."""
        logger.info("disconnect")
        self._serial.close()
        self.disconnected.emit()
        self.info.emit("disconnect")
        self._emit_serial_params_changed()

    def _params_dict(self) -> dict:
        return {
            "port": self._settings.get_last_port() or "—",
            "baud": self._settings.get_baud(),
            "parity": self._settings.get_parity(),
            "data_bits": self._settings.get_data_bits(),
            "stop_bits": self._settings.get_stop_bits(),
            "timeout_ms": self._settings.get_timeout_ms(),
        }

    def _emit_serial_params_changed(self) -> None:
        self.serial_params_changed.emit(self._serial.is_connected(), self._params_dict())

    def test(self) -> None:
        """Placeholder."""
        logger.info("Test executed (stub)")
        self.info.emit("Test executed (stub).")

    def connect_with(
        self,
        port: str,
        baud: int,
        parity: str,
        data_bits: int,
        stop_bits: str,
        timeout_ms: int,
    ) -> bool:
        """Update AppSettings, call SerialService.open(...); on success emit connected(port) and info(\"connected\") and return True; on failure emit error(\"connect failed\") and return False."""
        if not port or not port.strip():
            logger.error("connect failed")
            self.error.emit("connect failed")
            return False
        port = port.strip()
        self._settings.set_last_port(port)
        self._settings.set_baud(baud)
        self._settings.set_parity(parity)
        self._settings.set_data_bits(data_bits)
        self._settings.set_stop_bits(stop_bits)
        self._settings.set_timeout_ms(timeout_ms)
        if self._serial.open(port, baud=baud, timeout_ms=timeout_ms):
            logger.info("connected")
            self.connected.emit(port)
            self.info.emit("connected")
            self._emit_serial_params_changed()
            return True
        logger.error("connect failed")
        self.error.emit("connect failed")
        return False

    def set_last_connection(self, port: str, baud: int, timeout_ms: int) -> None:
        """Write port, baud, timeout_ms to AppSettings."""
        self._settings.set_last_port(port)
        self._settings.set_baud(baud)
        self._settings.set_timeout_ms(timeout_ms)
