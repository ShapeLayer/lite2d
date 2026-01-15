<script lang="ts">
  import { layoutTree, ui, windowedPanels } from "../store";
  import type { DockZone, LayoutNode, PanelWindow } from "../types";
  import FloatingWindow from "./FloatingWindow.svelte";
  import MenuBar from "./MenuBar.svelte";
  import PanelNode from "./PanelNode.svelte";
  import { get } from "svelte/store";

  let layout: LayoutNode | null = $state(null);
  let windows: PanelWindow[] = $state([]);
  let uiState = $state(get(ui));

  const dragging = $derived(uiState.draggingPanelId);
  
  // 드래그 시작 위치와 drop zone 표시 상태
  let dragStartPos = $state<{ x: number; y: number } | null>(null);
  let showDropZones = $state(false);

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

  // 마우스 이동 감지로 drop zone 활성화
  $effect(() => {
    if (!dragging) {
      showDropZones = false;
      dragStartPos = null;
      return;
    }

    const handleDragOver = (e: DragEvent) => {
      if (!dragStartPos) {
        dragStartPos = { x: e.clientX, y: e.clientY };
        return;
      }

      const distance = Math.hypot(e.clientX - dragStartPos.x, e.clientY - dragStartPos.y);
      if (distance > 20) {
        showDropZones = true;
      }
    };

    document.addEventListener('dragover', handleDragOver);
    return () => document.removeEventListener('dragover', handleDragOver);
  });

  const dropHandler = (zone: DockZone) => (event: DragEvent) => {
    event.preventDefault();
    event.stopPropagation();
    const panelId = event.dataTransfer?.getData("panel-id");
    if (panelId) {
      ui.dockPanel(panelId, zone);
      ui.setDragging(null);
    }
  };

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
      <div class="empty">Register a panel to get started.</div>
    {/if}
  </div>

  <!-- showDropZones 사용 -->
  <div class="drop-overlay" class:active={showDropZones} ondragover={allowDrop}>
    <div
      class="zone left"
      role="presentation"
      ondragover={allowDrop}
      ondrop={dropHandler("left")}
    >
      <span>Dock Left</span>
    </div>
    <div
      class="zone right"
      role="presentation"
      ondragover={allowDrop}
      ondrop={dropHandler("right")}
    >
      <span>Dock Right</span>
    </div>
    <div
      class="zone top"
      role="presentation"
      ondragover={allowDrop}
      ondrop={dropHandler("top")}
    >
      <span>Dock Top</span>
    </div>
    <div
      class="zone bottom"
      role="presentation"
      ondragover={allowDrop}
      ondrop={dropHandler("bottom")}
    >
      <span>Dock Bottom</span>
    </div>
    <div
      class="zone center"
      role="presentation"
      ondragover={allowDrop}
      ondrop={dropHandler("center")}
    >
      <span>Stack Here</span>
    </div>
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

  .drop-overlay {
    position: absolute;
    top: 48px;
    left: 0;
    right: 0;
    bottom: 0;
    pointer-events: none;
    opacity: 0;
    visibility: hidden;
    transition: opacity 100ms ease;
  }

  .drop-overlay.active {
    opacity: 1;
    visibility: visible;
  }

  .drop-overlay.active .zone {
    pointer-events: auto;
  }

  .zone {
    position: absolute;
    border: 2px dashed var(--ui-primary);
    background: color-mix(in srgb, var(--ui-primary) 12%, transparent);
    color: var(--ui-text);
    font-weight: 700;
    display: grid;
    place-items: center;
    border-radius: var(--ui-radius);
  }

  .zone.left {
    top: 8%;
    bottom: 8%;
    left: 10px;
    width: 26%;
  }

  .zone.right {
    top: 8%;
    bottom: 8%;
    right: 10px;
    width: 26%;
  }

  .zone.top {
    left: 14%;
    right: 14%;
    top: 10px;
    height: 22%;
  }

  .zone.bottom {
    left: 14%;
    right: 14%;
    bottom: 10px;
    height: 22%;
  }

  .zone.center {
    left: 30%;
    right: 30%;
    top: 30%;
    bottom: 30%;
    border-style: solid;
  }

  .zone span {
    background: var(--ui-surface);
    padding: 6px 10px;
    border-radius: var(--ui-radius);
    box-shadow: 0 4px 10px rgba(0, 0, 0, 0.2);
    border: 1px solid var(--ui-border);
  }
</style>
