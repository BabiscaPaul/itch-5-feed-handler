import { AppShell, Burger, Group, NavLink, Stack, Text } from '@mantine/core';
import { useDisclosure } from '@mantine/hooks';
import { NavLink as RouterNavLink, Outlet, useLocation } from 'react-router-dom';

const navItems = [
  { to: '/', label: 'Overview', end: true },
  { to: '/bbo', label: 'BBO Explorer' },
  { to: '/trades', label: 'Trades Tape' },
  { to: '/benchmarks', label: 'Benchmarks' },
];

export function Layout() {
  const [opened, { toggle, close }] = useDisclosure();
  const location = useLocation();

  const isActive = (to: string, end?: boolean) =>
    end ? location.pathname === to : location.pathname.startsWith(to);

  return (
    <AppShell
      header={{ height: 48 }}
      navbar={{
        width: 220,
        breakpoint: 'sm',
        collapsed: { mobile: !opened },
      }}
      padding="lg"
    >
      <AppShell.Header>
        <Group h="100%" px="md" gap="md" wrap="nowrap">
          <Burger opened={opened} onClick={toggle} hiddenFrom="sm" size="sm" />
          <Group gap={8} wrap="nowrap">
            <Text
              ff="monospace"
              fw={700}
              size="sm"
              style={{ letterSpacing: '0.12em' }}
              className="term-glow"
            >
              ITCH
            </Text>
            <Text
              ff="monospace"
              size="xs"
              c="dimmed"
              style={{ letterSpacing: '0.12em', textTransform: 'uppercase' }}
            >
              Replay Explorer
            </Text>
          </Group>
        </Group>
      </AppShell.Header>

      <AppShell.Navbar p="xs">
        <Stack gap={2}>
          {navItems.map((item) => (
            <NavLink
              key={item.to}
              component={RouterNavLink}
              to={item.to}
              end={item.end}
              label={item.label}
              active={isActive(item.to, item.end)}
              onClick={close}
            />
          ))}
        </Stack>
      </AppShell.Navbar>

      <AppShell.Main>
        <Outlet />
      </AppShell.Main>
    </AppShell>
  );
}
