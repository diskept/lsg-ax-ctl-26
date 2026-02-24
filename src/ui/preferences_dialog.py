"""Settings dialog with three sections: Serial, Node, Paths (no business logic)."""
from PyQt6.QtWidgets import (
    QComboBox,
    QDialog,
    QDialogButtonBox,
    QFormLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QListWidget,
    QListWidgetItem,
    QPushButton,
    QRadioButton,
    QSpinBox,
    QStackedWidget,
    QVBoxLayout,
    QWidget,
)

from src.services.serial_service import SerialService


class SerialSettingsPage(QWidget):
    """Serial: Port, Baudrate, Parity, Data bits, Stop bits, Timeout (ms). No INI/Log paths."""

    def __init__(self, parent: QWidget | None = None):
        super().__init__(parent)
        layout = QFormLayout(self)

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

        rescan_btn.clicked.connect(self._on_rescan)

    def _on_rescan(self) -> None:
        ports = SerialService().list_ports()
        self._port_combo.clear()
        self._port_combo.addItems(ports)


class NodeSettingsPage(QWidget):
    """Node: Sensor (Indoor/Outdoor + second level), Actuator (3 radios), optional placeholders."""

    def __init__(self, parent: QWidget | None = None):
        super().__init__(parent)
        layout = QFormLayout(self)

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
    """Paths: Settings (INI) path, Log folder path; placeholder for Language override."""

    def __init__(self, parent: QWidget | None = None):
        super().__init__(parent)
        layout = QFormLayout(self)

        self._ini_path_edit = QLineEdit()
        browse_ini = QPushButton("Browse...")
        ini_row = QWidget()
        ini_row_layout = QHBoxLayout(ini_row)
        ini_row_layout.setContentsMargins(0, 0, 0, 0)
        ini_row_layout.addWidget(self._ini_path_edit)
        ini_row_layout.addWidget(browse_ini)
        layout.addRow("Settings (INI) file path:", ini_row)

        self._log_path_edit = QLineEdit()
        browse_log = QPushButton("Browse...")
        log_row = QWidget()
        log_row_layout = QHBoxLayout(log_row)
        log_row_layout.setContentsMargins(0, 0, 0, 0)
        log_row_layout.addWidget(self._log_path_edit)
        log_row_layout.addWidget(browse_log)
        layout.addRow("Log folder path:", log_row)

        layout.addRow(QLabel("Language override (future)"))


class PreferencesDialog(QDialog):
    """Settings dialog: left list (Serial, Node, Paths), right stacked pages, OK/Cancel."""

    def __init__(self, parent: QWidget | None = None):
        super().__init__(parent)
        self.setWindowTitle("Settings")
        self._build_ui()

    def _build_ui(self) -> None:
        outer = QVBoxLayout(self)

        content = QWidget()
        main_layout = QHBoxLayout(content)
        main_layout.setContentsMargins(0, 0, 0, 0)

        self._list = QListWidget()
        self._list.addItem(QListWidgetItem("Serial"))
        self._list.addItem(QListWidgetItem("Node"))
        self._list.addItem(QListWidgetItem("Paths"))
        self._list.setCurrentRow(0)
        main_layout.addWidget(self._list)

        self._stack = QStackedWidget()
        self._stack.addWidget(SerialSettingsPage(self))
        self._stack.addWidget(NodeSettingsPage(self))
        self._stack.addWidget(PathsSettingsPage(self))
        main_layout.addWidget(self._stack, 1)

        self._list.currentRowChanged.connect(self._stack.setCurrentIndex)

        outer.addWidget(content)

        buttons = QDialogButtonBox(
            QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel
        )
        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)
        outer.addWidget(buttons)
