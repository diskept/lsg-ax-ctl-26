"""Application settings via QSettings (persisted to .ini file)."""
import os

from PyQt6.QtCore import QSettings, QStandardPaths


DEFAULT_BAUD = 115200
DEFAULT_TIMEOUT_MS = 1000
DEFAULT_PARITY = "None"
DEFAULT_DATA_BITS = 8
DEFAULT_STOP_BITS = "1"
DEFAULT_SENSOR_TYPE = "Indoor/Horticulture"
DEFAULT_ACTUATOR_TYPE = "16 Switch + 8 Actuator"
DEFAULT_PROTOCOL_VERSION = "1.0"
DEFAULT_LAST_NODE_GROUP = "sensor"
DEFAULT_DEBUG_PORT = "COM3"
DEFAULT_DEBUG_BAUD = 38400


def _settings_ini_path() -> str:
    """Return writable path: <AppConfig>/LSG/AxiomCanon/settings.ini."""
    base = QStandardPaths.writableLocation(QStandardPaths.StandardLocation.AppConfigLocation)
    config_dir = os.path.join(base, "LSG", "AxiomCanon")
    os.makedirs(config_dir, exist_ok=True)
    return os.path.join(config_dir, "settings.ini")


def get_settings_path() -> str:
    """Return the absolute path of the settings (INI) file used by QSettings."""
    return os.path.abspath(_settings_ini_path())


class AppSettings:
    """QSettings-backed configuration stored in a .ini file."""

    def __init__(self) -> None:
        ini_path = _settings_ini_path()
        self._q = QSettings(ini_path, QSettings.Format.IniFormat)

    def settings_path(self) -> str:
        """Return the absolute path of the settings (INI) file currently used by this instance."""
        return get_settings_path()

    def get_last_port(self) -> str | None:
        value = self._q.value("lastPort")
        return str(value) if value else None

    def set_last_port(self, port: str) -> None:
        self._q.setValue("lastPort", port)
        self._q.sync()

    def get_baud(self) -> int:
        value = self._q.value("baud")
        if value is None:
            return DEFAULT_BAUD
        try:
            return int(value)
        except (TypeError, ValueError):
            return DEFAULT_BAUD

    def set_baud(self, baud: int) -> None:
        self._q.setValue("baud", baud)
        self._q.sync()

    def get_timeout_ms(self) -> int:
        value = self._q.value("timeoutMs")
        if value is None:
            return DEFAULT_TIMEOUT_MS
        try:
            return int(value)
        except (TypeError, ValueError):
            return DEFAULT_TIMEOUT_MS

    def set_timeout_ms(self, timeout_ms: int) -> None:
        self._q.setValue("timeoutMs", timeout_ms)
        self._q.sync()

    def get_parity(self) -> str:
        value = self._q.value("parity")
        return str(value) if value else DEFAULT_PARITY

    def set_parity(self, parity: str) -> None:
        self._q.setValue("parity", parity)
        self._q.sync()

    def get_data_bits(self) -> int:
        value = self._q.value("dataBits")
        if value is None:
            return DEFAULT_DATA_BITS
        try:
            return int(value)
        except (TypeError, ValueError):
            return DEFAULT_DATA_BITS

    def set_data_bits(self, data_bits: int) -> None:
        self._q.setValue("dataBits", data_bits)
        self._q.sync()

    def get_stop_bits(self) -> str:
        value = self._q.value("stopBits")
        return str(value) if value else DEFAULT_STOP_BITS

    def set_stop_bits(self, stop_bits: str) -> None:
        self._q.setValue("stopBits", stop_bits)
        self._q.sync()

    def get_sensor_type(self) -> str:
        value = self._q.value("sensorType")
        return str(value) if value else DEFAULT_SENSOR_TYPE

    def set_sensor_type(self, sensor_type: str) -> None:
        self._q.setValue("sensorType", sensor_type)
        self._q.sync()

    def get_actuator_type(self) -> str:
        value = self._q.value("actuatorType")
        return str(value) if value else DEFAULT_ACTUATOR_TYPE

    def set_actuator_type(self, actuator_type: str) -> None:
        self._q.setValue("actuatorType", actuator_type)
        self._q.sync()

    def get_ocs_channel_count(self) -> int:
        """연결된 구동기 타입에 따른 개폐기(OCS) 채널 수. 8 또는 16."""
        t = (self.get_actuator_type() or "").strip()
        if "16 Actuator" in t or "1×32CH" in t or "1x32CH" in t or "2×16CH" in t or "2x16CH" in t:
            return 16
        return 8

    def get_node_address(self) -> str:
        value = self._q.value("nodeAddress")
        return str(value) if value else ""

    def set_node_address(self, node_address: str) -> None:
        self._q.setValue("nodeAddress", node_address)
        self._q.sync()

    def get_last_node_group(self) -> str:
        """Return last connected node group: 'sensor' or 'actuator'."""
        value = self._q.value("lastNodeGroup")
        return str(value) if value in ("sensor", "actuator") else DEFAULT_LAST_NODE_GROUP

    def set_last_node_group(self, group: str) -> None:
        self._q.setValue("lastNodeGroup", group)
        self._q.sync()

    def get_protocol_version(self) -> str:
        value = self._q.value("protocolVersion")
        return str(value) if value else DEFAULT_PROTOCOL_VERSION

    def set_protocol_version(self, protocol_version: str) -> None:
        self._q.setValue("protocolVersion", protocol_version)
        self._q.sync()

    def get_debug_port(self) -> str:
        value = self._q.value("debugPort")
        return str(value) if value else DEFAULT_DEBUG_PORT

    def set_debug_port(self, port: str) -> None:
        self._q.setValue("debugPort", port)
        self._q.sync()

    def get_debug_baud(self) -> int:
        value = self._q.value("debugBaud")
        if value is None:
            return DEFAULT_DEBUG_BAUD
        try:
            return int(value)
        except (TypeError, ValueError):
            return DEFAULT_DEBUG_BAUD

    def set_debug_baud(self, baud: int) -> None:
        self._q.setValue("debugBaud", int(baud))
        self._q.sync()
