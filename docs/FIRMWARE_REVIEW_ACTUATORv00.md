# 구동기 노드 펌웨어 검토 — 2025_ICT_LSGFARM_ACTUATORv00

## 1. 개요

| 항목 | 내용 |
|------|------|
| MCU | STM32F070CBT6 |
| 개발 도구 | STM32CubeIDE 2.0.0 |
| 프로토콜 | Modbus RTU over RS-485 (KS X 3267 기반) |
| 상태 | 외주 개발, 오류 발생 — 원인 분석 및 LSG-AX-ADG-26 신규 개발 예정 |

---

## 2. UART 구성

| UART | 용도 | RS-485 DE | 비고 |
|------|------|-----------|------|
| UART1 | **Master (PC ↔ 노드)** | PA12 (GHC) | PC 프로그램과 통신 |
| UART2 | Network | — | 내부 네트워크 |
| UART3 | **Actuator (노드 ↔ 릴레이)** | PB1 (ACT) | 16CH/32CH 릴레이 보드 |
| UART4 | Debug | — | printf, Debug_printf |

---

## 3. 통신 파라미터

### Master (UART1) — PC ↔ 노드

- 초기: **9600 8N1**
- EEPROM 설정에 따라: 9600 또는 115200
- `DEFAULT_NODE_BAUD = 0x0001` → 9600  
  `DEFAULT_ACTUATOR_BAUD = 0x0003` → 115200

### Actuator (UART3) — 노드 ↔ 릴레이

- 초기: **9600 8N1**
- EEPROM `nAct485Baud`: 0x0001→9600, 0x0003→115200

### PC 프로그램(LSG-AX-CTL-26) 설정

- 포트: COMx (연결된 시리얼)
- Baud: **9600** (기본) 또는 115200 — 노드 EEPROM 설정과 일치해야 함
- 8N1, RS-485

---

## 4. Modbus RTU 프로토콜 (Master/PC → 노드)

### 4.1 노드 주소 (Slave ID)

- **1~31**: GPIO (NODE_ID1~5)로 설정
- **0xFF**: 브로드캐스트 (일부 기능용)

### 4.2 사용 함수 코드

| 코드 | 이름 | 용도 |
|------|------|------|
| 0x03 | Read Holding Registers | 일반 레지스터 읽기 |
| 0x04 | Read Input Registers | 특수 주소 (ID/보드레이트 조회) |
| 0x06 | Write Single Register | 보드레이트 변경, 공장 초기화 |
| 0x10 | Write Multiple Registers | 제어 명령 (스위치, 개폐기) |

### 4.3 레지스터 맵 (주요)

#### 노드 정보 (읽기)

| 주소 | 내용 |
|------|------|
| 1 | 기관 코드 |
| 2 | 회사 코드 |
| 3 | 제품 유형 (2=구동기) |
| 4 | 제품 코드 |
| 5 | 프로토콜 버전 |
| 6 | 채널 수 (24 또는 32) |
| 7~8 | 시리얼 번호 |

#### 스위치 설정 (101~116, 117~132)

- 101~116: SWITCH 1~16 타입 (102=스위치)
- 117~132: OCSWITCH 1~16 타입 (112=개폐기)

#### 제어 주소

| 주소 | 내용 |
|------|------|
| 501 | 구동기 노드 제어권 (1=로컬, 2=원격, 3=수동) |
| 502 | OPID #0 |
| 503~565 | 스위치 1~16 제어 (CMD, OPID, Time 4바이트) |
| 567~629 | 개폐기 1~16 제어 |

#### 특수 주소

| 주소 | 기능 |
|------|------|
| 0xF000 | 공장 초기화 (Write Single) |
| 0xF001 | 노드 ID 확인 (Read Input) |
| 0xF002 | 노드 보드레이트 (Read/Write) |
| 0xF003 | Actuator 보드레이트 (Read/Write) |

---

## 5. 노드 버전 (NodeVersion)

