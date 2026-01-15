<script lang="ts">
  import { onMount } from 'svelte';
  import { PanelSurface, ui } from '$lib';
  import ConsolePanel from '$lib/demo/ConsolePanel.svelte';
  import HierarchyPanel from '$lib/demo/HierarchyPanel.svelte';
  import InspectorPanel from '$lib/demo/InspectorPanel.svelte';
  import ProjectPanel from '$lib/demo/ProjectPanel.svelte';
  import ScenePanel from '$lib/demo/ScenePanel.svelte';
  import type { MenuItem, PanelRegistration } from '$lib/ui/types';

  const demoPanels: PanelRegistration[] = [
    { id: 'scene', title: 'Scene', component: ScenePanel, minWidth: 280, minHeight: 220 },
    { id: 'hierarchy', title: 'Hierarchy', component: HierarchyPanel, minWidth: 220 },
    { id: 'inspector', title: 'Inspector', component: InspectorPanel, minWidth: 260 },
    { id: 'console', title: 'Console', component: ConsolePanel, minWidth: 320 },
    { id: 'project', title: 'Project', component: ProjectPanel, minWidth: 320 }
  ];

  const windowMenuItems = (): MenuItem[] =>
    demoPanels.map((panel, order) => ({
      id: panel.id,
      label: panel.title,
      order,
      onSelect: () => ui.openPanel(panel.id)
    }));

  onMount(() => {
    demoPanels.forEach((panel) => ui.registerPanel(panel));
    ui.registerMenu({ id: 'file', label: 'File', order: 0, items: [{ id: 'quit', label: 'Quit', enabled: false }] });
    ui.registerMenu({ id: 'edit', label: 'Edit', order: 1, items: [{ id: 'undo', label: 'Undo', enabled: false }] });
    ui.registerMenu({ id: 'window', label: 'Window', order: 2, items: windowMenuItems() });
  });
</script>

<main class="demo">
  <header class="intro">
    <h1>Editor UI Demo</h1>
    <p>Drag panels to dock, stack tabs, or pop them into floating windows.</p>
  </header>
  <section class="stage">
    <PanelSurface />
  </section>
</main>

<style>
  .demo {
    display: flex;
    flex-direction: column;
    gap: 16px;
    height: 100vh;
    padding: 16px;
    box-sizing: border-box;
    background: var(--ui-bg);
    color: var(--ui-text);
    font-family: var(--ui-font);
  }

  .intro {
    background: color-mix(in srgb, var(--ui-surface) 92%, transparent);
    border: 1px solid var(--ui-border);
    border-radius: var(--ui-radius);
    padding: 12px 14px;
    box-shadow: 0 12px 36px rgba(0, 0, 0, 0.22);
  }

  .intro h1 {
    margin: 0 0 6px;
  }

  .intro p {
    margin: 0;
    color: var(--ui-text-muted);
  }

  .stage {
    flex: 1;
    min-height: 0;
  }
</style>
