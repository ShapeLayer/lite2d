<script lang="ts">
  import { Button, Slider } from '$lib';

  let exposure = 1.0;
  let bloom = 0.12;
</script>

<div class="panel">
  <div class="hero">
    <div class="grid"></div>
    <div class="cursor">+</div>
  </div>
  <div class="controls">
    <Slider label="Exposure" min={-2} max={2} step={0.05} bind:value={exposure} />
    <Slider label="Bloom" min={0} max={1} step={0.01} bind:value={bloom} />
    <Button variant="primary" label="Play" />
  </div>
</div>

<style>
  .panel {
    display: grid;
    grid-template-rows: 1fr auto;
    gap: 12px;
    height: 100%;
  }

  .hero {
    background: radial-gradient(circle at 30% 40%, color-mix(in srgb, var(--ui-primary) 30%, transparent), transparent),
      radial-gradient(circle at 70% 60%, color-mix(in srgb, var(--ui-highlight) 50%, transparent), transparent),
      var(--ui-muted);
    border: 1px solid var(--ui-border);
    border-radius: var(--ui-radius);
    position: relative;
    overflow: hidden;
  }

  .grid {
    position: absolute;
    inset: 0;
    background-image: linear-gradient(var(--ui-border) 1px, transparent 1px), linear-gradient(90deg, var(--ui-border) 1px, transparent 1px);
    background-size: 40px 40px;
    opacity: 0.35;
  }

  .cursor {
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    font-size: 24px;
    color: var(--ui-primary);
  }

  .controls {
    display: grid;
    gap: 8px;
  }
</style>
