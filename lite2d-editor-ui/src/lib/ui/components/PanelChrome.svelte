
<script lang="ts">
  // createEventDispatcher is still the supported API for component events;
  // lints may warn in runes mode, so we keep it and silence the false-positive.
  // eslint-disable-next-line svelte/deprecations
  import { createEventDispatcher } from 'svelte';
  import { ui } from '../store';

  type $$Events = {
    close: CustomEvent<void>;
    toggle: CustomEvent<void>;
    menu: CustomEvent<void>;
    dragstart: CustomEvent<{ event: DragEvent; panelId: string }>;
    pointerdown: CustomEvent<PointerEvent>;
  };

  type Props = {
    title: string;
    panelId: string;
    isWindow?: boolean;
    showMenu?: boolean;
  };

  const dispatch = createEventDispatcher<{
    close: void;
    toggle: void;
    menu: void;
    dragstart: { event: DragEvent; panelId: string };
    pointerdown: PointerEvent;
  }>();
  let { title, panelId, isWindow = false, showMenu = false } = $props();

  const onClose = () => dispatch('close');
  const onToggleMode = () => dispatch('toggle');
  const onMenu = () => dispatch('menu');
</script>

<header
  class="chrome"
  role="presentation"
  draggable="true"
  onpointerdown={(event) => {
    event.stopPropagation();
    dispatch('pointerdown', event);
  }}
  ondragstart={(event) => {
    dispatch('dragstart', { event, panelId });
    event.dataTransfer?.setData('panel-id', panelId);
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
      {isWindow ? '[ ]' : 'POP'}
    </button>
    <button
      class="icon danger"
      title="Close panel"
      onclick={(event) => {
        event.stopPropagation();
        onClose();
      }}
    >
      x
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
