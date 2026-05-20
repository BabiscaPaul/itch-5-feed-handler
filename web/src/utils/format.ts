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
