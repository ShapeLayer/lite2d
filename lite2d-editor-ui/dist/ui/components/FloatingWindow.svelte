<script lang="ts">
  import { get } from 'svelte/store';
  import { ui } from '../store';
  import PanelChrome from './PanelChrome.svelte';

  type Props = {
    panelId: string;
    x: number;
    y: number;
    width: number;
    height: number;
    z: number;
  };

  let { panelId, x, y, width, height, z } = $props();
  let registry = $state(get(ui).registry);
  let dragging = $state(false);
  let resizing = $state(false);
  const panelEntry = $derived(registry[panelId]);
  const Active = $derived(panelEntry?.component);

  $effect(() => {
    const unsub = ui.subscribe((value) => {
      registry = value.registry;
    });
    return () => unsub();
  });

  const startMove = (event?: PointerEvent | DragEvent) => {
    if (!event) return;
    event.preventDefault();
    dragging = true;
    ui.focusWindow(panelId);
    let lastX = event.clientX;
    let lastY = event.clientY;
    const onMove = (moveEvent: PointerEvent) => {
      const dx = moveEvent.clientX - lastX;
      const dy = moveEvent.clientY - lastY;
      lastX = moveEvent.clientX;
      lastY = moveEvent.clientY;
      ui.moveWindow(panelId, dx, dy);
    };
    const onUp = () => {
      dragging = false;
      window.removeEventListener('pointermove', onMove);
      window.removeEventListener('pointerup', onUp);
    };
    window.addEventListener('pointermove', onMove);
    window.addEventListener('pointerup', onUp);
  };

  const startResize = (event: PointerEvent) => {
    event.preventDefault();
    resizing = true;
    ui.focusWindow(panelId);
    let lastX = event.clientX;
    let lastY = event.clientY;
    const minWidth = panelEntry?.minWidth ?? 240;
    const minHeight = panelEntry?.minHeight ?? 160;
    const onMove = (moveEvent: PointerEvent) => {
      const dx = moveEvent.clientX - lastX;
      const dy = moveEvent.clientY - lastY;
      lastX = moveEvent.clientX;
      lastY = moveEvent.clientY;
      ui.resizeWindow(panelId, dx, dy, minWidth, minHeight);
    };
    const onUp = () => {
      resizing = false;
      window.removeEventListener('pointermove', onMove);
      window.removeEventListener('pointerup', onUp);
    };
    window.addEventListener('pointermove', onMove);
    window.addEventListener('pointerup', onUp);
  };

  const onClose = () => ui.closePanel(panelId);
  const onToggleMode = () => ui.toggleWindowMode(panelId);
</script>

{#if panelEntry}
  <div
    class={`floating ${dragging ? 'dragging' : ''} ${resizing ? 'resizing' : ''}`}
    style={`left:${x}px;top:${y}px;width:${width}px;height:${height}px;z-index:${z};`}
    onmousedown={() => ui.focusWindow(panelId)}
    role="presentation"
  >
    <PanelChrome
      title={panelEntry.title}
      panelId={panelId}
      isWindow={true}
      onclose={onClose}
      ontoggle={onToggleMode}
      onpointerdown={(event) => startMove(event)}
      ondragstart={() => { /* dragstart sets dataTransfer in PanelChrome; no move needed here */ }}
    />
    <div class="content">
      {#if Active}
        <Active panelId={panelId} />
      {/if}
    </div>
    <div class="resize" onpointerdown={startResize}></div>
  </div>
{/if}

<style>
  .floating {
    position: absolute;
    background: var(--ui-surface);
    border: var(--ui-border-width) solid var(--ui-border);
    border-radius: var(--ui-radius);
    box-shadow: 0 20px 60px rgba(0, 0, 0, 0.35);
    overflow: hidden;
    pointer-events: auto;
  }

  .content {
    height: calc(100% - 46px);
    padding: var(--ui-spacing);
    overflow: auto;
  }

  .resize {
    position: absolute;
    width: 14px;
    height: 14px;
    bottom: 0;
    right: 0;
    cursor: nwse-resize;
    background: linear-gradient(135deg, transparent 50%, var(--ui-border) 50%);
  }

  .dragging {
    opacity: 0.95;
  }

  .resizing {
    opacity: 0.95;
  }
</style>
