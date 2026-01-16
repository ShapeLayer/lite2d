<script lang="ts">
  import { TextInput } from 'lite2d-editor-ui';
  import { CircleDot, Eye, EyeOff, GripHorizontal, Grid2x2, Image as ImageIcon } from '@lucide/svelte';
  import { tick } from 'svelte';
  import { moc3Actions, moc3State } from '../moc3/store';
  import type { Moc3Drawable } from '../moc3/parse';

  let meshFilter = '';
  let draggingId: string | null = null;
  let dragOverId: string | null = null;
  let rowElements: Record<string, HTMLDivElement> = {};
  let headerScrollEl: HTMLDivElement | null = null;
  let bodyScrollEl: HTMLDivElement | null = null;

  const handleFilterInput = (event: Event) => {
    const value = (event.currentTarget as HTMLInputElement | null)?.value ?? '';
    meshFilter = value;
    const first = $moc3State.model?.drawables.find((mesh) => mesh.id.toLowerCase().includes(value.toLowerCase()));
    if (first) selectMesh(first.id);
  };

  const registerRow = (node: HTMLDivElement, id: string) => {
    rowElements[id] = node;
    return {
      update(nextId: string) {
        if (nextId !== id) {
          delete rowElements[id];
          id = nextId;
          rowElements[id] = node;
        }
      },
      destroy() {
        delete rowElements[id];
      }
    };
  };

  const selectMesh = (id: string) => {
    moc3Actions.selectMesh(id);
    tick().then(() => {
      const row = rowElements[id];
      row?.scrollIntoView({ block: 'nearest', inline: 'nearest' });
    });
  };

  const handleOrderDragStart = (event: DragEvent, id: string) => {
    draggingId = id;
    event.dataTransfer?.setData('text/plain', id);
    event.dataTransfer?.setDragImage(event.currentTarget as Element, 12, 12);
    event.dataTransfer?.setData('application/x-lite2d-drawable', id);
  };

  const handleOrderDragOver = (event: DragEvent) => {
    event.preventDefault();
    if (event.dataTransfer) {
      event.dataTransfer.dropEffect = 'move';
    }
    const target = event.currentTarget as HTMLElement | null;
    const targetId = target?.dataset?.rowId ?? null;
    dragOverId = targetId;
  };

  const handleOrderDrop = (event: DragEvent, id: string) => {
    event.preventDefault();
    const dragged = event.dataTransfer?.getData('application/x-lite2d-drawable') || draggingId;
    draggingId = null;
    dragOverId = null;
    if (!dragged || !$moc3State.model) return;
    const order = $moc3State.renderOrder.length ? $moc3State.renderOrder : $moc3State.model.drawables.map((drawable) => drawable.id);
    const targetIndex = order.indexOf(id);
    moc3Actions.moveDrawableTo(dragged, targetIndex);
  };

  const handleOrderDragEnd = () => {
    draggingId = null;
    dragOverId = null;
  };

  const syncHeaderScroll = () => {
    if (!headerScrollEl || !bodyScrollEl) return;
    headerScrollEl.scrollLeft = bodyScrollEl.scrollLeft;
  };

  let meshRowsData: {
    id: string;
    vertices: number;
    uvs: number;
    draw: number;
    visible: boolean;
    orderIndex: number;
    orderTotal: number;
  }[] = [];

  $: meshRowsData = (() => {
    const model = $moc3State.model;
    if (!model) return [];
    const drawables = new Map(model.drawables.map((drawable) => [drawable.id, drawable]));
    const order = $moc3State.renderOrder.length ? $moc3State.renderOrder : model.drawables.map((drawable) => drawable.id);
    const filter = meshFilter.trim().toLowerCase();
    return order
      .map((id) => drawables.get(id))
      .filter((drawable): drawable is Moc3Drawable => Boolean(drawable))
      .filter((drawable) => drawable.id.toLowerCase().includes(filter))
      .map((drawable, index) => ({
        id: drawable.id,
        vertices: drawable.vertexCount,
        uvs: drawable.uvs.length,
        draw: drawable.textureIndex,
        visible: !$moc3State.hiddenDrawables[drawable.id],
        orderIndex: $moc3State.renderOrder.length ? $moc3State.renderOrder.indexOf(drawable.id) : index,
        orderTotal: $moc3State.renderOrder.length ? $moc3State.renderOrder.length : model.drawables.length
      }));
  })();
