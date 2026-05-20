import { useMemo, useState } from 'react';
import {
  Button,
  Center,
  Group,
  Paper,
  SegmentedControl,
  Stack,
  Switch,
  Text,
  Title,
} from '@mantine/core';
import { useQuery } from '@tanstack/react-query';
import type { AreaData, LineData } from 'lightweight-charts';
import { getBbo, type Resolution } from '../api/bbo';
import { API_BASE } from '../api/config';
import { bucketToUnixSeconds } from '../api/time';
import { BBOChart } from '../components/BBOChart';
import { SpreadChart } from '../components/SpreadChart';
import { SymbolPicker } from '../components/SymbolPicker';
import { Loading } from '../components/Loading';
import { ErrorBanner } from '../components/ErrorBanner';
import { useAppStore } from '../store/app';
import { fmtInt } from '../utils/format';

const RESOLUTIONS: Resolution[] = ['1m', '5m'];

export function BBOExplorerPage() {
  const runId = useAppStore((s) => s.selectedRunId);
  const symbol = useAppStore((s) => s.selectedSymbol);

  const [resolution, setResolution] = useState<Resolution>('1m');
  const [regularHours, setRegularHours] = useState(true);

  const bboQuery = useQuery({
    queryKey: ['bbo', runId, symbol, resolution, regularHours],
    queryFn: () =>
      getBbo({
        runId: runId!,
        symbol: symbol!,
        resolution,
        regularHours,
      }),
    enabled: !!runId && !!symbol,
  });

  const { bid, ask, spread } = useMemo(() => {
    const buckets = bboQuery.data ?? [];
    const bid: LineData[] = [];
    const ask: LineData[] = [];
    const spread: AreaData[] = [];
    for (const b of buckets) {
      const time = bucketToUnixSeconds(runId!, b.bucket_ts_ns);
      bid.push({ time, value: b.bid_close });
      ask.push({ time, value: b.ask_close });
      spread.push({ time, value: b.spread_avg });
    }
    return { bid, ask, spread };
  }, [bboQuery.data, runId]);

  if (!runId) {
    return (
      <Center h={400}>
        <Text
          c="dimmed"
          ff="monospace"
          size="sm"
          style={{ letterSpacing: '0.12em', textTransform: 'uppercase' }}
        >
          Select a run to get started
        </Text>
      </Center>
    );
  }

  return (
    <Stack gap="xl">
      <Stack gap={8}>
        <Title order={2}>BBO Explorer</Title>
        <Text c="dimmed" size="sm">
          Top-of-book bid/ask over time, bucketed and downsampled server-side.
        </Text>
      </Stack>

      <Paper
        p="md"
        withBorder
        radius="xs"
        style={{ background: '#0b0e14', borderColor: 'var(--terminal-border)' }}
      >
        <Group justify="space-between" align="flex-end" wrap="wrap" gap="md">
          <SymbolPicker />
          <Group gap="md" align="flex-end">
            <Stack gap={4}>
              <Text className="term-label">Resolution</Text>
              <SegmentedControl
                value={resolution}
                onChange={(v) => setResolution(v as Resolution)}
                data={RESOLUTIONS.map((r) => ({ label: r, value: r }))}
                size="xs"
              />
            </Stack>
            <Stack gap={4}>
              <Text className="term-label">Regular hours</Text>
              <Switch
                checked={regularHours}
                onChange={(e) => setRegularHours(e.currentTarget.checked)}
                size="md"
                onLabel="ON"
                offLabel="OFF"
              />
            </Stack>
            <Stack gap={4}>
              <Text className="term-label">Download</Text>
              <Button
                component="a"
                href={
                  symbol
                    ? `${API_BASE}/runs/${encodeURIComponent(runId)}/files/bbo?symbol=${encodeURIComponent(symbol)}`
                    : undefined
                }
                download
                disabled={!symbol}
                variant="default"
                size="sm"
                ff="monospace"
              >
                ↓ {symbol ?? '…'} BBO
              </Button>
            </Stack>
          </Group>
        </Group>
      </Paper>

      {!symbol ? (
        <Center h={300}>
          <Text
            c="dimmed"
            ff="monospace"
            size="sm"
            style={{ letterSpacing: '0.12em', textTransform: 'uppercase' }}
          >
            Pick a symbol to load BBO
          </Text>
        </Center>
      ) : bboQuery.isError ? (
        <ErrorBanner error={bboQuery.error} title="Failed to load BBO" />
      ) : bboQuery.isLoading ? (
        <Loading label="Loading BBO" />
      ) : bboQuery.data ? (
        <Stack gap="md">
          <Paper
            p="md"
            withBorder
            radius="xs"
            style={{ background: '#0b0e14', borderColor: 'var(--terminal-border)' }}
          >
            <Stack gap="sm">
              <Group justify="space-between" align="flex-end">
                <Stack gap={2}>
                  <Title order={4}>
                    {symbol}{' '}
                    <Text component="span" c="dimmed" size="sm" ff="monospace">
                      bid / ask
                    </Text>
                  </Title>
                  <Text size="xs" c="dimmed" ff="monospace">
                    {fmtInt(bid.length)} buckets · {resolution} ·{' '}
                    {regularHours ? 'regular hours' : 'full session'}
                  </Text>
                </Stack>
              </Group>
              <BBOChart bid={bid} ask={ask} />
            </Stack>
          </Paper>

          <Paper
            p="md"
            withBorder
            radius="xs"
            style={{ background: '#0b0e14', borderColor: 'var(--terminal-border)' }}
          >
            <Stack gap="sm">
              <Stack gap={2}>
                <Title order={4}>
                  Spread{' '}
                  <Text component="span" c="dimmed" size="sm" ff="monospace">
                    avg(ask − bid) per bucket
                  </Text>
                </Title>
              </Stack>
              <SpreadChart data={spread} />
            </Stack>
          </Paper>
        </Stack>
      ) : null}
    </Stack>
  );
}
