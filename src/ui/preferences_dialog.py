"""Settings dialog with tabbed UI: Serial, Paths (Node tab hidden for future)."""
from PyQt6.QtWidgets import (
    QComboBox,
    QDialog,
    QDialogButtonBox,
    QFileDialog,
    QFormLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QPushButton,
    QRadioButton,
    QSpinBox,
    QTabWidget,
    QVBoxLayout,
    QWidget,
)

from src.core.logging_setup import get_default_log_dir, get_log_dir
from src.core.settings import AppSettings, get_settings_path
from src.services.serial_service import SerialService


class SerialSettingsPage(QWidget):
    """Serial: Port, Baudrate, Parity, Data bits, Stop bits, Timeout (ms)."""

    def __init__(self, settings: AppSettings, parent: QWidget | None = None):
        super().__init__(parent)
        self._settings = settings
        layout = QFormLayout(self)
        layout.setVerticalSpacing(7)

        self._port_combo = QComboBox()
        self._port_combo.setEditable(True)
        rescan_btn = QPushButton("Rescan")
        port_row = QWidget()
        row_layout = QHBoxLayout(port_row)
        row_layout.setContentsMargins(0, 0, 0, 0)
        row_layout.addWidget(self._port_combo)
        row_layout.addWidget(rescan_btn)
        layout.addRow("Port:", port_row)

        self._baud_combo = QComboBox()
        self._baud_combo.addItems(["2400", "4800", "9600", "19200", "38400", "115200"])
        layout.addRow("Baudrate:", self._baud_combo)

        self._parity_combo = QComboBox()
        self._parity_combo.addItems(["None", "Even", "Odd", "Mark", "Space"])
        layout.addRow("Parity:", self._parity_combo)

        self._data_bits_combo = QComboBox()
        self._data_bits_combo.addItems(["5", "6", "7", "8"])
        layout.addRow("Data bits:", self._data_bits_combo)

        self._stop_bits_combo = QComboBox()
        self._stop_bits_combo.addItems(["1", "1.5", "2"])
        layout.addRow("Stop bits:", self._stop_bits_combo)

        self._timeout_spin = QSpinBox()
        self._timeout_spin.setRange(10, 10000)
        self._timeout_spin.setSingleStep(50)
        self._timeout_spin.setSuffix(" ms")
        layout.addRow("Timeout (ms):", self._timeout_spin)

        self._dbg_port_combo = QComboBox()
        self._dbg_port_combo.setEditable(True)
        layout.addRow("FW Debug Port:", self._dbg_port_combo)

        self._dbg_baud_combo = QComboBox()
        self._dbg_baud_combo.addItems(["38400"])
        layout.addRow("FW Debug Baud:", self._dbg_baud_combo)

        rescan_btn.clicked.connect(self._on_rescan)

    def _refresh_ports(self) -> None:
        ports = SerialService().list_ports()
        self._port_combo.clear()
        self._port_combo.addItems(ports)
        self._dbg_port_combo.clear()
        self._dbg_port_combo.addItems(ports)

    def _on_rescan(self) -> None:
        current = self._port_combo.currentText().strip()
        self._refresh_ports()
        if current:
            idx = self._port_combo.findText(current)
            if idx >= 0:
                self._port_combo.setCurrentIndex(idx)
            else:
                self._port_combo.setCurrentText(current)

    def load_from_settings(self) -> None:
        self._refresh_ports()
        last_port = self._settings.get_last_port()
        if last_port:
            idx = self._port_combo.findText(last_port)
            if idx >= 0:
                self._port_combo.setCurrentIndex(idx)
            else:
                self._port_combo.setCurrentText(last_port)
        dbg_port = self._settings.get_debug_port()
        if dbg_port:
            idx = self._dbg_port_combo.findText(dbg_port)
            if idx >= 0:
                self._dbg_port_combo.setCurrentIndex(idx)
            else:
                self._dbg_port_combo.setCurrentText(dbg_port)
        idx_baud = self._baud_combo.findText(str(self._settings.get_baud()))
        if idx_baud >= 0:
            self._baud_combo.setCurrentIndex(idx_baud)
        idx_dbg_baud = self._dbg_baud_combo.findText(str(self._settings.get_debug_baud()))
        if idx_dbg_baud >= 0:
            self._dbg_baud_combo.setCurrentIndex(idx_dbg_baud)
        idx_parity = self._parity_combo.findText(self._settings.get_parity())
        if idx_parity >= 0:
            self._parity_combo.setCurrentIndex(idx_parity)
        idx_data = self._data_bits_combo.findText(str(self._settings.get_data_bits()))
        if idx_data >= 0:
            self._data_bits_combo.setCurrentIndex(idx_data)
        idx_stop = self._stop_bits_combo.findText(self._settings.get_stop_bits())
        if idx_stop >= 0:
            self._stop_bits_combo.setCurrentIndex(idx_stop)
        self._timeout_spin.setValue(self._settings.get_timeout_ms())

    def save_to_settings(self) -> None:
        self._settings.set_last_port(self._port_combo.currentText().strip())
        self._settings.set_baud(int(self._baud_combo.currentText()))
        self._settings.set_parity(self._parity_combo.currentText())
        self._settings.set_data_bits(int(self._data_bits_combo.currentText()))
        self._settings.set_stop_bits(self._stop_bits_combo.currentText())
        self._settings.set_timeout_ms(self._timeout_spin.value())
        self._settings.set_debug_port(self._dbg_port_combo.currentText().strip())
        self._settings.set_debug_baud(int(self._dbg_baud_combo.currentText()))


