# 구동기 노드 펌웨어 검토 — 2025_ICT_LSGFARM_ACTUATORv00

## 1. 개요

- **MCU**: STM32F070CBT6  
- **툴**: STM32CubeIDE 2.0.0  
- **역할**: KS X 3267 기반 구동기 노드 (스위치 16CH + 개폐기 8/16CH, Modbus 485)  
- **통신**: PC(마스터) ↔ 노드 USART1(RS485), 노드 USART3 ↔ 릴레이 보드(RS485)

---

## 2. 하드웨어·UART 구성

| UART | 용도 | 초기 Baud | 비고 |
|------|------|-----------|------|
| USART1 | 노드 ↔ PC(설정/테스트 툴) | 9600 | RS485 DE: PA12 (GHC_TX/RX_Enable) |
| USART2 | 네트워크 | 38400 | |
| USART3 | 노드 ↔ 릴레이 보드 | 9600 | RS485 DE: PB1 (ACT_TX/RX_Enable), 런타임 변경 가능 |
| USART4 | 디버그 출력 | - | `_write()` 리다이렉트 |

- **노드 주소**: GPIO PB3,PB4,PB5,PB13,PB14 (5비트, 1~31). 브로드캐스트 시 슬레이브 ID `0xFF` 수신 가능.

---

## 3. 프로토콜 (Modbus RTU + KS X 3267)

### 3.1 프레임 형식

- **표준 Modbus RTU**: [SlaveID][Function][Data...][CRC16 Lo][CRC16 Hi]
- CRC: Modbus 표준 (crc_table 사용, LSB 선행).

### 3.2 기능 코드

| 기능 코드 | 이름 | 용도 |
|-----------|------|------|
| 0x03 | Read Holding Registers | 노드 레지스터 읽기 (주소 1~630) |
| 0x04 | Read Input Registers | 특수 주소: 0xF001(ID), 0xF002(노드 Baud), 0xF003(구동기 Baud) |
| 0x06 | Write Single Register | 0xF000 리셋, 0xF002 노드 Baud, 0xF003 구동기 Baud |
| 0x10 | Write Multiple Registers | 노드/스위치/개폐기 제어 (501~629) |

### 3.3 레지스터 맵 (요약)

- **1~7**: 노드 정보 (기관코드, 제품타입=2, 프로토콜버전, 채널수 24/32, 시리얼 등)
- **101~132**: 스위치/개폐기 설정 주소 (SWITCH_n_ADDR, OCSWITCH_n_ADDR)
- **201~329**: 상태 (ACTNODE_OPID_0, ACTNODE_STATUS, 각 SW/OC 상태·OPID·시간)
- **501~629**: 제어 (CON_ACTNODE_CMD/OPID, CON_SWITCH_n_*, CON_OCSWITCH_n_*)
- **0xF000**: 명령(리셋 등), **0xF001~0xF003**: ID/노드 Baud/구동기 Baud

### 3.4 보드레이트 코드 (EEPROM/레지스터 값)

- **노드(USART1) / 구동기(USART3) 공통**:  
  `0x0001`=9600, `0x0002`=19200, `0x0003`=38400, `0x0004`=57600, `0x0005`=115200  
- **기본값**: `DEFAULT_NODE_BAUD = 0x0001`, `DEFAULT_ACTUATOR_BAUD = 0x0003` (define.h).

---

## 4. KS X 3267 관련 정의 (define.h / struct.h)

- **제어 모드**: CONTROL_LOCAL(1), CONTROL_REMOTE(2), CONTROL_MANUAL(3)  
- **스위치 명령**: CONTROL_OFF(0), CONTROL_ON(201), CONTROL_TIMED_ON(202) 등  
- **개폐기 명령**: CONTROL_OPEN(301), CONTROL_CLOSE(302), CONTROL_TIMED_OPEN(303) 등  
- **상태 코드**: COMMON_ON(201), COMMON_OPENING(301), COMMON_CLOSING(302) 등  
- **노드 버전**: `NodeVersion` = RELAY_16CH_VER(2) or RELAY_32CH_VER(3) → 스위치 16 + 개폐기 8/16 또는 16+16.

---

## 5. 동작 흐름 (요약)

1. **전원/리셋**  
   EEPROM CONFIG_CHECK → NODE_CONFIG_NONE / DEFAULT_WRITE_OK / INSTALL_WRITE_OK 에 따라 Default/Install 설정 로드, 노드·구동기 Baud 적용.

