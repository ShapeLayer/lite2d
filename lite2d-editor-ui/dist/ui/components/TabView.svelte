<script lang="ts">
  import { ui } from '../store';
  import type { TabsNode } from '../types';
  import { get } from 'svelte/store';
  import PanelChrome from './PanelChrome.svelte';

  type Props = { node: TabsNode };

  let { node } = $props();
  let registry = $state(get(ui).registry);
  const panelEntry = $derived(registry[node.activeTabId]);
  const Active = $derived(panelEntry?.component);

  $effect(() => {
    const unsub = ui.subscribe((value) => {
      registry = value.registry;
    });
    return () => unsub();
  });

  const onTabClick = (id: string) => ui.setActiveTabId(node.id, id);
  const onClose = (id: string) => ui.closePanel(id);
  const onToggleWindow = (id: string) => ui.toggleWindowMode(id);
  
  const allowDrop = (event: DragEvent) => {
    event.preventDefault();
  };

  const handleDrop = (event: DragEvent) => {
    event.preventDefault();
    const panelId = event.dataTransfer?.getData('panel-id');
    if (panelId) {
      ui.attachPanelToTabs(panelId, node.id);
      ui.setDragging(null);
    }
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
</script>

<div class="tabs" role="presentation" ondragover={allowDrop} ondrop={handleDrop}>
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
        on:close={() => onClose(node.activeTabId)}
        on:toggle={() => onToggleWindow(node.activeTabId)}
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
