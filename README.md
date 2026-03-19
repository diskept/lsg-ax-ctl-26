# Axiom Canon — LSG-AX-CTL-26

Axiom Standard Node **Control Tool** (Configuration / Test / Calibration / Diagnostics).

- **LSG-AX-CTL-26**: PC 설정·로깅·테스트 프로그램 (PyQt6)
- **LSG-AX-ADG-26**: 신규 구동기 노드 프로젝트 예정 (현 펌웨어 분석·개선 후 진행)
- **참조 펌웨어**: `2025_ICT_LSGFARM_ACTUATORv00` (외주 개발, STM32F070CBT6, Modbus RS485, KS X 3267 기반)

## Quick start

```powershell
python -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -r requirements.txt
python main.py
# 또는
.\main.py
```

## 구동기 정지/오버런 원인 분석 (Canon + Firmware 로그)

### 통신 구성

- **Canon ↔ ACTUATOR**: RS485 (CON6), **9600 bps**, KS X 3267 (Modbus RTU 기반)
- **ACTUATOR ↔ Relay board**: RS485, **38400 bps** (`huart3`)
- **Firmware Debug → PC**: UART, **38400 bps** (PC의 COM 포트, 기본 `COM3`)

### Canon 사용 방법

1. **Settings → Serial**에서 아래를 설정
   - **Port / Baudrate**: ACTUATOR에 연결된 RS485 어댑터 포트, **9600**
   - **FW Debug Port**: 디버그 UART가 연결된 포트 (기본 `COM3`)
   - **FW Debug Baud**: **38400**
2. **Connect** 클릭
   - 제어 포트 연결 후, FW Debug 포트도 자동으로 열어 로그 캡처를 시도합니다.
3. **Node → Test...** 실행
   - OCS(개폐기) **1~16번**에 대해 **303(타임드 제어)** 를 **30초**로 설정하여 순차 전송
   - 채널 간 전송 간격은 **1분**
4. **Help → Open Logs Folder**에서 로그 파일 확인
   - **`[FW]`**: 펌웨어 디버그 UART 로그
   - **`TX ...` / `RX ...`**: Canon이 송수신한 Modbus RTU 프레임(hex)

### Firmware 쪽 추가된 디버그 포인트

- 릴레이보드 송신 직전에 아래 로그가 출력됩니다.
  - `[RB->] GET ...` (상태 읽기)
  - `[RB->] SET ...` (출력 설정/마스크 적용)

이렇게 하면 “Canon 명령 전송 → 펌웨어 수신/처리 → 릴레이보드 송신”까지를 한 타임라인으로 맞춰 원인을 추적할 수 있습니다.

## 펌웨어 (참조용)

| 항목 | 내용 |
|------|------|
| 경로 | `2025_ICT_LSGFARM_ACTUATORv00/` |
| MCU | STM32F070CBT6 |
| 도구 | STM32CubeIDE 2.0.0 (Build 26820_20251114_1348) |
| 통신 | Modbus RTU over RS485 (USART1: 노드↔PC, USART3: 노드↔릴레이) |
| 기준 | KS X 3267 (스위치 16CH, 개폐기 8CH/16CH·32CH) |

상세 프로토콜 및 검토 결과는 `docs/FIRMWARE_REVIEW_ACTUATORv00.md` 참고.