2. **마스터(PC) → 노드**  
   USART1 수신 → `Master_RS485_Push_Data` → `Master_Buffer_Process` → 패킷 완료 시 `Master_Packet_Process`.  
   - 패킷 끝 판단: 수신 중단 후 `ActNode.nRxEndTime` 만료 (RX_END_TIME = 4, 단위는 틱).

3. **노드 → 릴레이**  
   `Actuator_OCS_Relay_Control_Proecess`, `Actuator_SW_Relay_Control_Proecess` 등에서 USART3로 Modbus 전송.

4. **릴레이 → 노드**  
   USART3 수신 → `Actuator_RS485_Push_Data` → `Actuator_Buffer_Process` → CRC 검사 후 상태 반영.

---

## 6. 검토 시 유의사항 및 잠재 이슈

1. **마스터 수신 버퍼 오버플로우 (수정 완료)**  
   `Master_RS485_Push_Data`를 `Actuator_RS485_Push_Data`와 동일하게 수정: 버퍼 풀 시 Process 인덱스를 진행시켜 가장 오래된 바이트를 버리고, 최신 수신만 유지하도록 함.

2. **패킷 끝 판단 (4ms)**  
   `RX_END_TIME = 4` (4 틱)으로 프레임 종료 판단. 저속(9600)에서 프레임이 길면 4ms 미만 간격으로 연속 수신 시 이론상 프레임 분리 오인 가능. 필요 시 3.5 character time 이상으로 여유 두는 방식 검토.

3. **Read 레지스터 수량 상한 (예외 허용)**  
   Modbus 표준은 레지스터 Read 수량 최대 125(0x7D).  
   본 펌웨어는 **의도적으로 최대 135(0x87)까지 허용** (예외 처리). 해당 한도 유지 및 주석으로 명시함.

4. **EEPROM 보드레이트 저장**  
   `CONFIG_DATA.nNode485Baud`, `nAct485Baud`는 `uint16_t`인데, EEPROM에는 `E2P_Write_Byte`로 1바이트씩만 저장(Load 시에도 1바이트씩).  
   현재 사용 코드(0x0001~0x0005)는 하위 1바이트만 있어도 동작하지만, 상위 바이트가 의미 있게 쓰일 경우 저장/복원 불일치 가능.  
   장기적으로는 2바이트씩 저장/로드하거나, 사용 범위를 0~255로 고정하는 것이 명확함.

5. **디버그 출력**  
   `Debug_printf`는 USART4로 출력. 실제 보드에서 디버그 포트 미연결 시 불필요한 블로킹이나 버퍼 쌓임 가능.  
   `MASTER_DEBUG`, `ACTUATOR_DEBUG` 매크로로 로그 최소화 가능(define.h).

6. **Actuator 링버퍼**  
   `Actuator_RS485_Push_Data`에 오버플로우 처리 및 인덱스 wrap 수정(25.12.31) 적용됨.  
   통신 로그/오류 시 이 버퍼와 `nRxOverflowCnt` 확인하면 수신 쪽 이슈 분석에 도움됨.

---

## 7. 테스트 프로그램(LSG-AX-CTL-26) 연동 시 참고

- **연결**: PC는 노드 USART1(RS485)에 해당하는 COM 포트로 접속.  
- **초기 통신**: 노드 기본 노드 Baud 9600, 구동기 38400.  
- **노드 주소**: 1~31 또는 브로드캐스트 0xFF(해당 처리되는 명령만).  
- **레지스터 읽기**: 0x03으로 1번지부터 필요한 개수만큼 읽기 (수량 ≤125 권장).  
- **보드레이트 변경**: 0x04로 0xF002/0xF003 읽어 현재 값 확인 후, 0x06으로 0x0001~0x0005 기록.  
  변경 후에는 해당 UART를 선택한 Baud로 다시 오픈해야 함.

---

## 8. 요약

- 펌웨어는 **Modbus RTU over RS485**와 **KS X 3267** 주소·코드를 따르며, 스위치/개폐기 제어와 노드·구동기 보드레이트 설정이 가능함.  
- 통신 로그/오류 원인 분석 시에는 **마스터 수신 버퍼 처리**, **패킷 끝 타이밍**, **Read 수량 상한**, **EEPROM 보드레이트 저장 방식**을 우선 확인하는 것이 좋음.  
- 위 사항들을 반영한 수정·보완 후, 신규 프로젝트 **LSG-AX-ADG-26**으로 재구성하는 흐름을 권장함.
