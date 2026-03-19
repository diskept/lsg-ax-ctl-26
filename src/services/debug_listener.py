import logging
import threading
import time

from src.services.serial_service import SerialService

logger = logging.getLogger("debug_listener")


class DebugSerialListener:
    """Background reader for MCU debug UART -> logs lines/bytes."""

    def __init__(self) -> None:
        self._stop = threading.Event()
        self._thread: threading.Thread | None = None
        self._serial = SerialService()

    def start(self, port: str, baud: int = 38400, timeout_ms: int = 200) -> bool:
        if self.is_running():
            return True
        ok = self._serial.open(port, baud=baud, timeout_ms=timeout_ms)
        if not ok:
            return False
        self._stop.clear()
        self._thread = threading.Thread(target=self._run, name="DebugSerialListener", daemon=True)
        self._thread.start()
        logger.info("debug-listener started port=%s baud=%s", port, baud)
        return True

    def stop(self) -> None:
        self._stop.set()
        if self._thread is not None:
            self._thread.join(timeout=1.0)
        self._thread = None
        self._serial.close()
        logger.info("debug-listener stopped")

    def is_running(self) -> bool:
        return self._thread is not None and self._thread.is_alive()

    def _run(self) -> None:
        # Mixed firmware outputs happen; accept both lines and raw bytes.
        while not self._stop.is_set():
            try:
                line = self._serial.readline(4096)
                if line:
                    try:
                        text = line.decode("utf-8", errors="replace").rstrip("\r\n")
                    except Exception:
                        text = repr(line)
                    # Firmware prompt spam (Master_Actuator_OnTime_Process) — keep logs readable.
                    if text.strip() == ">>":
                        continue
                    logger.info("[FW] %s", text)
                    continue
                raw = self._serial.read_all()
                if raw:
                    logger.info("[FW:RAW] %s", raw.hex(" "))
                else:
                    time.sleep(0.02)
            except Exception as e:
                logger.exception("debug listener error: %s", e)
                time.sleep(0.2)

