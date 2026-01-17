<script lang="ts">
  import RenderPreview from './RenderPreview.svelte';
  import { Checkbox } from 'lite2d-editor-ui';
  import { FACE_PART_TAGS, moc3Actions, moc3State } from '../moc3/store';

  let renderStatus = 'No render yet';
  let excludeHiddenFromSelection = true;

  const handleContextAction = (event: CustomEvent<{ action: 'add-face-part' | 'clear-face-parts' | 'set-visible'; ids: string[]; tag?: string }>) => {
    const { action, ids, tag } = event.detail;
    if (!ids.length) return;
    if (action === 'add-face-part' && tag) {
      ids.forEach((id) => moc3Actions.addFacePartTag(id, tag));
      return;
    }
    if (action === 'clear-face-parts') {
      ids.forEach((id) => moc3Actions.clearFacePartsForMesh(id));
      return;
    }
    if (action === 'set-visible') {
      const visible = tag !== 'hidden';
      ids.forEach((id) => {
        if ($moc3State.hiddenDrawables[id] === visible) return;
        moc3Actions.toggleVisibility(id);
      });
    }
  };
</script>

<div class="panel">
  {#if $moc3State.model}
    <div class="render-header">
      <div>
        <div class="label">Render Preview</div>
        <div class="value">{$moc3State.textureName || 'Texture not loaded'}</div>
        <div class="status muted">{renderStatus}</div>
      </div>
      <div class="render-actions">
        <Checkbox label="Exclude hidden" bind:checked={excludeHiddenFromSelection} />
        <button type="button" class="ui-btn ghost" onclick={moc3Actions.showAllDrawables}>
          Show all
        </button>
        <button type="button" class="ui-btn ghost" onclick={moc3Actions.resetRenderSettings}>
          Reset order
        </button>
      </div>
    </div>
    <RenderPreview
      bind:status={renderStatus}
      model={$moc3State.model}
      textureImage={$moc3State.textureImage}
      renderOrder={$moc3State.renderOrder}
      hiddenDrawables={$moc3State.hiddenDrawables}
      selectedIds={$moc3State.selectedMeshIds}
      facePartTags={FACE_PART_TAGS}
      excludeHiddenFromSelection={excludeHiddenFromSelection}
      on:select={(event) => moc3Actions.selectMesh(event.detail.id)}
      on:selectrange={(event) => moc3Actions.selectMeshes(event.detail.ids)}
      on:contextaction={handleContextAction}
    />
  {:else}
    <div class="placeholder">No moc3 file loaded. Load a file to render.</div>
  {/if}
</div>

<style>
  .panel {
    display: flex;
    flex-direction: column;
    gap: 12px;
  }

  .render-header {
    display: flex;
    align-items: flex-end;
    justify-content: space-between;
    gap: 12px;
  }

  .render-actions {
    display: flex;
    gap: 8px;
    flex-wrap: wrap;
  }

  .label {
    color: var(--ui-text-muted);
    font-size: 12px;
  }

  .value {
    color: var(--ui-text);
    font-weight: 700;
  }

  .status {
    color: var(--ui-text-muted);
    font-size: 13px;
  }

  .status.muted {
    color: var(--ui-text-muted);
    font-size: 12px;
  }

  .ui-btn {
    border: var(--ui-border-width) solid var(--ui-border);
    background: var(--ui-muted);
    color: var(--ui-text);
    padding: 6px 10px;
    border-radius: var(--ui-radius);
    cursor: pointer;
    font-weight: 600;
  }

  .ui-btn.ghost {
    background: transparent;
  }

  .placeholder {
    color: var(--ui-text-muted);
    padding: 12px;
    border-radius: var(--ui-radius);
    background: color-mix(in srgb, var(--ui-surface) 92%, transparent);
  }
</style>
