"""Axiom Canon (LSG-AX-CTL-26) — Application entry point."""
import logging
import sys

from PyQt6.QtWidgets import QApplication

from src.core.logging_setup import init_logging
from src.ui.main_window import MainWindow

_LIGHT_STYLESHEET = """
QPushButton {
    padding: 5px 12px;
    border-radius: 4px;
}
QPushButton:hover {
    background-color: palette(highlight);
    color: palette(highlighted-text);
}
QComboBox, QSpinBox, QLineEdit {
    padding: 5px 8px;
    min-height: 1.2em;
}
QComboBox:focus, QSpinBox:focus, QLineEdit:focus {
    border: 1px solid palette(highlight);
}
"""


def main() -> int:
    app = QApplication(sys.argv)
    app.setStyleSheet(_LIGHT_STYLESHEET)
    app.setApplicationName("Axiom Canon")
    app.setOrganizationName("LSG-AX-CTL-26")

    init_logging("AxiomCanon")
    logging.getLogger(__name__).info("App started")

    window = MainWindow()
    window.show()

    return app.exec()


if __name__ == "__main__":
    sys.exit(main())
