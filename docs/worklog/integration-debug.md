# Worklog — Integration Debug (파트 5)

형식: 날짜별로 아래 블록을 **위에 추가** (최신이 위).

---

## 2026-04-07 — 테스트 중지 (오류 없음)

- **2026-04-07 16:54** (현지 시각): **Windows 재부팅**으로 통합/장시간 테스트 **중지**.
- **장애·오류 없음**으로 기록 (재부팅에 의한 의도적 중단).
- **이후 테스트 실행 방법**: Cursor 터미널이 아니라 **Windows PowerShell**에서 프로젝트 루트로 이동 후 `python main.py` 실행으로 진행.

---

## 2026-03-28 — 디버깅·자동 복구 패치

### 변경 요약
- **펌웨어 (구동기 `main.c` / `Actuator_Process.c`)**
  - 릴레이 버스로 **UART3 TX 직후** `g_uart3_last_tx_tick` 갱신 → “버스 활성” 구간에서만 스톨 감시.
  - **10초마다** `[FWDIAG][UART3][STAT]` 확장: `rx_delta`, `tx_age_ms`, `uart3_rx_idle_ms`, `mst_rx_age_ms`(UART1=Canon→노드 수신), `hal_st`, 기존 ring/err/DE, `rearm_f` / `s_rec` / `h_rec`.
  - 조건 충족 시 `[FWDIAG][UART3][STALL]` 후 **소프트 복구** → 짧은 간격 반복 시 **하드 복구** (`HAL_UART_DeInit` + `HAL_UART_Init(&huart3)` — **현재 보드레이트 유지**).
  - `[FWDIAG][UART3][RECOVER_HARD]` 카운트.
- `main.c` 시작 로그에 `[FWDIAG][BUILD] Actuator diag rev=...` 출력 추가(펌웨어 패치 적용 여부 확인용).
- **Canon**
  - `debug_listener`: **15s 이상** FW 무수신 시 `[CANON] FW debug RX idle …` 경고.
  - `actuator_test`: **`[CANON] Modbus fail streak=`** 및 누적 안내.
  - 상태바: **FW DBG 경과 / MB fail streak** (`comm_health`).

### 타이머/USART 관점 추가 조치 (2026-03-28)
- **IRQ 우선순위 분리 적용**
  - UART IRQ: priority `1` (`USART1`, `USART2`, `USART3_4`)
  - TIM IRQ: priority `2` (`TIM3` 1ms, `TIM6` 1s)
  - 반영 파일: `Core/Src/usart.c`, `Core/Src/tim.c`, `2025_ICT_LSGFARM_ACTUATORv00.ioc`
- **TIM ISR 경량화**
  - 10초 주기 진단/스톨 감시 본문을 TIM6 ISR 내부 직접 처리에서 메인 루프로 이관(`g_uart3_diag_10s_req` 플래그 방식).
  - 목적: 타이머 ISR 체류시간 감소로 USART IRQ 지연 가능성 축소.

### 다른 타이머 사용 가능성 검토
- 현재 `.ioc`에서 활성 타이머는 `TIM3`/`TIM6`만 사용.
- MCU(`STM32F070CBT6`) 특성상 **TIM14/15/16/17 사용 자체는 가능**하나, 본 이슈(USART RX 정지 의심)는
  “어느 TIM을 쓰느냐”보다 **IRQ 우선순위/ISR 체류시간** 영향이 훨씬 큼.
- 따라서 1차 대응은 이미 적용한 **USART 우선 + ISR 경량화**가 맞고,
  필요 시 2차로 `TIM6`(1s house-keeping)를 `TIM14` 등으로 분리해 실험 가능.
- 단, 타이머 교체는 CubeMX 재생성/테스트 비용이 있어 **현 단계는 보류**, 재현 결과 보고 결정.

### 로그로 판별
- `rx_delta=0` 지속 + `uart3_rx_idle_ms` 증가 + `tx_age_ms` 작음 → **UART3 수신 경로** 의심. `STALL` / `RECOVER_HARD` 확인.
- FW 로그만 멈춤 → MCU hang 또는 디버그 UART 단절; Canon idle 경고로 힌트.

### 관련 파일
- 펌웨어: `Core/Src/main.c`, `Device/Actuator/Actuator_Process.c`
- Canon: `src/services/debug_listener.py`, `actuator_test_sequence.py`, `node_manager.py`, `ui/main_window.py`

### 실험 절차 (변경 안내 — **오류 아님**)
- **구동기 펌웨어**를 현재 수정 버전으로 **다시 플래시(다운로드)** 한 뒤, **Canon을 종료했다가 최신 빌드로 재실행**하면 이전과 동일하게 장시간/타이머 시나리오 실험을 진행할 수 있음.
- 앱·펌웨어 양쪽에서 **로그에 남기는 내용과 타이밍이 늘어났거나 달라질 수 있음** (`[CANON] FW debug RX idle …`, `[CANON] Modbus fail streak=…`, `[FWDIAG][UART3][STAT]`/`[STALL]`/`[RECOVER_HARD]` 등). 이는 **통신 이상을 뜻하는 것이 아니라, 실험·진단 방법이 바뀐 결과로 기대되는 기록**임.
- 비교 시에는 같은 날짜의 로그에서 **`rx_delta`**, **`uart3_rx_idle_ms`**, **`h_rec`(하드 복구 횟수)**, Canon 상태바 헬스 문구를 함께 보면 됨.

