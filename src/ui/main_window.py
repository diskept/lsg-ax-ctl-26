"""Main window for Axiom Canon (LSG-AX-CTL-26)."""
import logging

from PyQt6.QtCore import QUrl, pyqtSlot
from PyQt6.QtGui import QAction, QDesktopServices
from PyQt6.QtWidgets import (
    QDialog,
    QFrame,
    QLabel,
    QMainWindow,
    QMenu,
    QMenuBar,
    QMessageBox,
    QStatusBar,
    QStyle,
    QToolBar,
    QWidget,
)
from src.core.app_context import AppContext
from src.core.logging_setup import get_log_dir
from src.core.settings import AppSettings
from src.ui.preferences_dialog import PreferencesDialog
from src.ui.connect_select_step1 import ConnectSelectStep1
from src.ui.connect_select_step2 import ConnectSelectStep2

logger = logging.getLogger("ui.main_window")


class MainWindow(QMainWindow):
    """Main application window with MenuBar, StatusBar, and central placeholder."""

    def __init__(self, parent: QWidget | None = None):
        super().__init__(parent)
        self._context = AppContext()
        self._settings = AppSettings()
        self._ever_connected = False
        self._build_ui()
        self._connect_signals()

    def _build_ui(self) -> None:
        self.setWindowTitle("Axiom Canon — LSG-AX-CTL-26")
        self.setMinimumSize(800, 600)
        self.resize(1024, 768)

        # Central placeholder
        central_placeholder = QFrame()
        central_placeholder.setFrameStyle(QFrame.Shape.StyledPanel | QFrame.Shadow.Sunken)
        central_placeholder.setObjectName("centralPlaceholder")
        self.setCentralWidget(central_placeholder)

        # Toolbar (icon-only)
        toolbar = QToolBar()
        toolbar.setWindowTitle("Main")
        style = self.style()
        self._connect_act = QAction(style.standardIcon(QStyle.StandardPixmap.SP_ArrowForward), "", self)
        self._connect_act.setToolTip("Connect")
        self._connect_act.setStatusTip("Connect to the configured serial port")
        self._connect_act.triggered.connect(self._on_connect)
        toolbar.addAction(self._connect_act)
        disconnect_act = QAction(style.standardIcon(QStyle.StandardPixmap.SP_DialogCloseButton), "", self)
        disconnect_act.setToolTip("Disconnect")
        disconnect_act.setStatusTip("Disconnect current connection")
        disconnect_act.triggered.connect(self._on_disconnect)
        toolbar.addAction(disconnect_act)
        self.addToolBar(toolbar)

        # MenuBar
        menubar = self.menuBar()
        self._build_file_menu(menubar)
        self._build_tools_menu(menubar)
        self._build_node_menu(menubar)
        self._build_help_menu(menubar)

        # StatusBar (summary label on left, hidden until first connect)
        self._status_bar = QStatusBar()
        self.summary_label = QLabel()
        self._status_bar.addWidget(self.summary_label, 1)
        self.summary_label.hide()
        self.setStatusBar(self._status_bar)

    def _connect_signals(self) -> None:
        nm = self._context.node_manager
        nm.serial_params_changed.connect(self._on_serial_params_changed)
        nm.info.connect(self._on_info)
        nm.error.connect(self._on_error)
        self._update_connect_actions_enabled(not nm.is_connected())

    def _update_connect_actions_enabled(self, enabled: bool) -> None:
        """Enable Connect actions when disconnected, disable when connected."""
        self._connect_act.setEnabled(enabled)
        self._connect_menu_act.setEnabled(enabled)

    @pyqtSlot(bool, dict)
    def _on_serial_params_changed(self, connected: bool, params: dict) -> None:
        self._update_connect_actions_enabled(not connected)
        if connected:
            self._ever_connected = True
            self.refresh_serial_summary(True, params)
            self.statusBar().showMessage("Connected", 1500)
        else:
            if self._ever_connected:
                self.statusBar().showMessage("Disconnected", 1500)
            self.refresh_serial_summary(False, params)

    @pyqtSlot(str)
    def _on_info(self, msg: str) -> None:
        self.statusBar().showMessage(msg, 2000)

    @pyqtSlot(str)
    def _on_error(self, msg: str) -> None:
        self.statusBar().showMessage(f"ERROR: {msg}", 3000)

    def refresh_serial_summary(
        self, connected: bool | None = None, params: dict | None = None
    ) -> None:
        """Update or hide the one-line serial summary. If connected is None, keep hidden."""
        if connected is None:
            return
        if connected:
            if params is None:
                s = self._settings
                params = {
                    "port": s.get_last_port() or "—",
                    "baud": s.get_baud(),
                    "parity": s.get_parity(),
                    "data_bits": s.get_data_bits(),
                    "stop_bits": s.get_stop_bits(),
                    "timeout_ms": s.get_timeout_ms(),
                }
            p = params
            text = f"Port: {p['port']} | Baud: {p['baud']} | Parity: {p['parity']} | Data: {p['data_bits']} | Stop: {p['stop_bits']} | Timeout: {p['timeout_ms']}ms"
            self.summary_label.setText(text)
            self.summary_label.setStyleSheet("color: #69d;")
            self.summary_label.show()
        else:
            self.summary_label.hide()

    def _build_file_menu(self, menubar: QMenuBar) -> None:
        file_menu = menubar.addMenu("&File")
        exit_action = QAction("E&xit", self)
        exit_action.setShortcut("Ctrl+Q")
        exit_action.triggered.connect(self.close)
        file_menu.addAction(exit_action)

    def _build_tools_menu(self, menubar: QMenuBar) -> None:
        tools_menu = menubar.addMenu("&Tools")
        settings_action = QAction("Settings...", self)
        settings_action.triggered.connect(self._on_settings)
        tools_menu.addAction(settings_action)
        tools_menu.addAction("Placeholder...").setEnabled(False)

    def _build_node_menu(self, menubar: QMenuBar) -> None:
        node_menu = menubar.addMenu("&Node")
        scan_action = QAction("&Scan...", self)
        scan_action.triggered.connect(self._on_scan)
        node_menu.addAction(scan_action)
        self._connect_menu_act = QAction("&Connect...", self)
        self._connect_menu_act.triggered.connect(self._on_connect)
        node_menu.addAction(self._connect_menu_act)
        test_action = QAction("&Test...", self)
        test_action.triggered.connect(self._on_test)
        node_menu.addAction(test_action)

    def _build_help_menu(self, menubar: QMenuBar) -> None:
        help_menu = menubar.addMenu("&Help")
        open_logs_action = QAction("Open Logs Folder", self)
        open_logs_action.triggered.connect(self._on_open_logs_folder)
        help_menu.addAction(open_logs_action)
        about_action = QAction("&About", self)
        about_action.triggered.connect(self._on_about)
        help_menu.addAction(about_action)


    @pyqtSlot()
    def _on_settings(self) -> None:
        dlg = PreferencesDialog(self._settings, self)
        dlg.exec()

    @pyqtSlot()
    def _on_connect(self) -> None:
        step1 = ConnectSelectStep1(self._settings, self)
        if not step1.exec():
            return
        group = step1.selected_group
        while True:
            step2 = ConnectSelectStep2(group, self._settings, self)
            result = step2.exec()
            if result == QDialog.DialogCode.Accepted:
                self._settings.set_last_node_group(group)
                if group == "sensor":
                    self._settings.set_sensor_type(step2.selected_sensor_type)
                else:
                    self._settings.set_actuator_type(step2.selected_actuator_type)
                self.statusBar().showMessage(
                    f"Node selected: {step2.selected_option_text}", 5000
                )
                nm = self._context.node_manager
                ok = nm.connect()
                if not ok:
                    self.statusBar().showMessage("Connection failed", 3000)
                return
            if result == ConnectSelectStep2.RESULT_BACK:
                step1 = ConnectSelectStep1(self._settings, self)
                if not step1.exec():
                    return
                group = step1.selected_group
                continue
            return

    @pyqtSlot()
    def _on_disconnect(self) -> None:
        self._context.node_manager.disconnect()  # emits serial_params_changed

    @pyqtSlot()
    def _on_scan(self) -> None:
        self._context.node_manager.scan()
        self.statusBar().showMessage("Scan complete", 2000)

    @pyqtSlot()
    def _on_test(self) -> None:
        self._context.node_manager.test()
        self.statusBar().showMessage("Test complete", 2000)

    @pyqtSlot()
    def _on_open_logs_folder(self) -> None:
        log_dir = get_log_dir()
        if log_dir:
            QDesktopServices.openUrl(QUrl.fromLocalFile(log_dir))
        else:
            self.statusBar().showMessage("Logs folder not available", 2000)

    @pyqtSlot()
    def _on_about(self) -> None:
        QMessageBox.about(
            self,
            "About Axiom Canon",
            "Axiom Canon — LSG-AX-CTL-26\n"
            "Axiom Standard Node Control Tool\n"
            "(Configuration / Test / Calibration / Diagnostics)\n\n"
            "TODO: logging — add version and build info",
        )
