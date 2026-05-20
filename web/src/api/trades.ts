import type { components } from './schema';
import { apiGet, type QueryParams } from './client';

export type Trade = components['schemas']['Trade'];
export type TradesPage = components['schemas']['TradesPage'];

export type GetTradesParams = {
  runId: string;
  symbol: string;
  fromTsNs?: number;
  toTsNs?: number;
  limit?: number;
  offset?: number;
};

export function getTrades(params: GetTradesParams): Promise<TradesPage> {
  const { runId, symbol, fromTsNs, toTsNs, limit, offset } = params;
  const query: QueryParams = {
    from_ts_ns: fromTsNs,
    to_ts_ns: toTsNs,
    limit,
    offset,
  };
  return apiGet<TradesPage>(
    `/runs/${encodeURIComponent(runId)}/symbols/${encodeURIComponent(symbol)}/trades`,
    query,
  );
}
