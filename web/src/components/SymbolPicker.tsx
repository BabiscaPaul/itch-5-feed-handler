import { Select } from '@mantine/core';
import { useQuery } from '@tanstack/react-query';
import { listSymbols } from '../api/runs';
import { useAppStore } from '../store/app';

export function SymbolPicker() {
  const selectedRunId = useAppStore((s) => s.selectedRunId);
  const selectedSymbol = useAppStore((s) => s.selectedSymbol);
  const setSymbol = useAppStore((s) => s.setSymbol);

  const { data: symbols, isLoading, isError } = useQuery({
    queryKey: ['symbols', selectedRunId],
    queryFn: () => listSymbols(selectedRunId!),
    enabled: !!selectedRunId,
  });

  return (
    <Select
      label="Symbol"
      placeholder={
        !selectedRunId ? 'Pick a run first' : isError ? 'Error loading symbols' : isLoading ? 'Loading…' : 'Pick a symbol'
      }
      data={symbols?.map((s) => s.symbol) ?? []}
      value={selectedSymbol}
      onChange={setSymbol}
      disabled={!selectedRunId || isLoading || isError}
      searchable
      allowDeselect={false}
      w={180}
      styles={{ input: { fontFamily: 'JetBrains Mono, ui-monospace, monospace' } }}
    />
  );
}
