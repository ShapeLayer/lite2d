<script lang="ts">
  type Asset = { name: string; type: string; size: string };

  const assets: Asset[] = [
    { name: 'Hero.mesh', type: 'Mesh', size: '1.1 MB' },
    { name: 'City.hdr', type: 'HDRI', size: '8.6 MB' },
    { name: 'SmokeBurst.vfx', type: 'Effect', size: '420 KB' },
    { name: 'UI.theme', type: 'Theme', size: '6 KB' }
  ];
</script>

<div class="project">
  <div class="header">Project Assets</div>
  <div class="grid">
    {#each assets as asset}
      <article class="card">
        <div class="name">{asset.name}</div>
        <div class="meta">{asset.type}</div>
        <div class="size">{asset.size}</div>
      </article>
    {/each}
  </div>
</div>

<style>
  .project {
    display: flex;
    flex-direction: column;
    gap: 10px;
  }

  .header {
    font-weight: 700;
    color: var(--ui-text);
  }

  .grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(160px, 1fr));
    gap: 10px;
  }

  .card {
    background: color-mix(in srgb, var(--ui-surface) 90%, transparent);
    border: var(--ui-border-width) solid var(--ui-border);
    border-radius: var(--ui-radius);
    padding: 10px 12px;
    display: flex;
    flex-direction: column;
    gap: 4px;
  }

  .name {
    font-weight: 700;
  }

  .meta {
    color: var(--ui-text-muted);
    font-size: 12px;
  }

  .size {
    color: var(--ui-primary);
    font-size: 12px;
  }
</style>
