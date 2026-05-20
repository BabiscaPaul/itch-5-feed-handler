import type { components } from './schema';
import { apiGet } from './client';

export type RunSummary = components['schemas']['RunSummary'];
export type RunDetail = components['schemas']['RunDetail'];
export type SymbolStat = components['schemas']['SymbolStat'];

export function listRuns(): Promise<RunSummary[]> {
  return apiGet<RunSummary[]>('/runs');
}

export function getRun(runId: string): Promise<RunDetail> {
  return apiGet<RunDetail>(`/runs/${encodeURIComponent(runId)}`);
}

export function listSymbols(runId: string): Promise<SymbolStat[]> {
  return apiGet<SymbolStat[]>(`/runs/${encodeURIComponent(runId)}/symbols`);
}
