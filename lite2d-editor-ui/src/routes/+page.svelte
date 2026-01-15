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

<main>
  <header class="hero">
    <div>
      <p class="eyebrow">Lite2D Editor UI</p>
      <h1>Dockable panel system inspired by Unity and Photoshop</h1>
      <p class="lede">
        Drag panels into tab bars or onto screen edges to dock. Pop panels out as floating windows, or reopen them from
        the Window menu.
      </p>
    </div>
  </header>
  <section class="canvas">
    <PanelSurface />
  </section>
</main>

<style>
  main {
    display: flex;
    flex-direction: column;
    gap: 18px;
    height: 100vh;
    background: radial-gradient(circle at 20% 20%, color-mix(in srgb, var(--ui-primary) 12%, transparent), transparent),
      radial-gradient(circle at 80% 30%, color-mix(in srgb, var(--ui-highlight) 10%, transparent), transparent),
      var(--ui-bg);
    color: var(--ui-text);
    padding: 18px 18px 0;
    box-sizing: border-box;
  }

  .hero {
    max-width: 900px;
    padding: 16px 18px;
    background: color-mix(in srgb, var(--ui-surface) 92%, transparent);
    border: 1px solid var(--ui-border);
    border-radius: var(--ui-radius);
    box-shadow: 0 18px 60px rgba(0, 0, 0, 0.25);
  }

  .eyebrow {
    color: var(--ui-primary);
    text-transform: uppercase;
    letter-spacing: 0.1em;
    font-weight: 700;
    font-size: 12px;
  }

  h1 {
    margin: 6px 0;
    font-size: 28px;
    line-height: 1.2;
  }

  .lede {
    color: var(--ui-text-muted);
    max-width: 720px;
  }

  .canvas {
    flex: 1;
    min-height: 0;
  }

  :global(body) {
    margin: 0;
    background: var(--ui-bg);
  }
</style>
