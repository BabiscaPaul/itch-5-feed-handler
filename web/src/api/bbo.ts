import type { components } from './schema';
import { apiGet, type QueryParams } from './client';

export type BboBucket = components['schemas']['BboBucket'];
export type Resolution = '100ms' | '1s' | '1m' | '5m';

export type GetBboParams = {
  runId: string;
  symbol: string;
  fromTsNs?: number;
  toTsNs?: number;
  resolution?: Resolution;
  regularHours?: boolean;
};

export function getBbo(params: GetBboParams): Promise<BboBucket[]> {
  const { runId, symbol, fromTsNs, toTsNs, resolution, regularHours } = params;
  const query: QueryParams = {
    from_ts_ns: fromTsNs,
    to_ts_ns: toTsNs,
    resolution,
    regular_hours: regularHours,
  };
  return apiGet<BboBucket[]>(
    `/runs/${encodeURIComponent(runId)}/symbols/${encodeURIComponent(symbol)}/bbo`,
    query,
  );
}
