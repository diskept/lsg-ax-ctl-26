# 구동기 펌웨어 전체 검토 — 2025_ICT_LSGFARM_ACTUATORv00

검토일: 펌웨어 경로 `2025_ICT_LSGFARM_ACTUATORv00` 내 애플리케이션/디바이스 소스 기준.

---

## 1. 수정 완료·정상으로 판단된 항목

| 항목 | 파일 | 상태 |
|------|------|------|
| 마스터 수신 버퍼 오버플로우 | `Device/Master/Master_Process.c` | ✅ 수정 완료. 버퍼 풀 시 최신 1바이트 유지 |
| 레지스터 Read 수량 135 예외 | `Device/Master/Master_Process.c` | ✅ 예외 허용 유지, 주석으로 명시 |
| 구동기 수신 링버퍼 | `Device/Actuator/Actuator_Process.c` | ✅ 25.12.31 수정 반영(오버플로우·인덱스 처리) |
| 타이머 ID 무효값 처리 | `Core/Src/tick_timer.c` | ✅ `TIMER_INVALID_ID` 반환 시 사용 안 함(create 32회만 호출) |
| msec_timer 초기화 플래그 | `Core/Src/tick_timer.c` | ✅ `msec_inited` 사용으로 수정됨(diskept) |
| EEPROM 저장 단위 | `Device/EEProm/eeprom.c` | ✅ 주소당 2바이트(반워드) 저장, 보드레이트 16비트 정상 저장 |

---

## 2. 이상·위험 가능성 있는 항목

### 2.1 Debug_printf 버퍼 오버런 (중요도: 중) — ✅ 수정 완료

**파일**: `Device/Actuator/Actuator_Process.c`

- **수정**: `vsprintf` → `vsnprintf(buffer, sizeof(buffer), fmt, argptr)` 로 변경하여 버퍼 오버런 방지.

---

### 2.2 sec_timer / msec_timer 인덱스 검사 없음 (중요도: 낮음) — ✅ 수정 완료

**파일**: `Core/Src/tick_timer.c`

- **수정**: `sec_timer_get_sec`, `sec_timer_reset`, `msec_timer_get_msec`, `msec_timer_reset` 에서 `tmr >= SEC_MAX_TIMER` 이면 return 0 또는 return(no-op) 하도록 범위 검사 추가.

---

### 2.3 Master_Buffer_Process 수신 패킷 상한

**파일**: `Device/Master/Master_Process.c`

- `RxBuffer[UART_RX_RING_BUFFER_SIZE]`(1024), `nIndex`는 0~1023까지 증가 후 1024에서 리셋됨. 인덱스 오버런 없음.
- Modbus RTU 최대 프레임 길이(256바이트)보다 여유 있음. **이상 없음.**

---

### 2.4 UART 초기화 순서

**파일**: `Core/Src/main.c`

- GPIO → TIM → USART1~4 → DMA → ADC 순. RS485 DE(USART1 PA12)는 `HAL_UART_MspInit`에서 설정. **이상 없음.**

---

### 2.5 인터럽트 내 콜백

**파일**: `Core/Src/main.c` — `HAL_UART_RxCpltCallback`, `HAL_TIM_PeriodElapsedCallback`

- UART 수신 콜백에서 링버퍼에 1바이트만 넣고 재등록. 처리 부하는 낮음.
- 타이머 콜백에서 `Master_1ms_Process`, `Actuator_1ms_Process` 호출. 이 함수들이 블로킹이나 긴 루프를 쓰지 않으면 문제 없음. **현재 구현상 특별한 이상 징후 없음.**

---

## 3. 기타 확인 사항

| 항목 | 결과 |
|------|------|
| define.h / struct.h 상수·타입 | DEFAULT_REGISTER_MAX(631), 레지스터 맵·버퍼 크기 일관됨 |
| NodeVersion (RELAY_16CH_VER / RELAY_32CH_VER) | define.h에서 RELAY_32CH_VER 사용, 분기 일관됨 |
| CRC 계산 | Modbus 표준 CRC 테이블 사용, Lo/Hi 바이트 순서 적용 |
| EEPROM 주소 | CONFIG_CHECK_ADDR, DEFAULT_CONFIG_ADDR, INSTALL_CONFIG_ADDR, E2P_ADDR_OFFSET(Address*2) 사용으로 2바이트 단위 유지 |

---

## 4. 요약 판정

- **수정 반영 완료**: (1) 마스터 수신 버퍼 오버플로우, (2) 레지스터 135 예외 주석, (3) `Debug_printf` → `vsnprintf`, (4) `sec_timer`/`msec_timer` 인덱스 범위 검사. 이상 **이상 없음**.
- **전체**: 권장 수정 사항 모두 반영됨. 내일 컴파일 후 다운로드·동작 시험 진행 가능.
