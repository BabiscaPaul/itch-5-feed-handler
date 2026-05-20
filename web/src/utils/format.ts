const integer = new Intl.NumberFormat('en-US');
const compact = new Intl.NumberFormat('en-US', {
  notation: 'compact',
  maximumFractionDigits: 1,
});

export function fmtInt(n: number): string {
  return integer.format(n);
}

export function fmtCompact(n: number): string {
  return compact.format(n);
}

export function fmtUsdCompact(n: number): string {
  return '$' + compact.format(n);
}

export function fmtDecimal(n: number, digits = 1): string {
  return n.toFixed(digits);
}

export function fmtUsd(n: number, digits = 2): string {
  return '$' + n.toFixed(digits);
}

export function fmtBytes(b: number): string {
  if (b < 1024) return `${b} B`;
  if (b < 1024 ** 2) return `${(b / 1024).toFixed(1)} KB`;
  if (b < 1024 ** 3) return `${(b / 1024 ** 2).toFixed(1)} MB`;
  return `${(b / 1024 ** 3).toFixed(2)} GB`;
}
