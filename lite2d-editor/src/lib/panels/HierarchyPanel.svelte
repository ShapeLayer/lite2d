<script lang="ts">
  type Node = { id: string; label: string; depth: number };

  const nodes: Node[] = [
    { id: 'root', label: 'Level Root', depth: 0 },
    { id: 'camera', label: 'Main Camera', depth: 1 },
    { id: 'lights', label: 'Lights', depth: 1 },
    { id: 'key', label: 'Key Light', depth: 2 },
    { id: 'fill', label: 'Fill Light', depth: 2 },
    { id: 'props', label: 'Props', depth: 1 },
    { id: 'mesh', label: 'Hero Mesh', depth: 2 },
    { id: 'fx', label: 'FX', depth: 1 },
    { id: 'smoke', label: 'Smoke Burst', depth: 2 }
  ];

  let selected = 'camera';
</script>

<div class="hierarchy">
  <div class="header">Hierarchy</div>
  <ul>
    {#each nodes as node}
      <li class:selected={selected === node.id} style={`--depth:${node.depth}`} onclick={() => (selected = node.id)}>
        <span class="bullet" aria-hidden="true" />
        <span>{node.label}</span>
      </li>
    {/each}
  </ul>
</div>

<style>
  .hierarchy {
    display: flex;
    flex-direction: column;
    gap: 10px;
    height: 100%;
  }

  .header {
    font-weight: 700;
    color: var(--ui-text);
    letter-spacing: 0.2px;
  }

  ul {
    list-style: none;
    margin: 0;
    padding: 0;
    display: flex;
    flex-direction: column;
    gap: 4px;
  }

  li {
    display: grid;
    grid-template-columns: 10px 1fr;
    gap: 8px;
    align-items: center;
    padding: 8px 10px;
    border-radius: var(--ui-radius);
    cursor: pointer;
    color: var(--ui-text);
    background: color-mix(in srgb, var(--ui-surface) 90%, transparent);
    border: var(--ui-border-width) solid transparent;
    padding-left: calc(12px + var(--depth) * 12px);
  }

  li:hover {
    border-color: color-mix(in srgb, var(--ui-border) 70%, transparent);
  }

  li.selected {
    border-color: color-mix(in srgb, var(--ui-primary) 80%, transparent);
    background: color-mix(in srgb, var(--ui-primary) 10%, var(--ui-surface) 90%);
  }

  .bullet {
    width: 8px;
    height: 8px;
    border-radius: 999px;
    background: var(--ui-text-muted);
  }

  li.selected .bullet {
    background: var(--ui-primary);
  }
</style>
