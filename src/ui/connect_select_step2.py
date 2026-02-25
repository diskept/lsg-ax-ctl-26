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

from src.core.settings import AppSettings


class ConnectSelectStep2(QDialog):
    """Select Node Option. For Sensor: Indoor/Outdoor + sub; for Actuator: 3 radio types. Exposes selection via attributes."""

    def __init__(self, group: str, settings: AppSettings, parent: QWidget | None = None):
        super().__init__(parent)
        self.setWindowTitle("Select Node Option")
        self._group = group  # "sensor" or "actuator"
        self._settings = settings
        self._selected_option_text = ""
        self._selected_sensor_type = "Indoor/Horticulture"
        self._selected_actuator_type = "16 Switch + 8 Actuator"
        self._build_ui()

    def showEvent(self, event) -> None:
        super().showEvent(event)
        if self._group == "sensor":
            st = self._settings.get_sensor_type()
            if st == "Outdoor/Common":
                self._sensor_outdoor.setChecked(True)
            else:
                self._sensor_indoor.setChecked(True)
                if "Livestock" in st:
                    self._sensor_indoor_combo.setCurrentText("Livestock")
                else:
                    self._sensor_indoor_combo.setCurrentText("Horticulture")
        else:
            at = self._settings.get_actuator_type()
            if "1×32CH" in at or "1x32CH" in at:
                self._actuator_16_1.setChecked(True)
            elif "2×16CH" in at or "2x16CH" in at:
                self._actuator_16_2.setChecked(True)
            else:
                self._actuator_8.setChecked(True)

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
                self._selected_sensor_type = f"Indoor/{sub}"
            else:
                self._selected_option_text = "Sensor - Outdoor/Common (Horticulture/Livestock)"
                self._selected_sensor_type = "Outdoor/Common"
        else:
            if self._actuator_8.isChecked():
                self._selected_option_text = "Actuator - 16 Switch + 8 Actuator"
                self._selected_actuator_type = "16 Switch + 8 Actuator"
            elif self._actuator_16_2.isChecked():
                self._selected_option_text = "Actuator - 16 Switch + 16 Actuator (2×16CH relay boards)"
                self._selected_actuator_type = "16 Switch + 16 Actuator (2×16CH relay boards)"
            else:
                self._selected_option_text = "Actuator - 16 Switch + 16 Actuator (1×32CH relay board)"
                self._selected_actuator_type = "16 Switch + 16 Actuator (1×32CH relay board)"
        self.accept()

    @property
    def selected_option_text(self) -> str:
        """Human-readable selection for status bar (e.g. 'Sensor - Indoor/Horticulture')."""
        return self._selected_option_text

    @property
    def selected_sensor_type(self) -> str:
        """For settings: Indoor/Horticulture, Indoor/Livestock, or Outdoor/Common."""
        return getattr(self, "_selected_sensor_type", "Indoor/Horticulture")

    @property
    def selected_actuator_type(self) -> str:
        """For settings: actuator type string."""
        return getattr(self, "_selected_actuator_type", "16 Switch + 8 Actuator")
