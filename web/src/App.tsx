import { Routes, Route, Navigate } from 'react-router-dom';
import { Layout } from './components/Layout';
import { OverviewPage } from './pages/OverviewPage';
import { BBOExplorerPage } from './pages/BBOExplorerPage';
import { TradesTapePage } from './pages/TradesTapePage';
import { BenchmarksPage } from './pages/BenchmarksPage';

export default function App() {
  return (
    <Routes>
      <Route element={<Layout />}>
        <Route index element={<OverviewPage />} />
        <Route path="bbo" element={<BBOExplorerPage />} />
        <Route path="trades" element={<TradesTapePage />} />
        <Route path="benchmarks" element={<BenchmarksPage />} />
        <Route path="*" element={<Navigate to="/" replace />} />
      </Route>
    </Routes>
  );
}
