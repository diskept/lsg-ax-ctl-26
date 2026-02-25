# Axiom Canon — LSG-AX-CTL-26

Axiom Standard Node **Control Tool** (Configuration / Test / Calibration / Diagnostics)

PC 설정·로깅 프로그램. LSG-AX-ADG-26 (구동기 노드)와 세트 판매 예정.

## 프로젝트 구조

- **LSG-AX-CTL-26** — PC 측 설정 및 로깅 프로그램 (본 프로젝트)
- **LSG-AX-ADG-26** — 구동기 노드 (신규 프로젝트, 예정)
- **2025_ICT_LSGFARM_ACTUATORv00** — 구동기 노드 외주 펌웨어 (STM32F070CBT6, 분석·테스트용)

## Quick start

```bash
python -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -r requirements.txt
python main.py
# 또는
.\main.py
```

## 펌웨어 참고

- [2025_ICT_LSGFARM_ACTUATORv00 펌웨어 검토](docs/FIRMWARE_REVIEW_ACTUATORv00.md)
