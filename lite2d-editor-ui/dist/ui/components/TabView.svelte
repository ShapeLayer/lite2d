<script lang="ts">
  import { ui } from '../store';
  import type { TabsNode } from '../types';
  import { get } from 'svelte/store';
  import PanelChrome from './PanelChrome.svelte';
  import type { DockZone } from '../types';

  type Props = { node: TabsNode };

  let { node } = $props();
  let registry = $state(get(ui).registry);
  let draggingPanelId = $state<string | null>(get(ui).draggingPanelId);
  let hoverZone = $state<DockZone | null>(null);
  let container: HTMLDivElement | null = null;
  const panelEntry = $derived(registry[node.activeTabId]);
  const Active = $derived(panelEntry?.component);

  $effect(() => {
    const unsub = ui.subscribe((value) => {
      registry = value.registry;
      draggingPanelId = value.draggingPanelId;
    });
    return () => unsub();
  });

  const onTabClick = (id: string) => ui.setActiveTabId(node.id, id);
  const onClose = (id: string) => ui.closePanel(id);
  const onToggleWindow = (id: string) => ui.toggleWindowMode(id);
  
  const resolveZone = (event: DragEvent): DockZone => {
    if (!container) return 'center';
    const rect = container.getBoundingClientRect();
    const x = event.clientX - rect.left;
    const y = event.clientY - rect.top;
    const edge = Math.max(24, Math.min(80, Math.min(rect.width, rect.height) * 0.22));
    if (x <= edge) return 'left';
    if (x >= rect.width - edge) return 'right';
    if (y <= edge) return 'top';
    if (y >= rect.height - edge) return 'bottom';
    return 'center';
  };

  const handleDragOver = (event: DragEvent) => {
    event.preventDefault();
    hoverZone = resolveZone(event);
  };

  const handleDragLeave = (event: DragEvent) => {
    if (!container) return;
    const rect = container.getBoundingClientRect();
    const x = event.clientX - rect.left;
    const y = event.clientY - rect.top;
    if (x < 0 || y < 0 || x > rect.width || y > rect.height) {
      hoverZone = null;
    }
  };

  const handleDrop = (event: DragEvent) => {
    event.preventDefault();
    const panelId = event.dataTransfer?.getData('panel-id');
    const zone = hoverZone ?? 'center';
    hoverZone = null;
    if (!panelId) return;
    ui.dockPanelToTabs(panelId, node.id, zone);
    ui.setDragging(null);
  };

  // 탭 드래그 시작 핸들러 분리
  const handleTabDragStart = (event: DragEvent, tabId: string) => {
    if (!event.dataTransfer) return;
    event.dataTransfer.effectAllowed = 'move';
    event.dataTransfer.setData('panel-id', tabId);
    ui.setDragging(tabId);


    // 동기적으로 확인
    console.log('setDragging called with:', tabId);
    console.log('ui state after:', get(ui).draggingPanelId);
  };

  $effect(() => {
    if (!draggingPanelId) {
      hoverZone = null;
    }
  });
</script>

<div
  class={`tabs ${draggingPanelId ? 'drag-active' : ''}`}
  role="presentation"
  bind:this={container}
  ondragover={handleDragOver}
  ondragleave={handleDragLeave}
  ondrop={handleDrop}
