<script lang="ts">
  type LogEntry = {
    level: 'info' | 'warn' | 'error';
    message: string;
    time: string;
  };

  const logs: LogEntry[] = [
    { level: 'info', message: 'Build completed in 214ms', time: '12:03:10' },
    { level: 'warn', message: 'Camera is missing a depth target, using default', time: '12:03:11' },
    { level: 'info', message: 'Hot reload: ScenePanel.svelte', time: '12:03:12' },
    { level: 'error', message: 'Material graph failed to compile (node 7)', time: '12:03:15' }
  ];
</script>

<div class="console">
  {#each logs as log}
    <div class={`row ${log.level}`}>
      <span class="time">{log.time}</span>
      <span class="level">{log.level}</span>
      <span class="message">{log.message}</span>
    </div>
  {/each}
</div>

<style>
  .console {
    display: flex;
    flex-direction: column;
    gap: 6px;
    font-family: 'SFMono-Regular', ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, 'Liberation Mono', 'Courier New', monospace;
    font-size: 12px;
  }

  .row {
    display: grid;
    grid-template-columns: 72px 62px 1fr;
    gap: 10px;
    align-items: center;
    padding: 6px 8px;
    border-radius: var(--ui-radius);
    background: color-mix(in srgb, var(--ui-surface) 90%, transparent);
    border: var(--ui-border-width) solid transparent;
  }

  .row.info {
    color: var(--ui-text);
  }

  .row.warn {
    color: #eab308;
    border-color: color-mix(in srgb, #eab308 30%, transparent);
  }

  .row.error {
    color: var(--ui-danger);
    border-color: color-mix(in srgb, var(--ui-danger) 40%, transparent);
  }

  .time {
    color: var(--ui-text-muted);
  }

  .level {
    text-transform: uppercase;
    letter-spacing: 0.05em;
    font-weight: 700;
  }

  .message {
    color: var(--ui-text);
  }
</style>
