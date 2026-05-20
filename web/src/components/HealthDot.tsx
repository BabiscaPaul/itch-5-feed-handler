import { Tooltip } from '@mantine/core';
import { useQuery } from '@tanstack/react-query';
import { getHealth } from '../api/health';

const COLOR_OK = 'var(--terminal-green)';
const COLOR_ERR = '#ff4d4d';
const COLOR_PENDING = '#6c7280';

export function HealthDot() {
  const { data, isLoading, isError } = useQuery({
    queryKey: ['health'],
    queryFn: getHealth,
    refetchInterval: 10_000,
    retry: false,
    staleTime: 0,
  });

  const ok = data?.status === 'ok';
  const color = isLoading ? COLOR_PENDING : isError || !ok ? COLOR_ERR : COLOR_OK;

  const label = isLoading
    ? 'Checking backend…'
    : isError || !ok
    ? 'Backend unreachable'
    : `Backend OK · ${data?.runs_count ?? '?'} run${data?.runs_count === 1 ? '' : 's'}`;

  return (
    <Tooltip label={label} position="bottom" withArrow>
      <span
        className="status-dot"
        style={{ background: color, boxShadow: `0 0 8px ${color}` }}
        aria-label={label}
      />
    </Tooltip>
  );
}
