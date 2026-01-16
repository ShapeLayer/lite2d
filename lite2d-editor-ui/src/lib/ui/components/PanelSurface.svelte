<script lang="ts">
  import { layoutTree, ui, windowedPanels } from "../store";
  import type { LayoutNode, PanelWindow } from "../types";
  import FloatingWindow from "./FloatingWindow.svelte";
  import MenuBar from "./MenuBar.svelte";
  import PanelNode from "./PanelNode.svelte";
  import { get } from "svelte/store";

  let layout: LayoutNode | null = $state(null);
  let windows: PanelWindow[] = $state([]);
  let uiState = $state(get(ui));

  // store 구독
  $effect(() => {
    const unsubLayout = layoutTree.subscribe((value) => (layout = value));
    const unsubWindows = windowedPanels.subscribe((value) => (windows = value));
    const unsubUi = ui.subscribe((value) => (uiState = value));
    return () => {
      unsubLayout();
      unsubWindows();
      unsubUi();
    };
  });

  const allowDrop = (event: DragEvent) => {
    event.preventDefault();
  };
</script>

<section class="surface" role="application">
  <MenuBar />
  <div class="workspace">
    {#if layout}
      <PanelNode node={layout} />
    {:else}
      <div class="empty" role="region" ondragover={allowDrop}>Register a panel to get started.</div>
    {/if}
  </div>

  {#each windows as win (win.panelId)}
    <FloatingWindow {...win} />
  {/each}
</section>

<style>
  .surface {
    position: relative;
    height: 100%;
    display: flex;
    flex-direction: column;
    background: var(--ui-bg);
    color: var(--ui-text);
    font-family: var(--ui-font);
  }

  .workspace {
    flex: 1;
    position: relative;
    padding: 12px;
  }

  .workspace > :global(*) {
    height: 100%;
  }

  .empty {
    height: 100%;
    display: grid;
    place-items: center;
    color: var(--ui-text-muted);
    border: 1px dashed var(--ui-border);
    border-radius: var(--ui-radius);
  }

</style>
