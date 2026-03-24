# LSG-AX-CTL-26 — 전체 개요

## 목적

Axiom 프로젝트를 병렬(멀티 채팅)로 진행하되, 같은 기준 문서와 git 이력으로
연속성을 유지한다.

## 병렬 파트

1. Canon 개발 (기능 / UI / 테스트 / 리포트 / 디버깅 UI)
2. Actuator 펌웨어 개발/디버그
3. Sensor 펌웨어 개발
4. Nexus 게이트웨이(데몬) 개발
5. 통합 디버깅/문제 해결

## 운영 원칙

- 채팅은 파트별로 분리한다.
- 진행 상태는 `docs/PARTS.md` + `docs/worklog/*.md`에 남긴다.
- 코드 진실은 git 커밋/브랜치로 관리한다.
- Actuator 펌웨어는 Canon 레포와 분리해 별도 repository로 운영한다.

## 문서

- `docs/PARTS.md`: 파트 허브 문서 (브랜치/블로커/의존성)
- `docs/worklog/*.md`: 파트별 작업 로그
- `docs/decisions/`: 중요한 구조/정책 결정(선택)
- `docs/protocol/*.md`: KS 표준 기반 Node-Cloud-Canon 프로토콜 적용 기준

## 새 채팅 시작 템플릿

```text
파트: (canon / actuator-fw / sensor-fw / nexus / integration)
브랜치: feature/<part>-<topic>
참고: @docs/PARTS.md @docs/worklog/<part>.md
오늘 할 일: (한 줄)
```
