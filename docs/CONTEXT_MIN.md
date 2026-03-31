# Context Minimal Guide

## 목적

긴 대화 기록 없이도 다음 채팅에서 즉시 작업을 재개하기 위한 최소 컨텍스트 기준.

## 현재 기준 상태

- **2026-03-28**: UART3 스톨 감시·STAT 확장·소프트/하드 복구 펌웨어 패치 + Canon FW idle / Modbus streak / 상태바. → `docs/worklog/integration-debug.md` (`2026-03-28`).
- **2026-03-27 통합 장애 요약**: 전 채널 타이머 종료·릴레이 OFF 후 `~08:41:33`부터 `debug_listener` 단절, 이후 Modbus 무응답. 상세·원인·대응 → `@docs/worklog/integration-debug.md` (섹션 `2026-03-27`).
- Canon 레포 최신 기준 커밋: `2c094d6`
- 최근 핵심 변경:
  - `src/services/actuator_test_sequence.py`
    - OPID wrap 시 `0` 대신 `1` 전송
    - read/write 전 stale byte drain
  - `src/services/debug_listener.py`
    - `FWDIAG` 이벤트 집계 로깅 추가
  - `src/services/fw_diag_parser.py`
    - `FWDIAG` 파싱 안정화
- 참고 로그: `.../logs/2026/03/2026-03-24.log` (18:00 이후 구간 확인 완료)

## 다음 채팅 시작 템플릿 (복붙용)

```text
작업 파트: Canon 통신 디버깅
기준 커밋: 2c094d6
기준 문서: @docs/worklog/canon.md @docs/CONTEXT_MIN.md
오늘 할 일: (한 줄로 구체적으로)
```

## 컨텍스트 절약 규칙

1. 긴 배경 설명 대신 아래 4개만 전달:
   - 작업 파트
   - 기준 커밋
   - 기준 문서 경로
   - 오늘 할 일 1줄
2. 로그 분석 요청 시:
   - 파일 경로
   - 시간 구간 (예: `18:00~18:40`)
   - 확인 포인트 키워드 (`FWDIAG`, `give up`, `RX error`)
3. 펌웨어/Canon 병행 시 채팅 분리:
   - Canon 채팅: Python/UI/로그 파서
   - Firmware 채팅: C/USART/OPID 로직

## 즉시 재개 체크리스트

- Canon 실행 후 `FWDIAG` 키 로그가 나오는지 확인
- `attempt 1/3` 빈도와 `give up` 발생 여부 확인
- OPID wrap 경계(`65535 -> ...`)에서 거부 로그 유무 확인

