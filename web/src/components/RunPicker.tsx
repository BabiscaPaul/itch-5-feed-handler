import { useEffect } from 'react';
import { Select } from '@mantine/core';
import { useQuery } from '@tanstack/react-query';
import { listRuns } from '../api/runs';
import { useAppStore } from '../store/app';

export function RunPicker() {
  const { data: runs, isLoading, isError } = useQuery({
    queryKey: ['runs'],
    queryFn: listRuns,
  });
  const selectedRunId = useAppStore((s) => s.selectedRunId);
  const setRun = useAppStore((s) => s.setRun);

  useEffect(() => {
    if (runs && runs.length > 0 && selectedRunId === null) {
      setRun(runs[0].run_id);
    }
  }, [runs, selectedRunId, setRun]);

  return (
    <Select
      placeholder={isError ? 'No backend' : isLoading ? 'Loading…' : 'Pick a run'}
      data={runs?.map((r) => r.run_id) ?? []}
      value={selectedRunId}
      onChange={setRun}
      disabled={isLoading || isError || !runs?.length}
      allowDeselect={false}
      size="xs"
      w={160}
      styles={{ input: { fontFamily: 'JetBrains Mono, ui-monospace, monospace' } }}
    />
  );
}
