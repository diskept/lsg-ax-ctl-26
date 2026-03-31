"""Main window for Axiom Canon (LSG-AX-CTL-26)."""
import logging

from PyQt6.QtCore import QUrl, pyqtSlot, QObject, pyqtSignal, Qt, QTimer
from PyQt6.QtGui import QAction, QDesktopServices
from PyQt6.QtWidgets import (
    QDockWidget,
    QDialog,
    QFrame,
    QLabel,
    QMainWindow,
    QMenu,
    QMenuBar,
    QMessageBox,
    QStatusBar,
    QStyle,
    QTextEdit,
    QToolBar,
    QWidget,
)
from src.core.app_context import AppContext
from src.core.logging_setup import get_log_dir
from src.core.settings import AppSettings
from src.ui.connect_select_step1 import ConnectSelectStep1
from src.ui.connect_select_step2 import ConnectSelectStep2
from src.ui.ocs_lamp_panel import OCSLampPanel, parse_ocs_status_line
from src.ui.preferences_dialog import PreferencesDialog

logger = logging.getLogger("ui.main_window")


class _LogEmitter(QObject):
    message = pyqtSignal(str)


class _QtLogHandler(logging.Handler):
    """Logging.Handler that forwards formatted records to a Qt signal."""

    def __init__(self) -> None:
        super().__init__()
        self.emitter = _LogEmitter()

    def emit(self, record: logging.LogRecord) -> None:
        try:
            msg = self.format(record)
        except Exception:
            return
        self.emitter.message.emit(msg)


