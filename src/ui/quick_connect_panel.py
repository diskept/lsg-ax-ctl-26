"""Quick Connect panel for dock — serial params and Connect button."""
from PyQt6.QtCore import pyqtSignal
from PyQt6.QtWidgets import (
    QComboBox,
    QFormLayout,
    QHBoxLayout,
    QPushButton,
    QSpinBox,
    QWidget,
)

from src.core.settings import AppSettings
from src.services.serial_service import SerialService


class QuickConnectPanel(QWidget):
    """Serial params form prefilled from AppSettings; Connect button invokes connect_requested."""

    connect_requested = pyqtSignal()

    def __init__(self, settings: AppSettings, parent: QWidget | None = None):
        super().__init__(parent)
        self._settings = settings
        self._build_ui()

    def _build_ui(self) -> None:
        layout = QFormLayout(self)

        self._port_combo = QComboBox()
        self._port_combo.setEditable(True)
        rescan_btn = QPushButton("Rescan")
        rescan_btn.clicked.connect(self._on_rescan)
        port_row = QWidget()
        port_row_layout = QHBoxLayout(port_row)
        port_row_layout.setContentsMargins(0, 0, 0, 0)
        port_row_layout.addWidget(self._port_combo)
        port_row_layout.addWidget(rescan_btn)
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

        connect_btn = QPushButton("Connect")
        connect_btn.clicked.connect(lambda: self.connect_requested.emit())
        layout.addRow(connect_btn)

    def _on_rescan(self) -> None:
        ports = SerialService().list_ports()
        self._port_combo.clear()
        self._port_combo.addItems(ports)
        current = self._port_combo.currentText().strip()
        if current:
            idx = self._port_combo.findText(current)
            if idx >= 0:
                self._port_combo.setCurrentIndex(idx)

    def load_from_settings(self) -> None:
        """Prefill from AppSettings."""
        ports = SerialService().list_ports()
        self._port_combo.clear()
        self._port_combo.addItems(ports)
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

    def showEvent(self, event) -> None:
        super().showEvent(event)
        self.load_from_settings()

    def get_port(self) -> str:
        return self._port_combo.currentText().strip()

    def get_baud(self) -> int:
        return int(self._baud_combo.currentText())

    def get_parity(self) -> str:
        return self._parity_combo.currentText()

    def get_data_bits(self) -> int:
        return int(self._data_bits_combo.currentText())

    def get_stop_bits(self) -> str:
        return self._stop_bits_combo.currentText()

    def get_timeout_ms(self) -> int:
        return self._timeout_spin.value()
