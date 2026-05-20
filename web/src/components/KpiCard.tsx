import { Paper, Stack, Text } from '@mantine/core';

type KpiCardProps = {
  label: string;
  value: string;
  unit?: string;
};

export function KpiCard({ label, value, unit }: KpiCardProps) {
  return (
    <Paper
      p="md"
      withBorder
      radius="xs"
      style={{
        background: '#0b0e14',
        borderColor: 'var(--terminal-border)',
      }}
    >
      <Stack gap={6}>
        <Text className="term-label">{label}</Text>
        <Text
          ff="monospace"
          fw={600}
          size="xl"
          style={{
            fontVariantNumeric: 'tabular-nums',
            letterSpacing: '-0.01em',
          }}
        >
          {value}
        </Text>
        {unit && (
          <Text size="xs" c="dimmed" ff="monospace">
            {unit}
          </Text>
        )}
      </Stack>
    </Paper>
  );
}
