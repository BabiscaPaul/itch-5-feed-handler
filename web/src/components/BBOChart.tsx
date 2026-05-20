import { useEffect, useRef } from 'react';
import {
  ColorType,
  CrosshairMode,
  LineSeries,
  createChart,
  type IChartApi,
  type ISeriesApi,
  type LineData,
} from 'lightweight-charts';

type Props = {
  bid: LineData[];
  ask: LineData[];
  height?: number;
};

const COLORS = {
  bg: '#0b0e14',
  text: '#8a93a3',
  grid: '#11151c',
  border: '#1c2230',
  bid: '#5cc8ff',
  ask: '#ff8c00',
};

export function BBOChart({ bid, ask, height = 380 }: Props) {
  const containerRef = useRef<HTMLDivElement | null>(null);
  const chartRef = useRef<IChartApi | null>(null);
  const bidRef = useRef<ISeriesApi<'Line'> | null>(null);
  const askRef = useRef<ISeriesApi<'Line'> | null>(null);

  useEffect(() => {
    if (!containerRef.current) return;

    const chart = createChart(containerRef.current, {
      width: containerRef.current.clientWidth,
      height,
      layout: {
        background: { type: ColorType.Solid, color: COLORS.bg },
        textColor: COLORS.text,
        fontFamily: 'JetBrains Mono, ui-monospace, monospace',
        fontSize: 11,
      },
      grid: {
        vertLines: { color: COLORS.grid },
        horzLines: { color: COLORS.grid },
      },
      crosshair: {
        mode: CrosshairMode.Normal,
        vertLine: { color: COLORS.border, width: 1, style: 3 },
        horzLine: { color: COLORS.border, width: 1, style: 3 },
      },
      rightPriceScale: { borderColor: COLORS.border },
      timeScale: {
        borderColor: COLORS.border,
        timeVisible: true,
        secondsVisible: false,
      },
    });

    bidRef.current = chart.addSeries(LineSeries, {
      color: COLORS.bid,
      lineWidth: 1,
      priceLineVisible: false,
      lastValueVisible: true,
      title: 'Bid',
    });

    askRef.current = chart.addSeries(LineSeries, {
      color: COLORS.ask,
      lineWidth: 1,
      priceLineVisible: false,
      lastValueVisible: true,
      title: 'Ask',
    });

    chartRef.current = chart;

    const ro = new ResizeObserver((entries) => {
      const entry = entries[0];
      if (entry) {
        chart.applyOptions({ width: entry.contentRect.width });
      }
    });
    ro.observe(containerRef.current);

    return () => {
      ro.disconnect();
      chart.remove();
      chartRef.current = null;
      bidRef.current = null;
      askRef.current = null;
    };
  }, [height]);

  useEffect(() => {
    if (!bidRef.current || !askRef.current || !chartRef.current) return;

    bidRef.current.setData(bid);
    askRef.current.setData(ask);

    chartRef.current.timeScale().fitContent();
  }, [bid, ask]);

  return <div ref={containerRef} style={{ width: '100%', height }} />;
}
