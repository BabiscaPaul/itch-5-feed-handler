import { createTheme, type MantineColorsTuple } from '@mantine/core';

const amber: MantineColorsTuple = [
  '#fff7e6',
  '#ffe7bf',
  '#ffd28f',
  '#ffba5c',
  '#ffa333',
  '#ff8c00',
  '#e07700',
  '#b85d00',
  '#8f4800',
  '#663300',
];

const carbon: MantineColorsTuple = [
  '#e8eaed',
  '#c6cbd1',
  '#9ba2ad',
  '#6e7785',
  '#4a525e',
  '#2f3640',
  '#1f242c',
  '#161a20',
  '#0e1116',
  '#070a0e',
];

export const theme = createTheme({
  primaryColor: 'amber',
  primaryShade: { light: 6, dark: 5 },
  defaultRadius: 'xs',
  fontFamily:
    'Inter, -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif',
  fontFamilyMonospace:
    '"JetBrains Mono", ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, monospace',
  headings: {
    fontFamily:
      'Inter, -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif',
    fontWeight: '600',
    sizes: {
      h1: { fontSize: '1.5rem', lineHeight: '1.3' },
      h2: { fontSize: '1.125rem', lineHeight: '1.35' },
      h3: { fontSize: '0.95rem', lineHeight: '1.4' },
      h4: { fontSize: '0.85rem', lineHeight: '1.4' },
    },
  },
  fontSizes: {
    xs: '0.7rem',
    sm: '0.8rem',
    md: '0.875rem',
    lg: '1rem',
    xl: '1.125rem',
  },
  colors: {
    amber,
    carbon,
  },
  components: {
    Title: {
      styles: {
        root: {
          letterSpacing: '-0.01em',
        },
      },
    },
  },
});
