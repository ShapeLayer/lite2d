<script lang="ts">
  const logs = [
    { level: 'info', text: 'Loaded scene in 0.23s' },
    { level: 'warn', text: 'Texture atlas nearing limit' },
    { level: 'error', text: 'Mesh uv channel missing; using default' },
    { level: 'info', text: 'GPU frame time 3.1ms' }
  ];
</script>

<div class="console">
  {#each logs as log, index}
    <div class={`row ${log.level}`}>
      <span class="badge">{log.level.toUpperCase()}</span>
      <span class="text">{log.text}</span>
      <span class="time">{(index + 1).toString().padStart(2, '0')}s</span>
    </div>
  {/each}
</div>

<style>
  .console {
    font-family: 'JetBrains Mono', 'SFMono-Regular', ui-monospace, monospace;
    font-size: 12px;
    display: grid;
    gap: 6px;
  }

  .row {
    display: grid;
    grid-template-columns: 90px 1fr 60px;
    align-items: center;
    padding: 8px 10px;
    border: 1px solid var(--ui-border);
    border-radius: var(--ui-radius);
  }

  .badge {
    padding: 2px 6px;
    border-radius: 6px;
    font-weight: 700;
    color: white;
    text-align: center;
  }

  .row.info .badge {
    background: var(--ui-primary);
  }

  .row.warn .badge {
    background: #f59e0b;
  }

  .row.error .badge {
    background: var(--ui-danger);
  }

  .text {
    color: var(--ui-text);
  }

  .time {
    text-align: right;
    color: var(--ui-text-muted);
  }
</style>