>
  <div class="tab-bar">
    {#each node.tabs as tab (tab)}
      <button
        draggable="true"
        ondragstart={(event) => {
          event.stopPropagation();
          handleTabDragStart(event, tab)
        }}
        ondragend={(event) => {
          event.stopPropagation();
          console.log('dragend fired for tab:', tab, 'dropEffect:', event.dataTransfer?.dropEffect);
          ui.setDragging(null);
        }}
        class:active={tab === node.activeTabId}
        onclick={() => onTabClick(tab)}
      >
        <span>{registry[tab]?.title ?? tab}</span>
      </button>
    {/each}
  </div>

  {#if node.activeTabId && registry[node.activeTabId]}
    <div class="panel-body">
      <PanelChrome
        title={registry[node.activeTabId].title}
        panelId={node.activeTabId}
        onclose={() => onClose(node.activeTabId)}
        ontoggle={() => onToggleWindow(node.activeTabId)}
      />  
      <div class="content">
        {#if Active}
          <Active panelId={node.activeTabId} />
        {/if}
      </div>
    </div>
  {:else}
    <div class="empty">No panel selected</div>
  {/if}

  <div class="dock-hints" class:active={!!draggingPanelId}>
    <div class={`dock-zone left ${hoverZone === 'left' ? 'hot' : ''}`}></div>
    <div class={`dock-zone right ${hoverZone === 'right' ? 'hot' : ''}`}></div>
    <div class={`dock-zone top ${hoverZone === 'top' ? 'hot' : ''}`}></div>
    <div class={`dock-zone bottom ${hoverZone === 'bottom' ? 'hot' : ''}`}></div>
    <div class={`dock-zone center ${hoverZone === 'center' ? 'hot' : ''}`}></div>
  </div>
</div>

<style>
  .tabs {
    display: flex;
    flex-direction: column;
    width: 100%;
    height: 100%;
    background: var(--ui-surface);
    border: var(--ui-border-width) solid var(--ui-border);
    border-radius: calc(var(--ui-radius) * 0.75);
    overflow: hidden;
    position: relative;
  }

  .tabs.drag-active {
    box-shadow: 0 0 0 2px color-mix(in srgb, var(--ui-primary) 85%, transparent);
    border-color: color-mix(in srgb, var(--ui-primary) 85%, var(--ui-border));
    outline: 2px solid color-mix(in srgb, var(--ui-primary) 60%, transparent);
    outline-offset: -2px;
  }

  .dock-hints {
    position: absolute;
    inset: 0;
    pointer-events: none;
    opacity: 0;
    transition: opacity 120ms ease;
  }

  .dock-hints.active {
    opacity: 1;
  }

  .dock-zone {
    position: absolute;
    border-radius: calc(var(--ui-radius) * 0.8);
    opacity: 0.55;
    transition: opacity 120ms ease, box-shadow 120ms ease, background 120ms ease;
  }

  .dock-zone.hot {
    opacity: 1;
    box-shadow: inset 0 0 0 2px color-mix(in srgb, var(--ui-primary) 90%, transparent);
    background: color-mix(in srgb, var(--ui-primary) 18%, transparent);
  }

  .dock-zone.left {
    top: 8px;
    bottom: 8px;
    left: 8px;
    width: 16px;
    background: linear-gradient(to right, color-mix(in srgb, var(--ui-primary) 60%, transparent), transparent 80%);
    box-shadow: inset 2px 0 0 color-mix(in srgb, var(--ui-primary) 75%, transparent);
  }

  .dock-zone.right {
    top: 8px;
    bottom: 8px;
    right: 8px;
    width: 16px;
    background: linear-gradient(to left, color-mix(in srgb, var(--ui-primary) 60%, transparent), transparent 80%);
    box-shadow: inset -2px 0 0 color-mix(in srgb, var(--ui-primary) 75%, transparent);
  }

  .dock-zone.top {
    left: 8px;
    right: 8px;
    top: 8px;
    height: 16px;
    background: linear-gradient(to bottom, color-mix(in srgb, var(--ui-primary) 60%, transparent), transparent 80%);
    box-shadow: inset 0 2px 0 color-mix(in srgb, var(--ui-primary) 75%, transparent);
  }

  .dock-zone.bottom {
    left: 8px;
    right: 8px;
    bottom: 8px;
    height: 16px;
    background: linear-gradient(to top, color-mix(in srgb, var(--ui-primary) 60%, transparent), transparent 80%);
    box-shadow: inset 0 -2px 0 color-mix(in srgb, var(--ui-primary) 75%, transparent);
  }

  .dock-zone.center {
    left: 22%;
    right: 22%;
    top: 22%;
    bottom: 22%;
    background: transparent;
    box-shadow: inset 0 0 0 2px color-mix(in srgb, var(--ui-primary) 55%, transparent);
  }

  .tab-bar {
    display: flex;
    align-items: center;
    gap: 6px;
    padding: 6px 8px;
    background: var(--ui-muted);
    border-bottom: var(--ui-border-width) solid var(--ui-border);
  }

  .tab-bar button {
    background: transparent;
    border: var(--ui-border-width) solid transparent;
    color: var(--ui-text-muted);
    padding: 6px 10px;
    border-radius: var(--ui-radius);
    cursor: pointer;
    font-weight: 600;
    transition: background 120ms ease, color 120ms ease, border-color 120ms ease;
  }

  .tab-bar button.active {
    background: var(--ui-surface);
    color: var(--ui-text);
    border-color: var(--ui-border);
  }

  .tab-bar button:hover {
    color: var(--ui-text);
    background: var(--ui-highlight);
  }

  .panel-body {
    display: flex;
    flex-direction: column;
    height: 100%;
  }

  .content {
    flex: 1;
    overflow: auto;
    padding: var(--ui-spacing);
  }

  .empty {
    flex: 1;
    display: grid;
    place-items: center;
    color: var(--ui-text-muted);
    font-size: 13px;
  }
</style>
