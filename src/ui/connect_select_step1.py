"""Step 1: Select node group (Sensor or Actuator)."""
from PyQt6.QtWidgets import (
    QDialog,
    QDialogButtonBox,
    QRadioButton,
    QVBoxLayout,
    QWidget,
)


class ConnectSelectStep1(QDialog):
    """Select Node Type: Sensor Node or Actuator Node. Exposes chosen group via selected_group."""

    GROUP_SENSOR = "sensor"
    GROUP_ACTUATOR = "actuator"

    def __init__(self, parent: QWidget | None = None):
        super().__init__(parent)
        self.setWindowTitle("Select Node Type")
        self._build_ui()

    def _build_ui(self) -> None:
        layout = QVBoxLayout(self)

        self._sensor_radio = QRadioButton("Sensor Node")
        self._actuator_radio = QRadioButton("Actuator Node")
        self._sensor_radio.setChecked(True)
        layout.addWidget(self._sensor_radio)
        layout.addWidget(self._actuator_radio)

        buttons = QDialogButtonBox(
            QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel
        )
        buttons.button(QDialogButtonBox.StandardButton.Ok).setText("Next")
        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)
        layout.addWidget(buttons)

    @property
    def selected_group(self) -> str:
        """Return 'sensor' or 'actuator'."""
        return self.GROUP_SENSOR if self._sensor_radio.isChecked() else self.GROUP_ACTUATOR