class MainWindow(QMainWindow):
    """Main application window with MenuBar, StatusBar, and central placeholder."""

    def __init__(self, parent: QWidget | None = None):
        super().__init__(parent)
        self._context = AppContext()
        self._settings = AppSettings()
        self._ever_connected = False
        self._log_view: QTextEdit | None = None
        self._log_handler: _QtLogHandler | None = None
        self._log_queue: list[str] = []
        self._log_flush_timer: QTimer | None = None
        self._ocs_lamp_dock: QDockWidget | None = None
        self._ocs_lamp_panel: OCSLampPanel | None = None
        self._log_dock: QDockWidget | None = None
        self._build_ui()
        self._init_ocs_lamp_dock()
        self._init_log_dock()
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
        self._health_label = QLabel()
        self._health_label.setStyleSheet("color: #c90;")
        self._health_label.hide()
        self._status_bar.addPermanentWidget(self._health_label)
        self.setStatusBar(self._status_bar)

    def _init_ocs_lamp_dock(self) -> None:
        """개폐기 상태 램프 도크 (연결 시 채널 수에 맞춰 재구성)."""
        dock = QDockWidget("개폐기 상태", self)
        dock.setObjectName("ocsLampDock")
        dock.setAllowedAreas(
            Qt.DockWidgetArea.BottomDockWidgetArea
            | Qt.DockWidgetArea.TopDockWidgetArea
        )
        # 초기에는 8채널 빈 패널 (연결 시 재구성)
        panel = OCSLampPanel(8)
        dock.setWidget(panel)
        self.addDockWidget(Qt.DockWidgetArea.BottomDockWidgetArea, dock)
        self._ocs_lamp_dock = dock
        self._ocs_lamp_panel = panel

    def _rebuild_ocs_lamp_panel(self, channel_count: int) -> None:
        """연결 시 선택된 채널 수로 램프 패널 삭제 후 새로 생성."""
        if not self._ocs_lamp_dock:
            return
        if self._ocs_lamp_panel:
            self._ocs_lamp_panel.deleteLater()
            self._ocs_lamp_panel = None
        panel = OCSLampPanel(channel_count)
        self._ocs_lamp_dock.setWidget(panel)
        self._ocs_lamp_panel = panel

    def _init_log_dock(self) -> None:
        """Create a dockable log viewer and connect it to Python logging."""
        dock = QDockWidget("Log", self)
        dock.setObjectName("logDock")
        dock.setAllowedAreas(
            Qt.DockWidgetArea.BottomDockWidgetArea
            | Qt.DockWidgetArea.TopDockWidgetArea
        )
        view = QTextEdit()
        view.setReadOnly(True)
        view.setLineWrapMode(QTextEdit.LineWrapMode.NoWrap)
        # Long-running test 중 로그가 매우 많아지면 append가 느려져 UI가 갱신이 지연될 수 있음.
        # QTextEdit는 QTextDocument를 통해 block count를 제한한다.
        view.document().setMaximumBlockCount(4000)
        dock.setWidget(view)
        self.addDockWidget(Qt.DockWidgetArea.BottomDockWidgetArea, dock)
        self._log_dock = dock
        self._log_view = view
        # 램프 도크를 로그 도크 위에 세로 분할
        if self._ocs_lamp_dock:
            self.splitDockWidget(self._ocs_lamp_dock, dock, Qt.Orientation.Vertical)

        handler = _QtLogHandler()
        handler.setLevel(logging.INFO)
        formatter = logging.Formatter(
            "%(asctime)s [%(name)s] %(levelname)s: %(message)s",
            datefmt="%H:%M:%S",
        )
        handler.setFormatter(formatter)
        handler.emitter.message.connect(self._append_log_line)
        logging.getLogger().addHandler(handler)
        self._log_handler = handler

        # 로그가 매우 많을 때 QTextEdit 업데이트가 과도해져 UI가 멈춘 것처럼 보일 수 있음.
        # 그래서 append를 즉시 하지 않고 일정 주기로 큐를 배치로 flush한다.
        self._log_flush_timer = QTimer(self)
        self._log_flush_timer.setInterval(150)  # ms
        self._log_flush_timer.timeout.connect(self._flush_log_queue)
        self._log_flush_timer.start()

    def _flush_log_queue(self) -> None:
        if not self._log_view or not self._log_queue:
            return

        bar = self._log_view.verticalScrollBar()
        was_near_bottom = bar.value() >= bar.maximum() - 3

        max_lines = 300  # 한 번 flush 시 최대 표시 라인
        batch = self._log_queue[:max_lines]
        self._log_queue = self._log_queue[max_lines:]

        for line in batch:
            self._log_view.append(line)

        # 사용자가 아래를 보고 있을 때만 따라가게 함
        if was_near_bottom:
            bar.setValue(bar.maximum())

    @pyqtSlot(str)
    def _append_log_line(self, line: str) -> None:
        # 1) 램프 업데이트는 즉시 처리 (가벼움)
        # 펌웨어 OCS 상태 메시지면 램프 갱신
        parsed = parse_ocs_status_line(line)
        if parsed and self._ocs_lamp_panel:
            ch, status = parsed
            self._ocs_lamp_panel.set_channel_status(ch, status)

        # 2) 텍스트 도크는 큐로 모아서 일정 주기로 배치 flush
        self._log_queue.append(line)

    def _connect_signals(self) -> None:
        nm = self._context.node_manager
        nm.serial_params_changed.connect(self._on_serial_params_changed)
        nm.comm_health.connect(self._on_comm_health)
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
            self._health_label.show()
            self.statusBar().showMessage("Connected", 1500)
            # 연결 시 선택된 구동기 타입에 따라 개폐기 램프 개수 재구성
            n = self._settings.get_ocs_channel_count()
            self._rebuild_ocs_lamp_panel(n)
        else:
            if self._ever_connected:
                self.statusBar().showMessage("Disconnected", 1500)
            self._health_label.hide()
            self._health_label.clear()
            self.refresh_serial_summary(False, params)

    @pyqtSlot(str)
    def _on_comm_health(self, text: str) -> None:
        if not text:
            self._health_label.clear()
            return
        self._health_label.setText(text)

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
        self.statusBar().showMessage("타임 동작 테스트 시작 (로그·램프 확인)", 4000)

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
