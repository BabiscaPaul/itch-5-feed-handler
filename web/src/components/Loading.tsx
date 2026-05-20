import { Center, Loader, Stack, Text } from '@mantine/core';

type Props = {
  height?: number | string;
  label?: string;
};

export function Loading({ height = 200, label }: Props) {
  return (
    <Center h={height}>
      <Stack align="center" gap={8}>
        <Loader size="sm" color="amber" />
        {label && (
          <Text c="dimmed" size="xs" ff="monospace" style={{ letterSpacing: '0.1em', textTransform: 'uppercase' }}>
            {label}
          </Text>
        )}
      </Stack>
    </Center>
  );
}
