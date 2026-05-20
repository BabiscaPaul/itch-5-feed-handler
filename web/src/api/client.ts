import { API_BASE } from './config';

export class ApiError extends Error {
  readonly status: number;
  readonly statusText: string;
  readonly detail: unknown;

  constructor(status: number, statusText: string, detail: unknown) {
    super(`HTTP ${status} ${statusText}`);
    this.name = 'ApiError';
    this.status = status;
    this.statusText = statusText;
    this.detail = detail;
  }
}

type QueryValue = string | number | boolean | undefined | null;
export type QueryParams = Record<string, QueryValue>;

function buildUrl(path: string, params?: QueryParams): string {
  const url = new URL(API_BASE.replace(/\/$/, '') + path);
  if (params) {
    for (const [key, value] of Object.entries(params)) {
      if (value !== undefined && value !== null) {
        url.searchParams.set(key, String(value));
      }
    }
  }
  return url.toString();
}

export async function apiGet<T>(path: string, params?: QueryParams): Promise<T> {
  const response = await fetch(buildUrl(path, params), {
    method: 'GET',
    headers: { Accept: 'application/json' },
  });

  const text = await response.text();
  let body: unknown = null;
  if (text) {
    try {
      body = JSON.parse(text);
    } catch {
      body = text;
    }
  }

  if (!response.ok) {
    const detail =
      body && typeof body === 'object' && 'detail' in body
        ? (body as { detail: unknown }).detail
        : body;
    throw new ApiError(response.status, response.statusText, detail);
  }

  return body as T;
}
