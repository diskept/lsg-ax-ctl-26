# Protocol Standard Reference

## Purpose

This document tracks the Korea standard protocol references used by this project and
defines where each reference applies.

## Source Standards

- `KS B 7958 스마트온실 - 노드와 온실통합제어기 사이의 전송제어프로토콜(TCP) 기반 모드버스 인터페이스.pdf`
- `KS X 3267.pdf`

## System Scope Mapping

- **Current production path**: Node <-> Cloud Server
- **Local engineering/test path**: Node <-> Canon (PC tool)

Even when Canon is used locally, frame semantics and error handling should stay aligned
with the production Node <-> Cloud behavior unless a test-only mode is explicitly enabled.

## Baseline Rules For Implementation

- Keep request/response data model compatible with KS-based Modbus mapping.
- Treat timeout and retry as profile parameters, not fixed constants.
- Separate transport concerns from protocol concerns:
  - transport: serial/TCP session status, reconnect policy
  - protocol: frame fields, address map, function code, exception handling
- Log enough metadata for incident triage:
  - timestamp, channel/address, op id, function code, retry count, response length

## Profile Concept (Recommended)

Define profiles so Canon and Cloud tests can share one protocol core:

- `prod-cloud`: strict profile for production interoperability
- `canon-local`: engineering profile for local test, with controlled timing overrides

Each profile should document:

- transport type (TCP/serial bridge)
- timeout budget
- retry count/interval
- allowed test-only behaviors

## Change Control

When protocol behavior changes, update all of the following together:

1. this file (`docs/protocol/standard-reference.md`)
2. mapping document (`docs/protocol/mapping-node-cloud-canon.md`)
3. related worklog entry in `docs/worklog/`

