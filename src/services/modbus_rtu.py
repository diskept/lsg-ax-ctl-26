import time


def crc16_modbus(data: bytes) -> int:
    """Compute Modbus RTU CRC16 (poly 0xA001), return 0..0xFFFF."""
    crc = 0xFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
    return crc & 0xFFFF


def build_write_multiple_registers_request(
    slave_id: int,
    start_addr: int,
    values: list[int],
) -> bytes:
    qty = len(values)
    if not (1 <= slave_id <= 247):
        raise ValueError("slave_id out of range")
    if not (0 <= start_addr <= 0xFFFF):
        raise ValueError("start_addr out of range")
    if not (1 <= qty <= 0x007B + 10):  # allow project exception headroom
        raise ValueError("qty out of range")
    payload = bytearray()
    payload.append(slave_id & 0xFF)
    payload.append(0x10)
    payload.extend([(start_addr >> 8) & 0xFF, start_addr & 0xFF])
    payload.extend([(qty >> 8) & 0xFF, qty & 0xFF])
    payload.append(qty * 2)
    for v in values:
        v &= 0xFFFF
        payload.extend([(v >> 8) & 0xFF, v & 0xFF])
    crc = crc16_modbus(bytes(payload))
    payload.extend([crc & 0xFF, (crc >> 8) & 0xFF])  # little-endian CRC
    return bytes(payload)


def build_read_holding_registers_request(
    slave_id: int,
    start_addr: int,
    quantity: int,
) -> bytes:
    """Build Modbus RTU 0x03 Read Holding Registers request."""
    if not (1 <= slave_id <= 247):
        raise ValueError("slave_id out of range")
    if not (0 <= start_addr <= 0xFFFF):
        raise ValueError("start_addr out of range")
    if not (1 <= quantity <= 0x007B + 10):
        raise ValueError("quantity out of range")
    payload = bytearray()
    payload.append(slave_id & 0xFF)
    payload.append(0x03)
    payload.extend([(start_addr >> 8) & 0xFF, start_addr & 0xFF])
    payload.extend([(quantity >> 8) & 0xFF, quantity & 0xFF])
    crc = crc16_modbus(bytes(payload))
    payload.extend([crc & 0xFF, (crc >> 8) & 0xFF])
    return bytes(payload)


def hex_bytes(data: bytes) -> str:
    """Return 'AA BB CC' style hex string."""
    return " ".join(f"{b:02X}" for b in data)


def read_exact(read_fn, n: int, timeout_s: float) -> bytes:
    """Read exactly n bytes using read_fn(size)->bytes, until timeout."""
    end = time.monotonic() + max(0.0, timeout_s)
    buf = bytearray()
    while len(buf) < n and time.monotonic() < end:
        chunk = read_fn(n - len(buf))
        if chunk:
            buf.extend(chunk)
        else:
            time.sleep(0.002)
    return bytes(buf)

