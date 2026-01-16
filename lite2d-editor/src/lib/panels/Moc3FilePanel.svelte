<script lang="ts">
  import { moc3Actions, moc3State } from '../moc3/store';

  const handleDrop = async (event: DragEvent) => {
    event.preventDefault();
    const files = event.dataTransfer?.files;
    if (!files?.length) return;

    const mocFile = Array.from(files).find((f) => f.name.endsWith('.moc3.json') || f.name.endsWith('.json'));
    if (mocFile) {
      const text = await mocFile.text();
      moc3Actions.loadFromText(text, mocFile.name);
    }

    moc3Actions.loadTextureFromFileList(files);
  };

</script>

<div class="panel">
  <div class="status">{$moc3State.errorMessage ? $moc3State.errorMessage : $moc3State.statusMessage}</div>

  <div
    class="dropzone"
    role="button"
    tabindex="0"
    ondragover={(event) => event.preventDefault()}
    ondrop={(event) => handleDrop(event)}
    aria-label="Drop moc3 file"
  >
    <div>
      <p>Drop a Live2D .moc3.json file here</p>
      <p class="hint">UVs and mesh stats will be extracted client-side.</p>
    </div>
  </div>

  {#if $moc3State.model}
    <div class="meta">
      <div class="meta-card">
        <div class="label">File</div>
        <div class="value">{$moc3State.model.name}</div>
        <div class="label">Texture</div>
        <div class="value">{$moc3State.textureName || 'N/A'}</div>
        <div class="status muted">{$moc3State.textureStatus}</div>
      </div>
      <div class="meta-card">
        <div class="label">Canvas</div>
        <div class="value">
          {$moc3State.model.canvas.width.toFixed(3)} × {$moc3State.model.canvas.height.toFixed(3)}
        </div>
        <div class="label">Pixels / Unit</div>
        <div class="value">{$moc3State.model.canvas.pixelsPerUnit.toFixed(2)}</div>
      </div>
      <div class="meta-card">
        <div class="label">Drawables</div>
        <div class="value">{$moc3State.model.drawables.length}</div>
        <div class="label">Totals (verts / uvs)</div>
        <div class="value">
          {$moc3State.model.drawables.reduce((sum, d) => sum + d.vertexCount, 0)} / {$moc3State.model.drawables.reduce((sum, d) => sum + d.uvs.length, 0)}
        </div>
      </div>
    </div>
  {:else}
    <div class="placeholder">No moc3 file loaded. Use the buttons above to start.</div>
  {/if}
</div>

<style>
  .panel {
    display: flex;
    flex-direction: column;
    gap: 12px;
  }

  .status {
    color: var(--ui-text-muted);
    font-size: 13px;
  }

  .status.muted {
    color: var(--ui-text-muted);
    font-size: 12px;
  }

  .dropzone {
    border: 1px dashed var(--ui-border);
    border-radius: var(--ui-radius);
    padding: 16px;
    background: color-mix(in srgb, var(--ui-surface) 92%, transparent);
    text-align: center;
    color: var(--ui-text);
  }

  .dropzone .hint {
    color: var(--ui-text-muted);
    margin: 4px 0 0 0;
    font-size: 12px;
  }

  .meta {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
    gap: 10px;
  }

  .meta-card {
    border: var(--ui-border-width) solid var(--ui-border);
    border-radius: var(--ui-radius);
    padding: 10px 12px;
    background: color-mix(in srgb, var(--ui-surface) 94%, transparent);
    display: grid;
    grid-template-columns: 1fr;
    gap: 4px;
  }

  .label {
    color: var(--ui-text-muted);
    font-size: 12px;
  }

  .value {
    color: var(--ui-text);
    font-weight: 700;
  }

  .placeholder {
    color: var(--ui-text-muted);
    padding: 12px;
    border-radius: var(--ui-radius);
    background: color-mix(in srgb, var(--ui-surface) 92%, transparent);
  }
</style>
