import {
  Button,
  Center,
  Group,
  Paper,
  SimpleGrid,
  Stack,
  Table,
  Text,
  Title,
} from '@mantine/core';
import { useQuery } from '@tanstack/react-query';
import { API_BASE } from '../api/config';
import { getRun, listSymbols } from '../api/runs';
import { KpiCard } from '../components/KpiCard';
import { Loading } from '../components/Loading';
import { ErrorBanner } from '../components/ErrorBanner';
import { useAppStore } from '../store/app';
import { fmtBytes, fmtDecimal, fmtInt, fmtUsd } from '../utils/format';

export function OverviewPage() {
  const runId = useAppStore((s) => s.selectedRunId);

  const runQuery = useQuery({
    queryKey: ['run', runId],
    queryFn: () => getRun(runId!),
    enabled: !!runId,
  });

  const symbolsQuery = useQuery({
    queryKey: ['symbols', runId],
    queryFn: () => listSymbols(runId!),
    enabled: !!runId,
  });

  if (!runId) {
    return (
      <Center h={400}>
        <Text c="dimmed" ff="monospace" size="sm" style={{ letterSpacing: '0.12em', textTransform: 'uppercase' }}>
          Select a run to get started
        </Text>
      </Center>
    );
  }

  const totalBytes = runQuery.data
    ? Object.values(runQuery.data.files).reduce((acc, n) => acc + (n as number), 0)
    : 0;

  const bboSize = runQuery.data?.files['bbo.csv'];

  return (
    <Stack gap="xl">
      <Group justify="space-between" align="flex-end" wrap="wrap" gap="md">
        <Stack gap={8}>
          <Title order={2}>Overview</Title>
          <Text c="dimmed" size="sm">
            Run{' '}
            <Text component="span" ff="monospace" c="amber.5">
              {runId}
            </Text>{' '}
            — derived from per-symbol BBO and trade output written by the C++ engine.
          </Text>
        </Stack>
        {bboSize !== undefined && (
          <Button
            component="a"
            href={`${API_BASE}/runs/${encodeURIComponent(runId)}/files/bbo`}
            download
            variant="default"
            size="sm"
            ff="monospace"
          >
            ↓ Download bbo.csv ({fmtBytes(bboSize)})
          </Button>
        )}
      </Group>

      {runQuery.isError && <ErrorBanner error={runQuery.error} title="Failed to load run" />}

      {runQuery.isLoading ? (
        <Loading label="Loading run details" />
      ) : runQuery.data ? (
        <SimpleGrid cols={{ base: 2, sm: 3, lg: 6 }} spacing="md">
          <KpiCard label="BBO rows" value={fmtInt(runQuery.data.bbo_rows)} />
          <KpiCard label="Trade rows" value={fmtInt(runQuery.data.trades_rows)} />
          <KpiCard label="Symbols" value={fmtInt(runQuery.data.symbols)} />
          <KpiCard label="First tick" value={runQuery.data.first_ts.slice(0, 12)} unit="ET" />
          <KpiCard label="Last tick" value={runQuery.data.last_ts.slice(0, 12)} unit="ET" />
          <KpiCard label="On disk" value={fmtBytes(totalBytes)} />
        </SimpleGrid>
      ) : null}

      <Paper
        p="md"
        withBorder
        radius="xs"
        style={{ background: '#0b0e14', borderColor: 'var(--terminal-border)' }}
      >
        <Stack gap="md">
          <Group justify="space-between" align="flex-end">
            <Stack gap={2}>
              <Title order={3}>Symbols by trade count</Title>
              <Text size="xs" c="dimmed">
                All symbols present in trades.csv, descending
              </Text>
            </Stack>
          </Group>

          {symbolsQuery.isError && (
            <ErrorBanner error={symbolsQuery.error} title="Failed to load symbols" />
          )}

          {symbolsQuery.isLoading ? (
            <Loading label="Aggregating symbols" />
          ) : symbolsQuery.data ? (
            <Table
              withRowBorders
              highlightOnHover
              verticalSpacing="xs"
              horizontalSpacing="md"
              stickyHeader
            >
              <Table.Thead>
                <Table.Tr>
                  <Table.Th w={56}>
                    <Text className="term-label">#</Text>
                  </Table.Th>
                  <Table.Th>
                    <Text className="term-label">Symbol</Text>
                  </Table.Th>
                  <Table.Th ta="right">
                    <Text className="term-label">Trades</Text>
                  </Table.Th>
                  <Table.Th ta="right">
                    <Text className="term-label">Shares</Text>
                  </Table.Th>
                  <Table.Th ta="right">
                    <Text className="term-label">VWAP</Text>
                  </Table.Th>
                  <Table.Th ta="right">
                    <Text className="term-label">First</Text>
                  </Table.Th>
                  <Table.Th ta="right">
                    <Text className="term-label">Last</Text>
                  </Table.Th>
                </Table.Tr>
              </Table.Thead>
              <Table.Tbody>
                {symbolsQuery.data.map((row, i) => (
                  <Table.Tr key={row.symbol}>
                    <Table.Td>
                      <Text ff="monospace" c="dimmed" size="sm">
                        {i + 1}
                      </Text>
                    </Table.Td>
                    <Table.Td>
                      <Text ff="monospace" fw={600} c="amber.5">
                        {row.symbol}
                      </Text>
                    </Table.Td>
                    <Table.Td ta="right">
                      <Text className="term-value" size="sm">
                        {fmtInt(row.trades)}
                      </Text>
                    </Table.Td>
                    <Table.Td ta="right">
                      <Text className="term-value" size="sm">
                        {fmtInt(row.shares)}
                      </Text>
                    </Table.Td>
                    <Table.Td ta="right">
                      <Text className="term-value" size="sm">
                        {fmtUsd(row.vwap)}
                      </Text>
                    </Table.Td>
                    <Table.Td ta="right">
                      <Text className="term-value" size="sm">
                        {fmtDecimal(row.first_price, 2)}
                      </Text>
                    </Table.Td>
                    <Table.Td ta="right">
                      <Text className="term-value" size="sm">
                        {fmtDecimal(row.last_price, 2)}
                      </Text>
                    </Table.Td>
                  </Table.Tr>
                ))}
              </Table.Tbody>
            </Table>
          ) : null}
        </Stack>
      </Paper>
    </Stack>
  );
}
