import re
from dataclasses import dataclass


_FWDIAG_RE = re.compile(
    r"\[FWDIAG\]\[(?P<group>[^\]]+)\]\[(?P<event>[^\]]+)\]\s*(?P<body>.*)$"
)
_KV_NUMERIC_RE = re.compile(r"^(0x[0-9A-Fa-f]+|[0-9]+)$")


@dataclass
class FwDiagEvent:
    group: str
    event: str
    values: dict[str, str]


def parse_fw_diag_line(line: str) -> FwDiagEvent | None:
    m = _FWDIAG_RE.search(line)
    if not m:
        return None
    body = m.group("body").strip()
    values: dict[str, str] = {}
    for token in body.split():
        if "=" not in token:
            continue
        k, v = token.split("=", 1)
        if not k:
            continue
        # FWDIAG fields are designed as numeric counters/flags.
        # Ignore concatenated trailing debug fragments to keep stats clean.
        if _KV_NUMERIC_RE.match(v):
            values[k] = v
    return FwDiagEvent(
        group=m.group("group"),
        event=m.group("event"),
        values=values,
    )

