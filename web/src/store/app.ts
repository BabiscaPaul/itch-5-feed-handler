import { create } from 'zustand';

type AppState = {
  selectedRunId: string | null;
  selectedSymbol: string | null;
  setRun: (id: string | null) => void;
  setSymbol: (s: string | null) => void;
};

export const useAppStore = create<AppState>((set) => ({
  selectedRunId: null,
  selectedSymbol: null,
  setRun: (id) => set({ selectedRunId: id, selectedSymbol: null }),
  setSymbol: (s) => set({ selectedSymbol: s }),
}));
