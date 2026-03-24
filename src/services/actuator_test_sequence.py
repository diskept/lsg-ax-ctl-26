import logging
import threading
import time

from src.core.settings import AppSettings
from src.services.modbus_rtu import (
    build_read_holding_registers_request,
    build_write_multiple_registers_request,
    hex_bytes,
    read_exact,
)
from src.services.serial_service import SerialService

logger = logging.getLogger("actuator_test")


# 구동기노드 타임 동작 테스트 조건 (엑셀 기준)
# - 매 분 0초: OCS1 → 1초 후 OCS2 → … (채널 간 1초), 채널 수는 설정(8/16) 따름
# - 다음 분 0초까지 대기 후 반복, 종료 시점 없음
SECONDS_PER_CHANNEL = 1
SECONDS_PER_MINUTE = 60
RETRY_COUNT = 3
RETRY_INTERVAL_S = 0.3


class ActuatorTimedSequence:
    """
    구동기노드 타임 동작 테스트 (엑셀 조건).
    KS X 3267 Modbus RTU: 매 분 0초에 OCS1, 1초 간격으로 OCS2~OCS16 전송 후
    다음 분까지 대기, 12시간 반복. OPID는 채널별 읽기 후 +1 전송.
    """

    CONTROL_TIMED_OPEN = 303
    OCS_CMD_BASE_ADDR = 567  # CON_OCSWITCH_1_CMD_ADDR (KS X 3267 개폐기 번호별 ADDR)
    OCS_ADDR_STEP = 4
    # 개폐기 번호별 ADDR: OCS1=567, OCS2=571, ..., OCS8=595, OCS9=599, ..., OCS16=627
    OCS_OPID_BASE_ADDR = 267  # OCSWITCH_1_OPID_17_ADDR
    OCS_STATUS_BASE_ADDR = 268  # OCSWITCH_1_STATUS_ADDR

    def __init__(self, serial: SerialService, settings: AppSettings):
        self._serial = serial
        self._settings = settings
        self._stop = threading.Event()
        self._thread: threading.Thread | None = None

    def start(self) -> bool:
        if self.is_running():
            return True
        if not self._serial.is_connected():
            return False
        self._stop.clear()
        self._thread = threading.Thread(target=self._run, name="ActuatorTimedSequence", daemon=True)
        self._thread.start()
        return True

    def stop(self) -> None:
        self._stop.set()
        if self._thread is not None:
            self._thread.join(timeout=2.0)
        self._thread = None

    def is_running(self) -> bool:
        return self._thread is not None and self._thread.is_alive()

    def _channel_count(self) -> int:
        return self._settings.get_ocs_channel_count()

    def _slave_id(self) -> int:
        raw = (self._settings.get_node_address() or "").strip()
        if not raw:
            return 1
        try:
            v = int(raw, 0)
            if 1 <= v <= 247:
                return v
        except Exception:
            pass
        return 1

    def _run(self) -> None:
        slave = self._slave_id()
        n_ch = self._channel_count()
        timed_sec = 30  # 30초 오픈
        wait_after_cycle = SECONDS_PER_MINUTE - n_ch * SECONDS_PER_CHANNEL

        time_low16 = timed_sec & 0xFFFF
        time_high16 = (timed_sec >> 16) & 0xFFFF

        logger.info(
            "TEST start: slave=%d, ch=%d, 30s open, no end (stop by user), %ds/ch, %ds until next minute",
            slave,
            n_ch,
            SECONDS_PER_CHANNEL,
            wait_after_cycle,
        )
        logger.info(
            "OCS ADDR map: OCS1=567 OCS2=571 OCS3=575 OCS4=579 OCS5=583 OCS6=587 OCS7=591 OCS8=595 OCS9=599 OCS10=603 OCS11=607 OCS12=611 OCS13=615 OCS14=619 OCS15=623 OCS16=627"
        )
        if n_ch == 16:
            logger.info("OCS 16ch mode (NodeVersion 32CH: single relay board OCS1~16)")

        while not self._stop.is_set():
            # 매 분: 0초 OCS1, 1초 OCS2, …
            for ch in range(1, n_ch + 1):
                if self._stop.is_set():
                    break

                # 1) 현재 개폐기 OPID 읽기 (실패 시 최대 3회 재시도, 500ms 간격)
                current_opid = 0
                opid_addr = self.OCS_OPID_BASE_ADDR + (ch - 1) * 4
                read_req = build_read_holding_registers_request(slave, opid_addr, 1)
                for attempt in range(1, RETRY_COUNT + 1):
                    # Keep request/response framing clean when prior attempt left bytes in buffer.
                    stale = self._serial.read_all()
                    if stale:
                        logger.warning(
                            "CH%02d pre-read drain %dB stale: %s",
                            ch,
                            len(stale),
                            hex_bytes(stale),
                        )
                    logger.info("CH%02d OPID-READ TX : %s", ch, hex_bytes(read_req))
                    self._serial.write(read_req)
                    resp = read_exact(self._serial.read, 7, timeout_s=1.5)
                    if len(resp) == 7 and resp[1] == 0x03 and resp[2] == 0x02:
                        current_opid = (resp[3] << 8) | resp[4]
                        logger.info("CH%02d current OPID=%d", ch, current_opid)
                        break
                    logger.error(
                        "CH%02d OPID-READ RX error len=%d (attempt %d/%d) : %s",
                        ch, len(resp), attempt, RETRY_COUNT, hex_bytes(resp),
                    )
                    if attempt < RETRY_COUNT:
                        time.sleep(RETRY_INTERVAL_S)

                opid = (current_opid + 1) & 0xFFFF
                # Firmware-side OPID gate is strict-increasing in many paths.
                # When current OPID wraps from 0xFFFF, avoid sending 0 and start from 1.
                if opid == 0:
                    logger.warning(
                        "CH%02d OPID wrap detected current=%d -> next=0; use OPID=1 to avoid wrap reject",
                        ch,
                        current_opid,
                    )
                    opid = 1
                addr = self.OCS_CMD_BASE_ADDR + (ch - 1) * self.OCS_ADDR_STEP
                logger.info("CH%02d ADDR=%d (OCS%d) OPID=%d", ch, addr, ch, opid)
                values = [
                    self.CONTROL_TIMED_OPEN,
                    opid,
                    time_low16,
                    time_high16,
                ]
                req = build_write_multiple_registers_request(slave, addr, values)

                # 2) 명령 전송 (실패 시 최대 3회 재시도, 500ms 간격)
                write_ok = False
                for attempt in range(1, RETRY_COUNT + 1):
                    stale = self._serial.read_all()
                    if stale:
                        logger.warning(
                            "CH%02d pre-write drain %dB stale: %s",
                            ch,
                            len(stale),
                            hex_bytes(stale),
                        )
                    logger.info("CH%02d OPID=%d TX : %s", ch, opid, hex_bytes(req))
                    self._serial.write(req)
                    resp = read_exact(self._serial.read, 8, timeout_s=2.0)
                    if len(resp) == 8:
                        logger.info("CH%02d OPID=%d RX : %s", ch, opid, hex_bytes(resp))
                        write_ok = True
                        break
                    logger.error(
                        "CH%02d OPID=%d RX timeout/short got=%d (attempt %d/%d) : %s",
                        ch, opid, len(resp), attempt, RETRY_COUNT, hex_bytes(resp),
                    )
                    if attempt < RETRY_COUNT:
                        time.sleep(RETRY_INTERVAL_S)
                if not write_ok:
                    logger.warning("CH%02d OPID=%d give up after %d attempts, skip to next channel", ch, opid, RETRY_COUNT)

                # 채널 간 1초 대기
                end = time.monotonic() + SECONDS_PER_CHANNEL
                while time.monotonic() < end and not self._stop.is_set():
                    time.sleep(0.05)

            # 다음 분 0초까지 대기 (44초)
            end = time.monotonic() + wait_after_cycle
            while time.monotonic() < end and not self._stop.is_set():
                time.sleep(0.1)

        logger.info("TEST stopped")

