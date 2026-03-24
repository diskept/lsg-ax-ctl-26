# Worklog — Actuator Firmware (파트 2)

형식: 날짜별로 아래 블록을 **위에 추가** (최신이 위).

---

## 2026-03-24

### 한 일
- `2026-03-24.log` 기준 장시간 장애 구간 분석.
- 오전 정상 구간에서는 FW debug 수신(`debug_listener`) 및 OCS RX/TX ack 정상 확인.
- 오후 장애 구간에서는 Canon TX만 반복, OPID-READ/WRITE RX가 전채널 0인 패턴 확인.
- `debug_listener` 마지막 수신이 오전 시점 이후 끊긴 패턴 확인.
- 저장소 분리 결정: Actuator 펌웨어는 `LSG-AX-CTL-26` 하위가 아닌 별도 repository로 전환.

### 다음 할 일
- 노드 리셋 전 진단 지표 추가:
  - UART 에러(OE/FE/NE/PE) 카운터
  - RX ring buffer head/tail 모니터
  - DE/RE 상태 핀 주기 로그
- 일정 시간 RX 0 지속 시 UART 재초기화 self-recovery 설계 검토.

### 프로젝트 경로 (CubeIDE 등)
- legacy: `2025_ICT_LSGFARM_ACTUATORv00` (이 레포에서 제거 예정)
- next (계획): `01.Axiom-A/LSG-AX-ADG-26`

### 참고 커밋
- `9f60d53` (FW debug 메시지 강화 포함)

### 이슈 / 메모
- Canon disconnect/connect로 회복되지 않음.
- TX 동작, RX 무응답(전채널) + debug_listener 무수신은 노드 UART/RS485 수신 경로 정지 가능성 높음.

---

## YYYY-MM-DD

### 한 일
- 

### 다음 할 일
- 

### 프로젝트 경로 (CubeIDE 등)
- 

### 참고 커밋
- 

### 이슈 / 메모
- 

