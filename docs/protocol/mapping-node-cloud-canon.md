# Node-Cloud-Canon Mapping

## Objective

Use one protocol interpretation baseline while supporting two operation paths:

- production: Node <-> Cloud
- engineering: Node <-> Canon

## Mapping Table (Template)

| Item | Node <-> Cloud (Production) | Node <-> Canon (Engineering/Test) | Notes |
|------|------------------------------|------------------------------------|-------|
| Transport | TCP (or gateway-mediated TCP) | Serial/RS485 or local TCP bridge | Keep protocol payload compatible |
| Addressing | Standard address map | Same address map | Do not fork register semantics |
| Function codes | Standard-defined set | Same set (test extensions must be isolated) | Extensions require explicit flag |
| Timeout | Production profile | Test profile (can be looser) | Keep documented rationale |
| Retry policy | Production profile | Test profile (debug friendly) | Log per-attempt evidence |
| Exception handling | Strict, report to server | Strict + verbose diagnostics | Exception code must be preserved |
| Health monitoring | Service-level alarms | UI-level warnings/log alerts | Same core metrics |

## Required Telemetry Fields

For both paths, keep these fields in logs:

- timestamp
- node id
- channel/register address
- operation id
- function code
- request length / response length
- timeout / retry attempt
- exception code (if present)

## Compatibility Policy

- Protocol parser/encoder logic must be shared as much as possible.
- Path-specific code should stay in transport adapters and profile config.
- If a test-only behavior is introduced, it must be:
  - disabled by default
  - clearly marked in log
  - excluded from production profile

## Review Checklist

- Does Canon test behavior still represent Cloud production semantics?
- Are address/function mappings unchanged from standard intent?
- Are timeout/retry changes documented with reason and measured effect?
- Are exception and no-response cases distinguishable in logs?

