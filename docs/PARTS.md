# 파트 정의 & 공유 허브

채팅은 컨텍스트 한계가 있으므로, 상태/결론/다음 할 일은 이 파일과 `docs/worklog/`에 남긴다.

## 현재 브랜치 / 블로커 (수시 갱신)

| 파트 | 권장 브랜치 | 담당 채팅 | 블로커 / 메모 |
|------|-------------|-----------|---------------|
| 1 Canon 개발 | `main` 또는 `feature/canon-*` | `canon-dev` | |
| 2 Actuator FW | `feature/actuator-fw-*` | `actuator-fw-debug` | |
| 3 Sensor FW | `feature/sensor-fw-*` | `sensor-fw-dev` | |
| 4 Nexus Gateway | `feature/nexus-*` | `nexus-dev` | |
| 5 통합 디버그 | `feature/integration-*` | `integration-debug` | |

---

## 파트 1 — Canon 개발 (기능 / UI / 테스트 툴)

- 대상: 현재 Python/PyQt 메인 프로그램 (`LSG-AX-CTL-26`)
- 범위:
  1) UI/UX 개발
  2) 구동기/센서 노드 기본 설정 기능 및 테스트
  3) 테스트 완료 후 리포트(자체 성능서 프린트)
  4) 디버깅 기능(로그/상태 패널)
- 장기:
  - 소비자용 프로그램과 개발용 프로그램 분리 또는 모드 분리
  - 개발자 모드는 기본 숨김 + 특수 전환 경로
- 일지: `docs/worklog/canon.md`

## 파트 2 — Actuator 펌웨어 개발/디버그

- 저장소 정책:
  - `2025_ICT_LSGFARM_ACTUATORv00`는 이 레포(`LSG-AX-CTL-26`)에서 제거한다.
  - 추후 오류 수정 및 신규 구성을 거쳐 **별도 repository**로 관리한다.
- 목표:
  - 기존 오류 수정 및 안정화
  - 이후 `01.Axiom-A/LSG-AX-ADG-26` 방향으로 신규 프로젝트 구성
  - Cursor 친화 코드/디버그 단계 기능 추가
- 일지: `docs/worklog/actuator-fw.md`

## 파트 3 — Sensor 노드 펌웨어 개발

- 대상: `01.Axiom-S/LSG-AX-GNI-26` (추후 생성)
- 목표:
  - 센서 노드 펌웨어 개발/디버깅
  - Actuator와 동일한 로그/진단 체계 맞추기
- 일지: `docs/worklog/sensor-fw.md`

## 파트 4 — Nexus(게이트웨이) 개발

- 목표:
  - 서버 <-> 노드 연결 게이트웨이/데몬 프로그램 개발
  - Canon 상위 버전 또는 엣지 PC용 중간 계층
- 일지: `docs/worklog/nexus.md`

## 파트 5 — 통합 디버깅/문제 해결

- 범위:
  - Canon + 펌웨어 + 서버/게이트웨이 전 영역 이슈 통합 추적
  - 원인 분리(PC/통신/노드) 및 재현/복구 절차 정리
- 일지: `docs/worklog/integration-debug.md`

---

## 파트 간 의존성 메모

- Canon 네트워크 테스트 기능은 Gateway/서버 인터페이스 정의와 동기화 필요.
- Actuator/Sensor 신규 프로젝트의 로그 포맷은 Canon 파서와 합의 필요.
