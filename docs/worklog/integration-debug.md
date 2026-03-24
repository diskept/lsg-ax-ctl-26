# Worklog — Integration Debug (파트 5)

형식: 날짜별로 아래 블록을 **위에 추가** (최신이 위).

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