</script>

<div class="panel">
  {#if $moc3State.model}
    <div class="table-wrapper">
      <div class="table-header">
        <TextInput bind:value={meshFilter} placeholder="Type id to focus" oninput={handleFilterInput} />
      </div>
      <div class="mesh-table">
        <div class="mesh-header-scroll" bind:this={headerScrollEl} aria-hidden="true">
          <div class="mesh-header">
            <span>ID</span>
            <span class="header-icon" title="Vertices"><CircleDot size={14} /></span>
            <span class="header-icon" title="UVs"><Grid2x2 size={14} /></span>
            <span class="header-icon" title="Texture"><ImageIcon size={14} /></span>
            <span>Order</span>
            <span>Drag</span>
          </div>
        </div>
        <div class="mesh-body" bind:this={bodyScrollEl} onscroll={syncHeaderScroll}>
          {#each meshRowsData as row}
            <div
              class={`mesh-row ${row.id === $moc3State.selectedMeshId ? 'selected' : ''} ${row.id === draggingId ? 'dragging' : ''} ${row.id === dragOverId ? 'drag-over' : ''}`}
              role="row"
              tabindex="0"
              draggable="true"
              data-row-id={row.id}
              ondragstart={(event) => handleOrderDragStart(event, row.id)}
              ondragover={handleOrderDragOver}
              ondrop={(event) => handleOrderDrop(event, row.id)}
              ondragend={handleOrderDragEnd}
              use:registerRow={row.id}
            >
              <span
                style={`
                  justify-content: center;
                  align-items: center;
                  display: flex;
                  flex-direction: row;
                  gap: 1pt;
                `}
              >
                <button
                  class={`visibility-button ${row.visible ? 'visible' : 'hidden'}`}
                  onclick={(event) => {
                    event.stopPropagation();
                    moc3Actions.toggleVisibility(row.id)
                  }}
                  aria-label={row.visible ? 'Hide layer' : 'Show layer'}
                >
                  {#if row.visible}
                    <Eye size={16} />
                  {:else}
                    <EyeOff size={16} />
                  {/if}
                </button>
                <button class="mesh-select" onclick={() => selectMesh(row.id)} title={row.id}>
                  {row.id}
                </button>
              </span>
              <span>{row.vertices}</span>
              <span>{row.uvs}</span>
              <span>{row.draw}</span>
              <div class="order-controls">
                <button onclick={(event) => {
                  event.stopPropagation();
                  moc3Actions.moveDrawable(row.id, -1)
                }} 
                disabled={row.orderIndex === 0}>
                  Up
                </button>
                <button
                  onclick={(event) => {
                    event.stopPropagation();
                    moc3Actions.moveDrawable(row.id, 1)
                  }}
                  disabled={row.orderIndex === row.orderTotal - 1}
                >
                  Down
                </button>
              </div>
              <div class={`drag-handle ${row.id === draggingId ? 'active' : ''}`} aria-label="Drag to reorder">
                <GripHorizontal size={16} />
              </div>
            </div>
          {/each}
        </div>
      </div>
    </div>
  {:else}
    <div class="placeholder">No moc3 file loaded. Load a file to inspect drawables.</div>
  {/if}
</div>

<style>
  .panel {
    display: flex;
    flex-direction: column;
    gap: 12px;
  }

  .table-wrapper {
    display: flex;
    flex-direction: column;
    gap: 10px;
    border: var(--ui-border-width) solid var(--ui-border);
    border-radius: var(--ui-radius);
    padding: 10px;
    background: color-mix(in srgb, var(--ui-surface) 92%, transparent);
    height: 100%;
  }

  .table-header {
    display: flex;
    align-items: flex-end;
    justify-content: space-between;
    gap: 12px;
  }

  .label {
    color: var(--ui-text-muted);
    font-size: 12px;
  }

  .value {
    color: var(--ui-text);
    font-weight: 700;
  }

  .mesh-table {
    display: flex;
    flex-direction: column;
    border: var(--ui-border-width) solid var(--ui-border);
    border-radius: var(--ui-radius);
    overflow: hidden;
    flex: 1;
    font-size: 10pt;
  }

  .mesh-header-scroll {
    overflow-x: hidden;
  }

  .mesh-header {
    display: grid;
    grid-template-columns: minmax(100px, 2fr) minmax(30px, 1fr) minmax(30px, 1fr) minmax(30px, 1fr) 90px minmax(30px, 1fr);
    padding: 8px 10px;
    background: var(--ui-muted);
    color: var(--ui-text);
    font-weight: 700;
    gap: 8px;
    align-items: center;
    /* min-width: 620px; */
  }

  .mesh-header .header-icon {
    display: inline-flex;
    align-items: center;
    justify-content: center;
  }

  .mesh-body {
    overflow: auto;
    overflow-x: scroll;
    flex: 1;
  }

  .mesh-row {
    width: 100%;
    display: grid;
    grid-template-columns: minmax(100px, 2fr) minmax(30px, 1fr) minmax(30px, 1fr) minmax(30px, 1fr) 90px minmax(30px, 1fr);
    gap: 5px;
    padding: 4px 5px;
    border-bottom: var(--ui-border-width) solid var(--ui-border);
    background: color-mix(in srgb, var(--ui-surface) 94%, transparent);
    color: var(--ui-text);
    align-items: center;
    /* min-width: 620px; */
  }

  .mesh-row:hover {
    background: color-mix(in srgb, var(--ui-primary) 10%, var(--ui-surface) 92%);
  }

  .mesh-row.selected {
    background: color-mix(in srgb, var(--ui-primary) 14%, var(--ui-surface) 90%);
    border-color: color-mix(in srgb, var(--ui-primary) 40%, var(--ui-border));
  }

  .mesh-row.dragging {
    opacity: 0.6;
  }

  .mesh-row.drag-over {
    box-shadow: inset 0 2px 0 color-mix(in srgb, var(--ui-primary) 70%, transparent);
  }

  .mesh-select {
    border: none;
    background: transparent;
    color: inherit;
    text-align: left;
    font: inherit;
    padding: 0;
    cursor: pointer;
    white-space: normal;
    word-break: break-all;
  }

  .visibility-button,
  .order-controls button {
    border: var(--ui-border-width) solid var(--ui-border);
    border-radius: 6px;
    background: color-mix(in srgb, var(--ui-surface) 92%, transparent);
    color: var(--ui-text);
    padding: 0;
    font-size: 10px;
    cursor: pointer;
  }

  .visibility-button {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    width: 16px;
    height: 14px;
    justify-content: center;
  }

  .visibility-button.hidden {
    opacity: 0.6;
  }

  .order-controls button:disabled {
    opacity: 0.4;
    cursor: not-allowed;
  }

  .order-controls {
    display: flex;
    gap: 6px;
  }

  .order-controls button {
    padding: 2px 6px;
    font-size: 11px;
  }

  .drag-handle {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    width: 32px;
    height: 28px;
    border: var(--ui-border-width) solid var(--ui-border);
    border-radius: 6px;
    background: color-mix(in srgb, var(--ui-surface) 92%, transparent);
    color: var(--ui-text-muted);
    font-size: 14px;
    cursor: grab;
    user-select: none;
  }

  .drag-handle.active {
    cursor: grabbing;
  }

  .placeholder {
    color: var(--ui-text-muted);
    padding: 12px;
    border-radius: var(--ui-radius);
    background: color-mix(in srgb, var(--ui-surface) 92%, transparent);
  }
</style>