| 값 | 구성 |
|----|------|
| RELAY_16CH_VER (2) | 스위치 16ch + 개폐기 8ch (16ch 릴레이 2개) |
| RELAY_32CH_VER (3) | 스위치 16ch + 개폐기 16ch (32ch 릴레이 1개) |

현재 `define.h`: `NodeVersion = RELAY_32CH_VER`

---

## 6. 릴레이 보드 Modbus (UART3)

### Slave ID

- 0x01: SW 16ch
- 0x02: OCS 16ch (첫 번째)
- 0x03: OCS 16ch EXT (두 번째, 32ch 구성 시)

### 명령

- **Read (0x03)**: Holding Register 0x0000, 2워드 → 32비트 릴레이 상태
- **Write Multiple Coils (0x0F)**: 주소 0x0000, 32코일 → 릴레이 제어

---

## 7. 알려진 문제 및 개선 포인트

### 7.1 OCS EXT 응답 처리 오류 (버그)

**파일**: `Actuator_Process.c` 606~615행

OCS EXT Write 응답 처리에서 `Actuator_OCS_Control.State`를 검사함 (OCS용).  
OCS EXT는 `Actuator_OCS_EXT_Control.State`를 사용해야 함.

```c
// 현재 (오류)
else if(Actuator_Ring_Buffer.Acutator == ACTUATOR_OCS_EXT_RELAY16CH_WRITE)
{
    if(pPacket[1] == 0x0F && pPacket[5] == 0x20)
    {
        Actuator_OCS_Control.State = ACTUATOR_SEND_OK;  // ← 잘못된 구조체
```

**수정 예시**:
```c
Actuator_OCS_EXT_Control.State = ACTUATOR_SEND_OK;
```

### 7.2 Master 링 버퍼 오버플로우 미처리

`Master_RS485_Push_Data`에는 오버플로우 처리가 없음.  
Actuator 링 버퍼는 `Actuator_RS485_Push_Data`에서 오버플로우 시 `nRxOverflowCnt` 증가 및 버퍼 정리 로직이 있음 (25.12.31 수정).

Master도 동일하게 오버플로우 대응이 필요함.

### 7.3 패킷 종료 판단

`ActNode.nRxEndTime`이 0이 되었을 때 패킷 종료로 간주.  
1ms 타이머에서 감소, `RX_END_TIME = 4` → 수신 후 4ms 무응답 시 종료.

- 고속 전송/다중 패킷에서 경계 오인 가능성 있음.
- Modbus RTU 권장: 3.5캐릭터 타임 이상 공백 시 프레임 종료.

### 7.4 Read Input vs Read Holding

- 0xF001, 0xF002, 0xF003: **Read Input (0x04)** 로 처리.
- 일반 데이터: **Read Holding (0x03)**.
- PC 프로그램 구현 시 이 구분을 준수해야 함.

### 7.5 보드레이트 코드

| 코드 | 보드레이트 |
|------|------------|
| 0x0001 | 9600 |
| 0x0003 | 115200 |

---

## 8. PC 프로그램(LSG-AX-CTL-26) 연동 시 참고

1. **연결**: RS-485를 통한 UART1 (Master) 연결
2. **초기 설정**: 9600 8N1로 시도, 필요 시 115200
3. **노드 주소**: 1~31 (하드웨어 점퍼/GPIO 설정 확인)
4. **레지스터**: `define.h` 레지스터 주소 및 `struct.h` 데이터 구조 참고
5. **CRC**: Modbus RTU CRC-16 (LSB 먼저)

---

## 9. 디버그 출력 (UART4)

`Debug_printf`로 다음 정보 출력:

- 부팅 메시지
- 패킷 수신/처리 결과
- 릴레이 읽기/쓰기 상태
- 보드레이트 변경 로그

통신 로그 수집 시 UART4를 PC에 연결해 시리얼 터미널로 확인 가능.
