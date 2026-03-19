"""개폐기(OCS) 상태 램프 패널: 열림/정지/닫힘."""
import re

from PyQt6.QtCore import Qt
from PyQt6.QtWidgets import (
    QFrame,
    QGridLayout,
    QLabel,
    QWidget,
)

# 펌웨어 디버그 메시지 형식: OCS_STATUS <ch> OPEN|STOP|CLOSE
OCS_STATUS_PATTERN = re.compile(r"OCS_STATUS\s+(\d+)\s+(OPEN|STOP|CLOSE)", re.IGNORECASE)


def ocs_channel_count_from_actuator_type(actuator_type: str) -> int:
    """연결 시 선택한 구동기 타입에서 개폐기(OCS) 채널 수 반환. 8 또는 16."""
    if not actuator_type:
        return 8
    t = actuator_type.strip()
    if "16 Actuator" in t or "1×32CH" in t or "1x32CH" in t or "2×16CH" in t or "2x16CH" in t:
        return 16
    return 8


class OCSLampPanel(QWidget):
    """개폐기별 열림(녹색)/정지(노랑)/닫힘(빨강) 램프. 한 채널당 하나만 켜짐."""

    STYLE_OFF = "background-color: #444; border-radius: 8px; min-width: 10px; min-height: 10px;"
    STYLE_OPEN = "background-color: #2ecc71; border-radius: 8px; min-width: 10px; min-height: 10px;"   # 녹색
    STYLE_STOP = "background-color: #f1c40f; border-radius: 8px; min-width: 10px; min-height: 10px;"  # 노랑
    STYLE_CLOSE = "background-color: #e74c3c; border-radius: 8px; min-width: 10px; min-height: 10px;" # 빨강

    def __init__(self, channel_count: int, parent: QWidget | None = None):
        super().__init__(parent)
        self._channel_count = max(1, min(16, int(channel_count)))
        self._states: list[str] = ["stop"] * self._channel_count  # open | stop | close
        self._lamps: list[tuple[QLabel, QLabel, QLabel]] = []  # (open, stop, close) per channel
        self._build_ui()

    def _build_ui(self) -> None:
        layout = QGridLayout(self)
        layout.setSpacing(6)
        # 헤더
        layout.addWidget(QLabel("채널"), 0, 0, Qt.AlignmentFlag.AlignCenter)
        layout.addWidget(QLabel("열림"), 0, 1, Qt.AlignmentFlag.AlignCenter)
        layout.addWidget(QLabel("정지"), 0, 2, Qt.AlignmentFlag.AlignCenter)
        layout.addWidget(QLabel("닫힘"), 0, 3, Qt.AlignmentFlag.AlignCenter)

        for ch in range(1, self._channel_count + 1):
            row = ch
            layout.addWidget(QLabel(f"OCS{ch}"), row, 0, Qt.AlignmentFlag.AlignCenter)
            open_lamp = QLabel()
            open_lamp.setFixedSize(14, 14)
            open_lamp.setStyleSheet(self.STYLE_OFF)
            open_lamp.setToolTip("열림")
            stop_lamp = QLabel()
            stop_lamp.setFixedSize(14, 14)
            stop_lamp.setStyleSheet(self.STYLE_OFF)
            stop_lamp.setToolTip("정지")
            close_lamp = QLabel()
            close_lamp.setFixedSize(14, 14)
            close_lamp.setStyleSheet(self.STYLE_OFF)
            close_lamp.setToolTip("닫힘")
            layout.addWidget(open_lamp, row, 1, Qt.AlignmentFlag.AlignCenter)
            layout.addWidget(stop_lamp, row, 2, Qt.AlignmentFlag.AlignCenter)
            layout.addWidget(close_lamp, row, 3, Qt.AlignmentFlag.AlignCenter)
            self._lamps.append((open_lamp, stop_lamp, close_lamp))

        # 초기 상태: 정지
        for i in range(self._channel_count):
            self._update_lamp(i, "stop")

    def _update_lamp(self, index: int, state: str) -> None:
        if index < 0 or index >= len(self._lamps):
            return
        state = (state or "stop").lower()
        if state not in ("open", "stop", "close"):
            state = "stop"
        self._states[index] = state
        open_l, stop_l, close_l = self._lamps[index]
        open_l.setStyleSheet(self.STYLE_OPEN if state == "open" else self.STYLE_OFF)
        stop_l.setStyleSheet(self.STYLE_STOP if state == "stop" else self.STYLE_OFF)
        close_l.setStyleSheet(self.STYLE_CLOSE if state == "close" else self.STYLE_OFF)

    def set_channel_status(self, channel: int, status: str) -> None:
        """채널 번호 1~N, status: OPEN|STOP|CLOSE."""
        if 1 <= channel <= len(self._lamps):
            self._update_lamp(channel - 1, status)

    def set_channel_count(self, count: int) -> None:
        """채널 수 변경 시 패널 다시 생성."""
        count = max(1, min(16, int(count)))
        if count == self._channel_count:
            return
        self._channel_count = count
        self._states = ["stop"] * count
        self._lamps.clear()
        # 자식 위젯 제거 후 재구성
        old = self.layout()
        if old:
            while old.count():
                item = old.takeAt(0)
                if item.widget():
                    item.widget().deleteLater()
        self._build_ui()

    def clear_states(self) -> None:
        """모두 정지로."""
        for i in range(len(self._lamps)):
            self._update_lamp(i, "stop")


def parse_ocs_status_line(line: str) -> tuple[int, str] | None:
    """로그 라인에서 OCS_STATUS n OPEN|STOP|CLOSE 추출. (channel, status) 또는 None."""
    m = OCS_STATUS_PATTERN.search(line)
    if not m:
        return None
    ch = int(m.group(1))
    status = m.group(2).upper()
    if status not in ("OPEN", "STOP", "CLOSE"):
        return None
    return (ch, status)
