<script lang="ts">
  import { ui } from '../store';
  import type { SplitNode } from '../types';
  import PanelNode from './PanelNode.svelte';

  type Props = { node: SplitNode };
  let { node } = $props();
  let container: HTMLDivElement | null = null;

  const startResize = (index: number, event: PointerEvent) => {
    if (!container) return;
    const isHorizontal = node.direction === 'horizontal';
    const startPos = isHorizontal ? event.clientX : event.clientY;
    const startSizes = [...node.sizes];
    const containerSize = isHorizontal ? container.clientWidth : container.clientHeight;

    const onMove = (moveEvent: PointerEvent) => {
      const delta = (isHorizontal ? moveEvent.clientX : moveEvent.clientY) - startPos;
      const ratio = delta / Math.max(1, containerSize);
      let nextSizes = [...startSizes];
      nextSizes[index] = Math.max(0.1, startSizes[index] + ratio);
      nextSizes[index + 1] = Math.max(0.1, startSizes[index + 1] - ratio);
      const total = nextSizes.reduce((acc, size) => acc + size, 0) || 1;
      nextSizes = nextSizes.map((size) => size / total);
      ui.updateSplitSizes(node.id, nextSizes);
    };

    const onUp = () => {
      window.removeEventListener('pointermove', onMove);
      window.removeEventListener('pointerup', onUp);
    };

    window.addEventListener('pointermove', onMove);
    window.addEventListener('pointerup', onUp);
  };
</script>

<div
  class={`split ${node.direction}`}
  bind:this={container}
>
  {#each node.children as child, index (child.id)}
    <div class="pane" style={`flex:${node.sizes[index] ?? 1} 1 0`}>
      <PanelNode node={child} />
    </div>
    {#if index < node.children.length - 1}
      <div
        class={`divider ${node.direction}`}
        onpointerdown={(event) => startResize(index, event)}
      ></div>
    {/if}
  {/each}
</div>

<style>
  .split {
    display: flex;
    height: 100%;
    width: 100%;
    gap: 0;
  }

  .split.horizontal {
    flex-direction: row;
  }

  .split.vertical {
    flex-direction: column;
  }

  .pane {
    display: flex;
    min-width: 160px;
    min-height: 120px;
    position: relative;
    overflow: hidden;
  }

  .divider {
    background: var(--ui-border);
    transition: background 120ms ease;
    position: relative;
    flex: 0 0 auto;
    z-index: 10;
  }

  .divider.horizontal {
    width: 6px;
    cursor: col-resize;
  }

  .divider.vertical {
    height: 6px;
    cursor: row-resize;
  }

  .divider::after {
    content: '';
    position: absolute;
    top: -4px;
    bottom: -4px;
    left: -4px;
    right: -4px;
    background: transparent;
  }

  .divider:hover {
    background: var(--ui-primary);
  }
</style>
