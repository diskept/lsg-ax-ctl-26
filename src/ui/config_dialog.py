"""Configuration dialog for port, baudrate, timeout."""
from PyQt6.QtWidgets import (
    QComboBox,
    QDialog,
    QDialogButtonBox,
    QFormLayout,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QSpinBox,
    QTabWidget,
    QVBoxLayout,
    QWidget,
)

from src.core.settings import AppSettings
from src.services.serial_service import SerialService


class ConfigDialog(QDialog):
    """Dialog for connection settings. Loads from AppSettings on open, saves on OK."""

    def __init__(self, parent: QWidget | None = None):
        super().__init__(parent)
        self._settings = AppSettings()
        self._build_ui()

    def _build_ui(self) -> None:
        self.setWindowTitle("Settings")
        main_layout = QVBoxLayout(self)

        tabs = QTabWidget()

        # Serial tab
        serial_tab = QWidget()
        serial_layout = QFormLayout(serial_tab)
        self._port_combo = QComboBox()
        self._port_combo.setEditable(True)
        rescan_btn = QPushButton("Rescan")
        rescan_btn.clicked.connect(self._on_rescan)
        port_row = QWidget()
        port_row_layout = QHBoxLayout(port_row)
        port_row_layout.setContentsMargins(0, 0, 0, 0)
        port_row_layout.addWidget(self._port_combo)
        port_row_layout.addWidget(rescan_btn)
        serial_layout.addRow("Port:", port_row)

        self._baud_combo = QComboBox()
        self._baud_combo.addItems(["2400", "4800", "9600", "19200", "38400", "115200"])
        serial_layout.addRow("Baudrate:", self._baud_combo)

        self._parity_combo = QComboBox()
        self._parity_combo.addItems(["None", "Even", "Odd", "Mark", "Space"])
        serial_layout.addRow("Parity:", self._parity_combo)

        self._data_bits_combo = QComboBox()
        self._data_bits_combo.addItems(["5", "6", "7", "8"])
        serial_layout.addRow("Data bits:", self._data_bits_combo)

        self._stop_bits_combo = QComboBox()
        self._stop_bits_combo.addItems(["1", "1.5", "2"])
        serial_layout.addRow("Stop bits:", self._stop_bits_combo)

        self._timeout_spin = QSpinBox()
        self._timeout_spin.setRange(10, 10000)
        self._timeout_spin.setSingleStep(50)
        self._timeout_spin.setSuffix(" ms")
        serial_layout.addRow("Timeout (ms):", self._timeout_spin)

        tabs.addTab(serial_tab, "Serial")

        # General tab (placeholder)
        general_tab = QWidget()
        general_layout = QVBoxLayout(general_tab)
        general_layout.addWidget(QLabel("Placeholder for future options (e.g., language override)."))
        tabs.addTab(general_tab, "General")

        main_layout.addWidget(tabs)

        buttons = QDialogButtonBox(
            QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel
        )
        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)
        main_layout.addWidget(buttons)

    def refresh_ports(self) -> list[str]:
        """Call SerialService.list_ports(), clear and repopulate Port combo; return the list."""
        ports = SerialService().list_ports()
        self._port_combo.clear()
        self._port_combo.addItems(ports)
        return ports

    def _on_rescan(self) -> None:
        """Rescan ports, keep selection if still available."""
        current = self._port_combo.currentText().strip()
        ports = self.refresh_ports()
        if not ports:
            return
        idx = self._port_combo.findText(current)
        if idx >= 0:
            self._port_combo.setCurrentIndex(idx)

    def showEvent(self, event) -> None:
        super().showEvent(event)
        ports = self.refresh_ports()
        last_port = self._settings.get_last_port()
        if last_port:
            idx = self._port_combo.findText(last_port)
            if idx >= 0:
                self._port_combo.setCurrentIndex(idx)
            else:
                self._port_combo.setCurrentText(last_port)
        idx_baud = self._baud_combo.findText(str(self._settings.get_baud()))
        if idx_baud >= 0:
            self._baud_combo.setCurrentIndex(idx_baud)
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

    def accept(self) -> None:
        port = self._port_combo.currentText().strip()
        if port:
            self._settings.set_last_port(port)
        self._settings.set_baud(int(self._baud_combo.currentText()))
        self._settings.set_timeout_ms(self._timeout_spin.value())
        self._settings.set_parity(self._parity_combo.currentText())
        self._settings.set_data_bits(int(self._data_bits_combo.currentText()))
        self._settings.set_stop_bits(self._stop_bits_combo.currentText())
        super().accept()
