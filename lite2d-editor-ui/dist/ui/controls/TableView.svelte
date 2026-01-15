<script lang="ts">
  type Column = { key: string; label: string; width?: number };
  type Row = Record<string, string | number>;

  let { columns = [] as Column[], rows = [] as Row[] } = $props();
  let widths = $state(columns.map((c) => c.width ?? 160));

  $effect(() => {
    widths = columns.map((c, index) => widths[index] ?? c.width ?? 160);
  });

  const startResize = (index: number, event: PointerEvent) => {
    event.preventDefault();
    const startX = event.clientX;
    const startWidth = widths[index];
    const onMove = (move: PointerEvent) => {
      const dx = move.clientX - startX;
      widths[index] = Math.max(80, startWidth + dx);
      widths = [...widths];
    };
    const onUp = () => {
      window.removeEventListener('pointermove', onMove);
      window.removeEventListener('pointerup', onUp);
    };
    window.addEventListener('pointermove', onMove);
    window.addEventListener('pointerup', onUp);
  };
</script>

<div class="table">
  <div class="header">
    {#each columns as column, index}
      <div class="cell" style={`width:${widths[index]}px`}>
        <span>{column.label}</span>
        <div class="resizer" onpointerdown={(event) => startResize(index, event)}></div>
      </div>
    {/each}
  </div>
  <div class="body">
    {#each rows as row, rowIndex}
      <div class="row" class:striped={rowIndex % 2 === 1}>
        {#each columns as column, colIndex}
          <div class="cell" style={`width:${widths[colIndex]}px`}>
            {row[column.key] ?? ''}
          </div>
        {/each}
      </div>
    {/each}
  </div>
</div>

<style>
  .table {
    border: var(--ui-border-width) solid var(--ui-border);
    border-radius: var(--ui-radius);
    overflow: hidden;
    background: var(--ui-surface);
    font-size: 13px;
    color: var(--ui-text);
  }

  .header, .row {
    display: flex;
  }

  .header {
    background: var(--ui-muted);
    border-bottom: var(--ui-border-width) solid var(--ui-border);
    font-weight: 700;
  }

  .cell {
    padding: 8px 10px;
    border-right: var(--ui-border-width) solid var(--ui-border);
    box-sizing: border-box;
    position: relative;
    flex-shrink: 0;
  }

  .cell:last-child {
    border-right: none;
  }

  .resizer {
    position: absolute;
    right: 0;
    top: 0;
    width: 6px;
    height: 100%;
    cursor: col-resize;
  }

  .body {
    max-height: 320px;
    overflow: auto;
  }

  .row.striped {
    background: color-mix(in srgb, var(--ui-muted) 60%, transparent);
  }
</style>
