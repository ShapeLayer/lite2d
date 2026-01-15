<script lang="ts">
  import { onMount } from 'svelte';
  import { activeTheme, Button, PanelSurface, ui } from 'lite2d-editor-ui';
  import type { MenuItem, PanelRegistration } from 'lite2d-editor-ui';

  import Moc3Panel from '$lib/panels/Moc3Panel.svelte';


  let themeName: 'dark' | 'light' = 'dark';

  const panels: PanelRegistration[] = [
    { id: 'moc3', title: 'MOC3', component: Moc3Panel, minWidth: 340, minHeight: 320 }
  ];

  const windowMenuItems = (): MenuItem[] =>
    panels.map((panel, order) => ({ id: panel.id, label: panel.title, order, onSelect: () => ui.openPanel(panel.id) }));

  const setupLayout = () => {
    const nextTheme = themeName;

    ui.reset();
    ui.setTheme(nextTheme);
    panels.forEach((panel) => ui.registerPanel(panel));

    ui.registerMenu({ id: 'file', label: 'File', order: 0, items: [{ id: 'save', label: 'Save (stub)', enabled: false }] });
    ui.registerMenu({
      id: 'view',
      label: 'View',
      order: 1,
      items: [
        { id: 'theme-dark', label: 'Dark theme', onSelect: () => ui.setTheme('dark') },
        { id: 'theme-light', label: 'Light theme', onSelect: () => ui.setTheme('light') }
      ]
    });
    ui.registerMenu({ id: 'window', label: 'Window', order: 2, items: windowMenuItems() });

    ui.dockPanel('moc3', 'center');
  };

  onMount(() => {
    setupLayout();
  });

  $: themeName = $activeTheme.name;

  const toggleTheme = () => ui.setTheme(themeName === 'dark' ? 'light' : 'dark');
</script>

<svelte:head>
  <title>Lite2D Editor UI Demo</title>
</svelte:head>

<main class="page">
  <div class="toolbar">
    <Button variant="ghost" label={`Switch to ${themeName === 'dark' ? 'Light' : 'Dark'}`} on:click={toggleTheme} />
    <Button variant="primary" label="Reset layout" on:click={setupLayout} />
  </div>
  <section class="surface-wrapper">
    <PanelSurface />
  </section>
</main>

<style>
  :global(body) {
    margin: 0;
    background: var(--ui-bg);
    color: var(--ui-text);
    font-family: var(--ui-font);
  }

  .page {
    display: flex;
    flex-direction: column;
    gap: 12px;
    min-height: 100vh;
    padding: 14px;
    box-sizing: border-box;
    background: var(--ui-bg);
  }

  .toolbar {
    display: flex;
    gap: 10px;
    justify-content: flex-end;
  }

  .surface-wrapper {
    min-height: 70vh;
    border-radius: var(--ui-radius);
    overflow: hidden;
    border: var(--ui-border-width) solid var(--ui-border);
    box-shadow: 0 18px 40px rgba(0, 0, 0, 0.2);
  }
</style>
