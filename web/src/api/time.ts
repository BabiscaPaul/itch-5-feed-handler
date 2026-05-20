import type { UTCTimestamp } from 'lightweight-charts';

const NS_PER_SECOND = 1_000_000_000;

/**
 * Convert a backend timestamp (ns since midnight ET on the run's date)
 * to a UNIX timestamp suitable for lightweight-charts.
 *
 * We treat ET-midnight as UTC-midnight for display purposes — the chart
 * axis ends up showing "09:30:00" wall-clock, which is what we want. We
 * never compare timestamps across runs, so the absolute UTC offset is
 * irrelevant.
 */
export function bucketToUnixSeconds(runId: string, tsNs: number): UTCTimestamp {
  const [year, month, day] = runId.split('-').map(Number);
  const midnightUnix = Date.UTC(year, month - 1, day) / 1000;
  return (midnightUnix + tsNs / NS_PER_SECOND) as UTCTimestamp;
}
