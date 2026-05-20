import { Alert, List, Text } from '@mantine/core';
import { ApiError } from '../api/client';

type Props = {
  error: unknown;
  title?: string;
};

type FieldError = {
  loc?: (string | number)[];
  msg?: string;
};

function isFieldErrorArray(detail: unknown): detail is FieldError[] {
  return Array.isArray(detail) && detail.every((d) => typeof d === 'object' && d !== null);
}

export function ErrorBanner({ error, title = 'Request failed' }: Props) {
  if (error instanceof ApiError) {
    const { status, statusText, detail } = error;
    const heading = `${title} — HTTP ${status} ${statusText}`;

    if (typeof detail === 'string') {
      return (
        <Alert color="red" variant="light" radius="xs" title={heading}>
          <Text size="sm" ff="monospace">{detail}</Text>
        </Alert>
      );
    }

    if (isFieldErrorArray(detail)) {
      return (
        <Alert color="red" variant="light" radius="xs" title={heading}>
          <List size="sm" spacing={4}>
            {detail.map((d, i) => (
              <List.Item key={i}>
                <Text size="sm" ff="monospace">
                  <Text component="span" c="dimmed">{(d.loc ?? []).join('.')}: </Text>
                  {d.msg ?? 'invalid'}
                </Text>
              </List.Item>
            ))}
          </List>
        </Alert>
      );
    }

    return (
      <Alert color="red" variant="light" radius="xs" title={heading}>
        <Text size="sm" ff="monospace">{JSON.stringify(detail)}</Text>
      </Alert>
    );
  }

  const message = error instanceof Error ? error.message : String(error);
  return (
    <Alert color="red" variant="light" radius="xs" title={title}>
      <Text size="sm" ff="monospace">{message}</Text>
    </Alert>
  );
}
