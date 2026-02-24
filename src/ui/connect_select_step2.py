"""Step 2: Select node option based on group (Sensor sub-options or Actuator type)."""
from PyQt6.QtWidgets import (
    QComboBox,
    QDialog,
    QDialogButtonBox,
    QLabel,
    QPushButton,
    QRadioButton,
    QVBoxLayout,
    QWidget,
)


class ConnectSelectStep2(QDialog):
    """Select Node Option. For Sensor: Indoor/Outdoor + sub; for Actuator: 3 radio types. Exposes selection via attributes."""

    def __init__(self, group: str, parent: QWidget | None = None):
        super().__init__(parent)
        self.setWindowTitle("Select Node Option")
        self._group = group  # "sensor" or "actuator"
        self._selected_option_text = ""  # Human-readable summary for status bar
        self._build_ui()

    def _build_ui(self) -> None:
        layout = QVBoxLayout(self)

        if self._group == "sensor":
            self._sensor_indoor = QRadioButton("Indoor")
            self._sensor_outdoor = QRadioButton("Outdoor")
            self._sensor_indoor.setChecked(True)
            layout.addWidget(self._sensor_indoor)
            self._sensor_indoor_combo = QComboBox()
            self._sensor_indoor_combo.addItems(["Horticulture", "Livestock"])
            layout.addWidget(self._sensor_indoor_combo)
            layout.addWidget(self._sensor_outdoor)
            self._sensor_outdoor_label = QLabel("Common (Horticulture/Livestock)")
            layout.addWidget(self._sensor_outdoor_label)
        else:
            self._actuator_8 = QRadioButton("16 Switch + 8 Actuator")
            self._actuator_16_2 = QRadioButton("16 Switch + 16 Actuator (2×16CH relay boards)")
            self._actuator_16_1 = QRadioButton("16 Switch + 16 Actuator (1×32CH relay board)")
            self._actuator_8.setChecked(True)
            layout.addWidget(self._actuator_8)
            layout.addWidget(self._actuator_16_2)
            layout.addWidget(self._actuator_16_1)

        buttons = QDialogButtonBox(
            QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel
        )
        back_btn = QPushButton("Back")
        back_btn.clicked.connect(lambda: self.done(ConnectSelectStep2.RESULT_BACK))
        buttons.addButton(back_btn, QDialogButtonBox.ButtonRole.ActionRole)
        buttons.accepted.connect(self._on_ok)
        buttons.rejected.connect(self.reject)
        layout.addWidget(buttons)

    RESULT_BACK = 2  # custom result for Back (must not equal Accepted)

    def _on_ok(self) -> None:
        if self._group == "sensor":
            if self._sensor_indoor.isChecked():
                sub = self._sensor_indoor_combo.currentText()
                self._selected_option_text = f"Sensor - Indoor/{sub}"
            else:
                self._selected_option_text = "Sensor - Outdoor/Common (Horticulture/Livestock)"
        else:
            if self._actuator_8.isChecked():
                self._selected_option_text = "Actuator - 16 Switch + 8 Actuator"
            elif self._actuator_16_2.isChecked():
                self._selected_option_text = "Actuator - 16 Switch + 16 Actuator (2×16CH relay boards)"
            else:
                self._selected_option_text = "Actuator - 16 Switch + 16 Actuator (1×32CH relay board)"
        self.accept()

    @property
    def selected_option_text(self) -> str:
        """Human-readable selection for status bar (e.g. 'Sensor - Indoor/Horticulture')."""
        return self._selected_option_text
