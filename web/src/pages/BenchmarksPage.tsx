import { Center, Stack, Text } from '@mantine/core';

export function BenchmarksPage() {
  return (
    <Center h={400}>
      <Stack align="center" gap="xs">
        <Text ff="monospace" size="sm" style={{ letterSpacing: '0.12em', textTransform: 'uppercase' }}>
          Benchmarks
        </Text>
        <Text c="dimmed" ff="monospace" size="xs">
          Coming soon — awaiting engine meta.json output.
        </Text>
      </Stack>
    </Center>
  );
}
