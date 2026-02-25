"""Silent file logging infrastructure — rotating log files, no on-screen log panel."""
import logging
import os

from logging.handlers import TimedRotatingFileHandler

from PyQt6.QtCore import QStandardPaths


_log_dir: str | None = None


def init_logging(app_name: str = "AxiomCanon") -> dict:
    """Configure Python logging with rotating file handler. Return log_dir and log_file paths."""
    global _log_dir
    base = QStandardPaths.writableLocation(QStandardPaths.StandardLocation.AppDataLocation)
    log_dir = os.path.join(base, "LSG", app_name, "logs")
    os.makedirs(log_dir, exist_ok=True)
    _log_dir = os.path.abspath(log_dir)

    log_file = os.path.join(log_dir, "app.log")

    root = logging.getLogger()
    root.setLevel(logging.INFO)
    for h in root.handlers[:]:
        root.removeHandler(h)

    handler = TimedRotatingFileHandler(
        log_file,
        when="midnight",
        backupCount=7,
        encoding="utf-8",
    )
    handler.setLevel(logging.INFO)
    formatter = logging.Formatter("%(asctime)s | %(levelname)s | %(name)s | %(message)s")
    handler.setFormatter(formatter)
    root.addHandler(handler)

    return {"log_dir": log_dir, "log_file": log_file}


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
