import { useEffect, useState } from 'react';
import {
  Button,
  Center,
  Group,
  Pagination,
  Paper,
  Select,
  Stack,
  Table,
  Text,
  Title,
} from '@mantine/core';
import { keepPreviousData, useQuery } from '@tanstack/react-query';
import { API_BASE } from '../api/config';
import { getTrades } from '../api/trades';
import { SymbolPicker } from '../components/SymbolPicker';
import { Loading } from '../components/Loading';
import { ErrorBanner } from '../components/ErrorBanner';
import { useAppStore } from '../store/app';
import { fmtInt, fmtUsd } from '../utils/format';

const PAGE_SIZES = ['50', '100', '500', '1000'];

export function TradesTapePage() {
  const runId = useAppStore((s) => s.selectedRunId);
  const symbol = useAppStore((s) => s.selectedSymbol);

  const [pageSize, setPageSize] = useState(100);
  const [page, setPage] = useState(1);

  useEffect(() => {
    setPage(1);
  }, [runId, symbol, pageSize]);

  const offset = (page - 1) * pageSize;

  const tradesQuery = useQuery({
    queryKey: ['trades', runId, symbol, pageSize, offset],
    queryFn: () =>
      getTrades({
        runId: runId!,
        symbol: symbol!,
        limit: pageSize,
        offset,
      }),
    enabled: !!runId && !!symbol,
    placeholderData: keepPreviousData,
  });

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

  const total = tradesQuery.data?.total ?? 0;
  const totalPages = Math.max(1, Math.ceil(total / pageSize));

  return (
    <Stack gap="xl">
      <Stack gap={8}>
        <Title order={2}>Trades Tape</Title>
        <Text c="dimmed" size="sm">
          Raw executed trades for the selected symbol, paginated server-side.
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
              <Text className="term-label">Page size</Text>
              <Select
                value={String(pageSize)}
                onChange={(v) => v && setPageSize(Number(v))}
                data={PAGE_SIZES}
                w={100}
                allowDeselect={false}
                styles={{
                  input: { fontFamily: 'JetBrains Mono, ui-monospace, monospace' },
                }}
              />
            </Stack>
            <Stack gap={4}>
              <Text className="term-label">Download</Text>
              <Button
                component="a"
                href={
                  symbol
                    ? `${API_BASE}/runs/${encodeURIComponent(runId)}/files/trades?symbol=${encodeURIComponent(symbol)}`
                    : undefined
                }
                download
                disabled={!symbol}
                variant="default"
                size="sm"
                ff="monospace"
              >
                ↓ {symbol ?? '…'} trades
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
            Pick a symbol to load trades
          </Text>
        </Center>
      ) : tradesQuery.isError ? (
        <ErrorBanner error={tradesQuery.error} title="Failed to load trades" />
      ) : tradesQuery.isLoading ? (
        <Loading label="Loading trades" />
      ) : tradesQuery.data ? (
        <Paper
          p="md"
          withBorder
          radius="xs"
          style={{ background: '#0b0e14', borderColor: 'var(--terminal-border)' }}
        >
          <Stack gap="md">
            <Group justify="space-between" align="flex-end" wrap="wrap" gap="md">
              <Stack gap={2}>
                <Title order={4}>
                  {symbol}{' '}
                  <Text component="span" c="dimmed" size="sm" ff="monospace">
                    {fmtInt(total)} trades
                  </Text>
                </Title>
                <Text size="xs" c="dimmed" ff="monospace">
                  Page {page} of {fmtInt(totalPages)} · rows{' '}
                  {fmtInt(offset + 1)}–{fmtInt(Math.min(offset + pageSize, total))}
                </Text>
              </Stack>
              <Pagination
                value={page}
                onChange={setPage}
                total={totalPages}
                size="sm"
                siblings={1}
                boundaries={1}
                withEdges
              />
            </Group>

            <Table
              withRowBorders
              highlightOnHover
              verticalSpacing="xs"
              horizontalSpacing="md"
              stickyHeader
            >
              <Table.Thead>
                <Table.Tr>
                  <Table.Th w={72}>
                    <Text className="term-label">#</Text>
                  </Table.Th>
                  <Table.Th>
                    <Text className="term-label">Timestamp (ET)</Text>
                  </Table.Th>
                  <Table.Th ta="right">
                    <Text className="term-label">Price</Text>
                  </Table.Th>
                  <Table.Th ta="right">
                    <Text className="term-label">Shares</Text>
                  </Table.Th>
                </Table.Tr>
              </Table.Thead>
              <Table.Tbody>
                {tradesQuery.data.items.map((trade, i) => (
                  <Table.Tr key={`${trade.ts_ns}-${i}`}>
                    <Table.Td>
                      <Text ff="monospace" c="dimmed" size="sm">
                        {fmtInt(offset + i + 1)}
                      </Text>
                    </Table.Td>
                    <Table.Td>
                      <Text ff="monospace" size="sm">
                        {trade.ts}
                      </Text>
                    </Table.Td>
                    <Table.Td ta="right">
                      <Text className="term-value" size="sm">
                        {fmtUsd(trade.price)}
                      </Text>
                    </Table.Td>
                    <Table.Td ta="right">
                      <Text className="term-value" size="sm">
                        {fmtInt(trade.shares)}
                      </Text>
                    </Table.Td>
                  </Table.Tr>
                ))}
              </Table.Tbody>
            </Table>
          </Stack>
        </Paper>
      ) : null}
    </Stack>
  );
}
