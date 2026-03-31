"""NodeManager service — scan/connect/test using SerialService and AppSettings."""
import logging

from PyQt6.QtCore import QObject, QTimer, pyqtSignal

from src.core.settings import AppSettings
from src.services.actuator_test_sequence import ActuatorTimedSequence
from src.services.debug_listener import DebugSerialListener
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
    comm_health = pyqtSignal(str)  # short line for status bar (FW debug age, Modbus streak)

    def __init__(self, parent: QObject | None = None):
        super().__init__(parent)
        self._serial = SerialService(self)
        self._settings = AppSettings()
        self._debug = DebugSerialListener()
        self._act_test = ActuatorTimedSequence(self._serial, self._settings)
        self._health_timer = QTimer(self)
        self._health_timer.setInterval(1500)
        self._health_timer.timeout.connect(self._emit_comm_health)

    def is_connected(self) -> bool:
        """Return whether the serial service is currently connected."""
        return self._serial.is_connected()

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

    def connect(self) -> bool:
        """Read from AppSettings and call connect_with(...). Return True on success."""
        port = self._settings.get_last_port()
        if not port or not port.strip():
            logger.error("connect failed")
            self.error.emit("connect failed")
            return False
        return self.connect_with(
            port.strip(),
            self._settings.get_baud(),
            self._settings.get_parity(),
            self._settings.get_data_bits(),
            self._settings.get_stop_bits(),
            self._settings.get_timeout_ms(),
        )

    def disconnect(self) -> None:
        """Close the service and emit disconnected, info, serial_params_changed. 테스트 시퀀스 정지."""
        logger.info("disconnect")
        try:
            self.stop_firmware_debug_log()
        except Exception:
            pass
        if self._act_test.is_running():
            self._act_test.stop()
            logger.info("test sequence stopped by disconnect")
        self._serial.close()
        self._health_timer.stop()
        self.comm_health.emit("")
        self.disconnected.emit()
        self.info.emit("disconnect")
        self._emit_serial_params_changed()

    def start_firmware_debug_log(self) -> bool:
        """Start reading firmware debug UART and write to app log."""
        port = self._settings.get_debug_port()
        baud = self._settings.get_debug_baud()
        ok = self._debug.start(port=port, baud=baud, timeout_ms=200)
        if ok:
            self.info.emit(f"FW debug listening: {port} @ {baud}")
        else:
            self.error.emit(f"FW debug open failed: {port} @ {baud}")
        return ok

    def stop_firmware_debug_log(self) -> None:
        if self._debug.is_running():
            self._debug.stop()
            self.info.emit("FW debug listener stopped")

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

    def _emit_comm_health(self) -> None:
        if not self._serial.is_connected():
            return
        parts: list[str] = []
        idle = self._debug.idle_seconds()
        if idle is not None:
            if idle >= 12.0:
                parts.append(f"FW DBG idle {idle:.0f}s !")
            else:
                parts.append(f"FW DBG {idle:.1f}s")
        if self._act_test.is_running() and self._act_test.modbus_fail_streak > 0:
            parts.append(f"MB fail ×{self._act_test.modbus_fail_streak}")
        self.comm_health.emit(" · ".join(parts) if parts else " comm OK")

    def test(self) -> None:
        """Run actuator OCS timed test sequence (1..16)."""
        if not self._serial.is_connected():
            self.error.emit("Not connected")
            return
        # Ensure debug capture is running (best-effort).
        try:
            self.start_firmware_debug_log()
        except Exception:
            pass
        if self._act_test.is_running():
            self.info.emit("Test already running")
            return
        ok = self._act_test.start()
        if ok:
            self.info.emit("타임 동작 테스트 시작 (종료 없음, 매 분 OCS 1초 간격)")
        else:
            self.error.emit("Failed to start test")

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
            if self._act_test.is_running():
                self._act_test.stop()
                logger.info("test sequence reset by connect")
            self.connected.emit(port)
            self.info.emit("connected")
            self._emit_serial_params_changed()
            try:
                self.start_firmware_debug_log()
            except Exception:
                pass
            self._health_timer.start()
            return True
        logger.error("connect failed")
        self.error.emit("connect failed")
        return False

    def set_last_connection(self, port: str, baud: int, timeout_ms: int) -> None:
        """Write port, baud, timeout_ms to AppSettings."""
        self._settings.set_last_port(port)
        self._settings.set_baud(baud)
        self._settings.set_timeout_ms(timeout_ms)
