"""Silent file logging infrastructure.

Log files are written under:

    <AppData>/LSG/<app_name>/logs/YYYY/MM/YYYY-MM-DD.log

날짜가 바뀌면 자동으로 새로운 파일을 열어 이어서 기록합니다.
"""
import logging
import os
from datetime import datetime

from PyQt6.QtCore import QStandardPaths


_log_dir: str | None = None


class DailyDirectoryFileHandler(logging.Handler):
    """Logging handler that creates logs/YYYY/MM/YYYY-MM-DD.log and rolls at midnight."""

    def __init__(self, root_log_dir: str) -> None:
        super().__init__(level=logging.INFO)
        self._root_log_dir = os.path.abspath(root_log_dir)
        self._stream: logging.StreamHandler | None = None
        self._current_date: str | None = None  # YYYY-MM-DD

    def _current_paths(self) -> tuple[str, str]:
        """Return (day_dir, file_path)."""
        now = datetime.now()
        year = f"{now.year:04d}"
        month = f"{now.month:02d}"
        day = f"{now.day:02d}"
        day_dir = os.path.join(self._root_log_dir, year, month)
        file_path = os.path.join(day_dir, f"{year}-{month}-{day}.log")
        return day_dir, file_path

    def _ensure_stream(self) -> None:
        day_dir, file_path = self._current_paths()
        date_str = os.path.basename(file_path).split(".")[0]
        if self._stream is not None and self._current_date == date_str:
            return
        # 날짜 변경 또는 최초 호출: 기존 스트림 정리 후 새 파일 열기
        if self._stream is not None:
            try:
                self._stream.close()
            except Exception:
                pass
            self._stream = None

        os.makedirs(day_dir, exist_ok=True)
        self._current_date = date_str
        # mode="a" append
        file_handler = logging.FileHandler(file_path, mode="a", encoding="utf-8")
        # FileHandler도 Handler이지만 여기서는 내부 위임용으로만 사용
        self._stream = file_handler  # type: ignore[assignment]

    def emit(self, record: logging.LogRecord) -> None:
        try:
            self._ensure_stream()
            if self._stream is None:
                return
            msg = self.format(record)
            # FileHandler의 stream.write 사용
            stream = self._stream.stream  # type: ignore[attr-defined]
            stream.write(msg + "\n")
            stream.flush()
        except Exception:
            self.handleError(record)

    def close(self) -> None:
        if self._stream is not None:
            try:
                self._stream.close()
            except Exception:
                pass
            self._stream = None
        super().close()


def init_logging(app_name: str = "AxiomCanon") -> dict:
    """Configure Python logging with daily directory/file layout. Return log_dir and last log_file path."""
    global _log_dir
    base = QStandardPaths.writableLocation(QStandardPaths.StandardLocation.AppDataLocation)
    log_dir = os.path.join(base, "LSG", app_name, "logs")
    os.makedirs(log_dir, exist_ok=True)
    _log_dir = os.path.abspath(log_dir)

    root = logging.getLogger()
    root.setLevel(logging.INFO)
    for h in root.handlers[:]:
        root.removeHandler(h)

    handler = DailyDirectoryFileHandler(log_dir)
    formatter = logging.Formatter("%(asctime)s | %(levelname)s | %(name)s | %(message)s")
    handler.setFormatter(formatter)
    root.addHandler(handler)

    # 현재 날짜 기준 마지막 파일 경로를 함께 반환 (편의용)
    day_dir, file_path = handler._current_paths()
    return {"log_dir": day_dir, "log_file": file_path}


def get_log_dir() -> str | None:
    """Return the absolute log directory path set by init_logging(), or None if not yet initialized."""
    if _log_dir is None:
        return None
    return os.path.abspath(_log_dir)


def get_default_log_dir(app_name: str = "AxiomCanon") -> str:
    """Return the default log directory path as absolute (used when init_logging has not been called)."""
    base = QStandardPaths.writableLocation(
        QStandardPaths.StandardLocation.AppDataLocation
    )
    log_dir = os.path.join(base, "LSG", app_name, "logs")
    return os.path.abspath(log_dir)
