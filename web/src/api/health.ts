import type { components } from './schema';
import { apiGet } from './client';

export type HealthResponse = components['schemas']['HealthResponse'];

export function getHealth(): Promise<HealthResponse> {
  return apiGet<HealthResponse>('/health');
}
