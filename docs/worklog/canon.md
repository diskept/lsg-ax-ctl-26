# Worklog — Canon (파트 1)

형식: 날짜별로 아래 블록을 **위에 추가** (최신이 위).

---

## 2026-03-24 (추가 디버깅 라운드)

### 한 일
- `2026-03-24.log` 18:00 이후 구간 재분석.
- 펌웨어 `[FWDIAG][UART3][STAT]` 및 Canon `debug_listener.diag` 집계 동작 확인.
- OPID 래핑 구간 대응:
  - Canon에서 `OPID=0` 전송 회피(`65535 -> 1`) 로직 추가.
- 간헐 `attempt 1/3` 실패 완화:
  - OPID-READ/WRITE 송신 전 `read_all()`로 stale 바이트 drain 후 전송.
- FWDIAG 파서 안정화:
  - 숫자형 `key=value`만 집계하여 붙은 문자열로 인한 통계 오염 감소.

### 다음 할 일
- 최신 빌드로 장시간 재시험 후 `FWDIAG UART3.STAT`/`OCS GIVEUP` 전후 비교.
- 필요 시 펌웨어 `Master_Process.c`의 OPID 비교를 wrap-aware 방식으로 개선.

### 참고 파일 / 커밋
- 파일: `src/services/actuator_test_sequence.py`, `src/services/debug_listener.py`, `src/services/fw_diag_parser.py`
- 커밋: (이번 커밋 예정)

### 이슈 / 메모
- 18시 이후 구간에서 치명 장애는 크게 줄었고, 간헐 1차 시도 실패는 남아 있음.

---

## 2026-03-24

### 한 일
- 장시간 테스트 중 무응답 장애 로그 분석 (`2026-03-24.log`) 수행.
- `actuator_test` 재시도 패턴(READ/WRITE 3회 실패 후 skip)과 채널 전체 무응답 패턴 확인.
- `debug_listener` 로그가 오전 구간 이후 끊기며, 오후에는 Canon TX만 반복되는 상태 확인.
- UI 로그창 지연 대응: `QTextEdit.document().setMaximumBlockCount(4000)` 적용, 로그 배치 flush 타이머 적용.

### 다음 할 일
- Canon 측 헬스체크(최근 RX 시각 기반 경고/상태 표시) 추가 검토.
- 통신 단절 시 자동 재연결(close/open) 옵션(수동/자동) 설계.

### 참고 파일 / 커밋
- 파일: `src/ui/main_window.py`, `src/services/actuator_test_sequence.py`
- 커밋: `9f60d53`

### 이슈 / 메모
- TX는 발생하지만 RX가 장시간 0이면 Canon 로직보다 링크/노드 쪽 정지 가능성이 큼.

---

## 2026-03-23

### 한 일
- Connect/Disconnect 동작 개선:
  - disconnect 시 테스트 stop
  - connect 성공 시 기존 테스트 reset
- 타임 테스트 시 채널별 ADDR/OPID 로그 강화.
- 재시도 정책 적용:
  - retry 3회, 간격 300ms
  - OPID-READ timeout 1.5s
  - OPID-WRITE timeout 2.0s

### 다음 할 일
- 장시간 운용 시 통신 단절 자동 감지/복구 방안 보강.

### 참고 파일 / 커밋
- 파일: `src/services/node_manager.py`, `src/services/actuator_test_sequence.py`
- 커밋: `9f60d53`

### 이슈 / 메모
- CH별 간헐 실패는 재시도로 복구되지만, 특정 시점 이후 전채널 무응답 케이스가 존재.

---

## YYYY-MM-DD

### 한 일
- 

### 다음 할 일
- 

### 참고 파일 / 커밋
- 파일: 
- 커밋: 

### 이슈 / 메모
- 
