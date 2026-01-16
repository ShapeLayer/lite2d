<script lang="ts">
  import { Button, Checkbox } from 'lite2d-editor-ui';

  let playing = false;
  let showGrid = true;

  const togglePlay = () => {
    playing = !playing;
  };
</script>

<div class="scene-panel">
  <div class="toolbar">
    <Button variant={playing ? 'primary' : 'ghost'} label={playing ? 'Pause' : 'Play'} onclick={togglePlay} />
    <Button variant="ghost" label="Frame Debug" />
    <Checkbox label="Grid" bind:checked={showGrid} />
  </div>

  <div class="viewport" data-playing={playing}>
    <div class="sky" />
    <div class={`ground ${showGrid ? 'with-grid' : ''}`} />
    <div class="subject">
      <span class="title">Preview Camera</span>
      <span class="hint">Drag panels to rearrange the UI.</span>
    </div>
  </div>
</div>

<style>
  .scene-panel {
    display: flex;
    flex-direction: column;
    gap: 10px;
    height: 100%;
  }

  .toolbar {
    display: flex;
    align-items: center;
    gap: 8px;
  }

  .viewport {
    position: relative;
    flex: 1;
    min-height: 240px;
    background: linear-gradient(180deg, #1f2937 0%, #0f172a 60%);
    border-radius: var(--ui-radius);
    overflow: hidden;
    border: var(--ui-border-width) solid var(--ui-border);
    box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.04);
  }

  .viewport[data-playing='true'] {
    outline: 2px solid color-mix(in srgb, var(--ui-primary) 60%, transparent);
    outline-offset: 2px;
  }

  .sky {
    position: absolute;
    inset: 0;
    background: radial-gradient(circle at 35% 25%, rgba(88, 166, 255, 0.25), transparent 40%),
      radial-gradient(circle at 70% 10%, rgba(255, 255, 255, 0.08), transparent 32%);
    pointer-events: none;
  }

  .ground {
    position: absolute;
    left: -10%;
    right: -10%;
    bottom: -5%;
    height: 52%;
    background: linear-gradient(180deg, rgba(18, 22, 30, 0.8), rgba(12, 16, 24, 1));
    clip-path: polygon(0 30%, 100% 0, 100% 100%, 0 100%);
    transform: rotate(-2deg);
  }

  .ground.with-grid::after {
    content: '';
    position: absolute;
    inset: 0;
    background-image: linear-gradient(rgba(255, 255, 255, 0.08) 1px, transparent 1px),
      linear-gradient(90deg, rgba(255, 255, 255, 0.08) 1px, transparent 1px);
    background-size: 24px 24px;
    mix-blend-mode: screen;
    opacity: 0.35;
  }

  .subject {
    position: absolute;
    left: 50%;
    top: 42%;
    transform: translate(-50%, -50%);
    padding: 14px 18px;
    background: color-mix(in srgb, var(--ui-surface) 82%, transparent);
    border: var(--ui-border-width) solid var(--ui-border);
    border-radius: var(--ui-radius);
    box-shadow: 0 18px 40px rgba(0, 0, 0, 0.35);
    display: flex;
    flex-direction: column;
    gap: 4px;
    text-align: center;
  }

  .title {
    font-weight: 700;
    color: var(--ui-text);
  }

  .hint {
    color: var(--ui-text-muted);
    font-size: 12px;
  }
</style>