---

## 2026-03-27

### 통합 이슈
- 전 채널 동작(Open) 후 **타이머 만료 → 릴레이 전부 OFF** 구간 이후부터 증상 발생(현장 관측).
- **디버그 시리얼 로그(`debug_listener`, `[FW]` / `FWDIAG`)가 `08:41:33` 전후부터 끊김.**
- 그 이후 Canon 측 `actuator_test`는 계속 **OPID-READ / 0x10 Write TX**를 보내지만, **RX는 `len=0` 또는 timeout** (`OPID-READ RX error len=0`, `RX timeout/short got=0`).
- 같은 로그 파일 후반(`12:27~12:30` 등)에도 동일 패턴이 반복됨 → **후속 구간은 “최초 단절”이 아니라 이미 무응답 상태가 이어진 것**으로 보면 됨.

### 근거 (로그)
- 파일: `%AppData%/LSG-AX-CTL-26/Axiom Canon/.../logs/2026/03/2026-03-27.log`
- `08:41:30`대까지: `Open nRemainingTime`, `[RB->]`, `OCS TX ack` 등 펌웨어 디버그 정상.
- `08:42:16`부터: `CH01 OPID-READ RX error len=0` → 이후 CH 단위로 연속 실패.
- 이전 장애(2026-03-25)와 같이 **FWDIAG `rx_irq` 정지·`err_total` 정지** 패턴이 유력하나, 본 구간에서는 `debug_listener` 단절로 UART3 STAT 확정은 로그 단독으로는 어렵다.

### 발생 원인 (가능성 정리)
| 구분 | 설명 |
|------|------|
| **유력** | 노드 **UART3(RS485 ↔ 릴레이) 수신 경로 hang** 또는 **HAL RX 재무장 실패** → Modbus 응답이 나오지 않음. 과거 `REARM_FAIL` 이후 `rx_irq` 고정 사례와 동일 계열. |
| **동시 가능** | 디버그 UART(펌웨어 `Debug_printf` 경로)도 멈춰 **`[FW]` 로그만 먼저 사라지는** 경우 — Canon은 Modbus 포트로만 계속 시도하므로 “명령만 있고 응답 없음”이 됨. |
| **Canon 단독 원인** 가능성 | 낮음: TX·타임아웃·재시도 로그가 계속이므로 PC 시리얼 송신 자체는 동작하는 것으로 보임. |

※ **“타이머 종료로 인한 논리 버그”**만으로는 설명하기 어렵고, **부하·버스·UART 상태가 특정 시점에 무너지는 점**이 반복 재현과 맞음.

### 해결 방안 (우선순위)
1. **펌웨어 (근본)**
   - `rx_irq` 또는 “마지막 수신 tick” 기준 **스톨 감지** → `HAL_UART_AbortReceive` → `Receive_IT` 재시도 → 실패 시 **UART DeInit/Init** → 최후 **소프트 리셋** 또는 WDG.
   - 타이머 만료·OCS STOP 직후 **짧은 윈도우**에만 추가 `FWDIAG`(DE 핀, `g_uart3_*`) 를 넣어 원인 샘플링(로그 볼륨은 최소화).
2. **Canon (운용·조기 알림)**
   - `debug_listener` **마지막 수신 시각**과 **연속 Modbus 무응답 횟수**로 다이얼로그/상태바 경고.
   - (선택) 일정 실패 후 시리얼 재오픈 시도 — **MCU hang이면 근본 해결은 아님**.
3. **하드웨어·현장**
   - 전 채널 OFF 직후 버스 전환·노이즈·DE/RE 타이밍·터미네이션 점검; 릴레이 보드가 긴 응답/버스트를 내는지 스코프 확인.

### 다음 채팅에서 이어갈 때
- 이 파일 + 로그 경로 + **최초 단절 시각 `08:41:33` 전후** 구간 지정.
- 목표: 펌웨어 복구 패치 적용 여부 / Canon 헬스 UI 적용 여부 합의.

---

## 2026-03-24

### 통합 이슈
- 장시간 구동 후 통신 무응답:
  - Canon TX는 반복 발생
  - RX는 OPID-READ/WRITE 모두 0
  - 전 채널 공통 발생

### 근거
- `2026-03-24.log` 장애 구간에서 `OPID-READ RX error len=0` 다수 반복.
- `give up after 3 attempts`가 CH1~CH16 반복.
- `debug_listener` 수신이 오전 이후 끊긴 패턴.

### 분리 판단 (가설)
- Canon 앱 로직보다는 노드 UART/RS485 수신 경로 정지 가능성이 높음.

### 다음 액션
- 노드 리셋 전 진단 로그 강화:
  - UART error 카운터
  - RX ring buffer 상태
  - DE/RE 핀 상태
- Canon에 링크 헬스체크(최근 RX 시각) 표시 검토.

---

## YYYY-MM-DD

### 통합 이슈
- 

### 근거
- 

### 분리 판단 (가설)
- 

### 다음 액션
- 

