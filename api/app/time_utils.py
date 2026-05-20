NS_PER_SECOND = 1_000_000_000
NS_PER_MINUTE = 60 * NS_PER_SECOND
NS_PER_HOUR = 60 * NS_PER_MINUTE

REGULAR_OPEN_NS = 9 * NS_PER_HOUR + 30 * NS_PER_MINUTE
REGULAR_CLOSE_NS = 16 * NS_PER_HOUR


def ns_to_iso(ts_ns: int) -> str:
    if ts_ns < 0:
        raise ValueError(f"ts_ns must be non-negative, got {ts_ns}")
    secs, frac = divmod(ts_ns, NS_PER_SECOND)
    hours, secs = divmod(secs, 3600)
    minutes, seconds = divmod(secs, 60)
    return f"{hours:02d}:{minutes:02d}:{seconds:02d}.{frac:09d}"


def iso_to_ns(iso: str) -> int:
    h, m, rest = iso.split(":", 2)
    if "." in rest:
        s, frac = rest.split(".", 1)
        frac = frac.ljust(9, "0")[:9]
    else:
        s, frac = rest, "0" * 9
    return int(h) * NS_PER_HOUR + int(m) * NS_PER_MINUTE + int(s) * NS_PER_SECOND + int(frac)


def regular_hours_window() -> tuple[int, int]:
    return REGULAR_OPEN_NS, REGULAR_CLOSE_NS
