"""Axiom Canon (LSG-AX-CTL-26) — Application entry point."""
import logging
import sys

from PyQt6.QtWidgets import QApplication

from src.core.logging_setup import init_logging
from src.ui.main_window import MainWindow


def main() -> int:
    app = QApplication(sys.argv)
    app.setApplicationName("Axiom Canon")
    app.setOrganizationName("LSG-AX-CTL-26")

    init_logging("AxiomCanon")
    logging.getLogger(__name__).info("App started")

    window = MainWindow()
    window.show()

    return app.exec()


if __name__ == "__main__":
    sys.exit(main())
