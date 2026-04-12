from dataclasses import dataclass


@dataclass(frozen=True)
class SystemStatus:
    raw: int
    legacy_status: int
    service_ready: bool
    link_connected: bool
    sim_mode: bool
    imu_ready: bool
    odom_ready: bool
    arm_state: int


def decode_status_word(raw_value: int) -> SystemStatus:
    value = int(raw_value) & 0xFFFFFFFF
    return SystemStatus(
        raw=value,
        legacy_status=value & 0x00FFFFFF,
        service_ready=bool(value & (1 << 24)),
        link_connected=bool(value & (1 << 25)),
        sim_mode=bool(value & (1 << 26)),
        imu_ready=bool(value & (1 << 27)),
        odom_ready=bool(value & (1 << 28)),
        arm_state=(value >> 29) & 0x03,
    )