class NodeSettingsPage(QWidget):
    """Node: Sensor (Indoor/Outdoor + second level), Actuator (3 radios). Kept for future; no tab in UI."""

    def __init__(self, parent: QWidget | None = None):
        super().__init__(parent)
        layout = QFormLayout(self)
        layout.setVerticalSpacing(7)

        sensor_group = QGroupBox("Sensor Node type")
        sensor_layout = QVBoxLayout(sensor_group)
        self._sensor_indoor = QRadioButton("Indoor")
        self._sensor_outdoor = QRadioButton("Outdoor")
        self._sensor_indoor.setChecked(True)
        sensor_layout.addWidget(self._sensor_indoor)
        self._sensor_indoor_combo = QComboBox()
        self._sensor_indoor_combo.addItems(["Horticulture", "Livestock"])
        sensor_layout.addWidget(self._sensor_indoor_combo)
        sensor_layout.addWidget(self._sensor_outdoor)
        self._sensor_outdoor_label = QLabel("Common (Horticulture/Livestock)")
        sensor_layout.addWidget(self._sensor_outdoor_label)
        layout.addRow(sensor_group)

        actuator_group = QGroupBox("Actuator Node type")
        actuator_layout = QVBoxLayout(actuator_group)
        self._actuator_8 = QRadioButton("16 Switch + 8 Actuator")
        self._actuator_16_2 = QRadioButton("16 Switch + 16 Actuator (2×16CH relay boards)")
        self._actuator_16_1 = QRadioButton("16 Switch + 16 Actuator (1×32CH relay board)")
        self._actuator_8.setChecked(True)
        actuator_layout.addWidget(self._actuator_8)
        actuator_layout.addWidget(self._actuator_16_2)
        actuator_layout.addWidget(self._actuator_16_1)
        layout.addRow(actuator_group)

        self._node_address_edit = QLineEdit()
        self._node_address_edit.setPlaceholderText("Optional")
        layout.addRow("Node Address:", self._node_address_edit)

        self._protocol_combo = QComboBox()
        self._protocol_combo.addItems(["1.0", "1.1", "2.0"])
        layout.addRow("Protocol Version:", self._protocol_combo)


class PathsSettingsPage(QWidget):
    """Paths: Settings (INI) path, Log folder path; Language override placeholder (hidden)."""

    def __init__(self, parent: QWidget | None = None):
        super().__init__(parent)
        layout = QFormLayout(self)
        layout.setVerticalSpacing(7)

        self._ini_path_edit = QLineEdit()
        browse_ini = QPushButton("Browse...")
        browse_ini.clicked.connect(self._on_browse_ini)
        ini_row = QWidget()
        ini_row_layout = QHBoxLayout(ini_row)
        ini_row_layout.setContentsMargins(0, 0, 0, 0)
        ini_row_layout.addWidget(self._ini_path_edit)
        ini_row_layout.addWidget(browse_ini)
        layout.addRow("Settings (INI) file path:", ini_row)

        self._log_path_edit = QLineEdit()
        browse_log = QPushButton("Browse...")
        browse_log.clicked.connect(self._on_browse_log)
        log_row = QWidget()
        log_row_layout = QHBoxLayout(log_row)
        log_row_layout.setContentsMargins(0, 0, 0, 0)
        log_row_layout.addWidget(self._log_path_edit)
        log_row_layout.addWidget(browse_log)
        layout.addRow("Log folder path:", log_row)

        self._lang_override_placeholder = QLabel("Language override (future)")
        self._lang_override_placeholder.hide()

    def _on_browse_ini(self) -> None:
        path, _ = QFileDialog.getSaveFileName(
            self, "Select Settings File", self._ini_path_edit.text(), "INI (*.ini)"
        )
        if path:
            self._ini_path_edit.setText(path)

    def _on_browse_log(self) -> None:
        path = QFileDialog.getExistingDirectory(
            self, "Select Log Folder", self._log_path_edit.text()
        )
        if path:
            self._log_path_edit.setText(path)

    def load_paths(self, settings: AppSettings | None = None) -> None:
        ini_path = (
            settings.settings_path() if settings else get_settings_path()
        )
        self._ini_path_edit.setText(ini_path)
        log_dir = get_log_dir()
        self._log_path_edit.setText(log_dir if log_dir else get_default_log_dir())


class PreferencesDialog(QDialog):
    """Settings dialog: QTabWidget (Serial, Paths), OK/Cancel."""

    def __init__(
        self, settings: AppSettings | None = None, parent: QWidget | None = None
    ):
        super().__init__(parent)
        self._settings = settings or AppSettings()
        self.setWindowTitle("Settings")
        self._build_ui()

    def _build_ui(self) -> None:
        outer = QVBoxLayout(self)

        self._tabs = QTabWidget()
        self._serial_page = SerialSettingsPage(self._settings, self)
        self._paths_page = PathsSettingsPage(self)
        self._tabs.addTab(self._serial_page, "Serial")
        self._tabs.addTab(self._paths_page, "Paths")
        outer.addWidget(self._tabs)

        buttons = QDialogButtonBox(
            QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel
        )
        buttons.accepted.connect(self._on_ok)
        buttons.rejected.connect(self.reject)
        outer.addWidget(buttons)

    def showEvent(self, event) -> None:
        super().showEvent(event)
        self._serial_page.load_from_settings()
        self._paths_page.load_paths(self._settings)

    def _on_ok(self) -> None:
        self._serial_page.save_to_settings()
        self.accept()
