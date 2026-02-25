"""Node connect step — sensor/actuator options; save to AppSettings then caller performs connect."""
from PyQt6.QtWidgets import (
    QComboBox,
    QDialog,
    QDialogButtonBox,
    QFormLayout,
    QGroupBox,
    QLineEdit,
    QRadioButton,
    QVBoxLayout,
    QWidget,
)

from src.core.settings import AppSettings


class NodeConnectDialog(QDialog):
    """Step 2: Set node options (sensor/actuator type, Node Address, Protocol). Saves to AppSettings on OK."""

    def __init__(self, settings: AppSettings, parent: QWidget | None = None):
        super().__init__(parent)
        self._settings = settings
        self._build_ui()

    def _build_ui(self) -> None:
        self.setWindowTitle("Connect — Node")
        layout = QFormLayout(self)
        layout.setVerticalSpacing(7)

        sensor_group = QGroupBox("Sensor Node type")
        sensor_layout = QVBoxLayout(sensor_group)
        self._sensor_indoor_hort = QRadioButton("Indoor → Horticulture")
        self._sensor_indoor_live = QRadioButton("Indoor → Livestock")
        self._sensor_outdoor = QRadioButton("Outdoor → Common")
        sensor_layout.addWidget(self._sensor_indoor_hort)
        sensor_layout.addWidget(self._sensor_indoor_live)
        sensor_layout.addWidget(self._sensor_outdoor)
        layout.addRow(sensor_group)

        self._actuator_combo = QComboBox()
        self._actuator_combo.addItems([
            "16 Switch + 8 Actuator",
            "16 Switch + 16 Actuator (2×16CH)",
            "16 Switch + 16 Actuator (1×32CH)",
        ])
        layout.addRow("Actuator Node type:", self._actuator_combo)

        self._node_address_edit = QLineEdit()
        self._node_address_edit.setPlaceholderText("Optional")
        layout.addRow("Node Address:", self._node_address_edit)

        self._protocol_combo = QComboBox()
        self._protocol_combo.addItems(["1.0", "1.1", "2.0"])
        layout.addRow("Protocol Version:", self._protocol_combo)

        buttons = QDialogButtonBox(
            QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel
        )
        buttons.button(QDialogButtonBox.StandardButton.Ok).setText("Connect")
        buttons.accepted.connect(self._on_ok)
        buttons.rejected.connect(self.reject)
        layout.addRow(buttons)

    def showEvent(self, event) -> None:
        super().showEvent(event)
        st = self._settings.get_sensor_type()
        if st == "Indoor/Horticulture":
            self._sensor_indoor_hort.setChecked(True)
        elif st == "Indoor/Livestock":
            self._sensor_indoor_live.setChecked(True)
        else:
            self._sensor_outdoor.setChecked(True)
        idx = self._actuator_combo.findText(self._settings.get_actuator_type())
        if idx >= 0:
            self._actuator_combo.setCurrentIndex(idx)
        self._node_address_edit.setText(self._settings.get_node_address())
        idx_p = self._protocol_combo.findText(self._settings.get_protocol_version())
        if idx_p >= 0:
            self._protocol_combo.setCurrentIndex(idx_p)

    def _on_ok(self) -> None:
        if self._sensor_indoor_hort.isChecked():
            self._settings.set_sensor_type("Indoor/Horticulture")
        elif self._sensor_indoor_live.isChecked():
            self._settings.set_sensor_type("Indoor/Livestock")
        else:
            self._settings.set_sensor_type("Outdoor/Common")
        self._settings.set_actuator_type(self._actuator_combo.currentText())
        self._settings.set_node_address(self._node_address_edit.text().strip())
        self._settings.set_protocol_version(self._protocol_combo.currentText())
        self.accept()
