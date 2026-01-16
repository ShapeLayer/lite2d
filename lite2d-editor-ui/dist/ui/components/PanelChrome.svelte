<script lang="ts">
  import { get } from 'svelte/store';
  import { ui } from '../store';
  import { InspectionPanel, PictureInPicture2, X } from '@lucide/svelte';

  type Props = {
    title: string;
    panelId: string;
    isWindow?: boolean;
    showMenu?: boolean;
    onclose?: () => void;
    ontoggle?: () => void;
    onmenu?: () => void;
    ondragstart?: (payload: { event: DragEvent; panelId: string }) => void;
    onpointerdown?: (event: PointerEvent) => void;
  };
  let {
    title,
    panelId,
    isWindow = false,
    showMenu = false,
    onclose,
    ontoggle,
    onmenu,
    ondragstart,
    onpointerdown
  } = $$props;

  const onClose = () => onclose?.();
  const onToggleMode = () => ontoggle?.();
  const onMenu = () => onmenu?.();
</script>

<header
  class="chrome"
  role="presentation"
  draggable="true"
  onpointerdown={(event) => {
    event.stopPropagation();
    onpointerdown?.(event);
  }}
  ondragstart={(event) => {
    ondragstart?.({ event, panelId });
    if (event.dataTransfer) {
      event.dataTransfer.effectAllowed = 'move';
      event.dataTransfer.setData('panel-id', panelId);
      event.dataTransfer.setData('text/plain', panelId);
    }
    ui.setDragging(panelId);
  }}
  ondragend={(event) => {
    // 현재 드래그 중인 패널이 이 패널일 때만 null로 설정
    if (get(ui).draggingPanelId === panelId) {
      ui.setDragging(null);
    }
  }}
>
  <div class="left">
    <div class="handle">::</div>
    <span class="title">{title}</span>
  </div>
  <div class="actions">
    {#if showMenu}
      <button
        class="icon"
        title="Panel menu"
        onclick={(event) => {
          event.stopPropagation();
          onMenu();
        }}
      >
        |||
      </button>
    {/if}
    <button
      class="icon"
      title={isWindow ? 'Dock panel' : 'Pop out panel'}
      onclick={(event) => {
        event.stopPropagation();
        onToggleMode();
      }}
    >
      {#if isWindow}
        <PictureInPicture2 />
      {:else}
        <InspectionPanel />
      {/if}
    </button>
    <button
      class="icon danger"
      title="Close panel"
      onclick={(event) => {
        event.stopPropagation();
        onClose();
      }}
    >
      <X />
    </button>
  </div>
</header>

<style>
  .chrome {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 8px;
    padding: 6px 10px;
    background: linear-gradient(180deg, var(--ui-muted) 0%, var(--ui-surface) 100%);
    border-bottom: var(--ui-border-width) solid var(--ui-border);
    cursor: grab;
    user-select: none;
  }

  .left {
    display: flex;
    align-items: center;
    gap: 8px;
  }

  .handle {
    font-size: 14px;
    color: var(--ui-text-muted);
  }

  .title {
    font-size: 13px;
    color: var(--ui-text);
    font-weight: 600;
  }

  .actions {
    display: flex;
    align-items: center;
    gap: 6px;
  }

  .icon {
    background: var(--ui-muted);
    border: var(--ui-border-width) solid var(--ui-border);
    color: var(--ui-text);
    border-radius: var(--ui-radius);
    padding: 4px 8px;
    font-size: 12px;
    cursor: pointer;
    transition: background 120ms ease, border-color 120ms ease;
  }

  .icon:hover {
    background: var(--ui-highlight);
    border-color: var(--ui-primary);
  }

  .icon.danger {
    color: var(--ui-danger);
  }

  button {
    font-family: inherit;
  }
</style>
