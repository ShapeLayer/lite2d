<script lang="ts">
  import { TextInput } from 'lite2d-editor-ui';
  import { XCircle } from '@lucide/svelte';
  import { moc3Actions, moc3State, BODY_PART_TAGS } from '../moc3/store';

  let filter = '';

  $: filteredDrawables = $moc3State.model
    ? $moc3State.model.drawables.filter((drawable) => drawable.id.toLowerCase().includes(filter.toLowerCase()))
    : [];

  const hasTag = (meshId: string, tag: string) => {
    const tags = $moc3State.bodyParts[meshId] ?? [];
    return tags.includes(tag);
  };
</script>

<div class="panel">
  {#if $moc3State.model}
    <div class="header">
      <div>
        <div class="label">Body/Seam Parts Mapping</div>
        <div class="value">{$moc3State.model.drawables.length} drawables</div>
      </div>
      <TextInput placeholder="Filter drawables" value={filter} on:input={(event) => (filter = (event.target as HTMLInputElement).value)} />
    </div>

    <div class="table">
      <div class="table-head">
        <div>Drawable</div>
        <div>Tags</div>
        <div class="actions"></div>
      </div>
      <div class="table-body">
        {#each filteredDrawables as drawable}
          <div class="row">
            <div class="id" title={drawable.id}>{drawable.id}</div>
            <div class="tags">
              {#each BODY_PART_TAGS as tag}
                <label class:active={hasTag(drawable.id, tag)}>
                  <input
                    type="checkbox"
                    checked={hasTag(drawable.id, tag)}
                    on:change={() => moc3Actions.toggleBodyPart(drawable.id, tag)}
                  />
                  <span>{tag}</span>
                </label>
              {/each}
            </div>
            <div class="actions">
              <button type="button" class="ui-btn ghost" title="Clear tags" on:click={() => moc3Actions.clearBodyPartsForMesh(drawable.id)}>
                <XCircle size={16} />
                Clear
              </button>
            </div>
          </div>
        {/each}
      </div>
    </div>
  {:else}
    <div class="placeholder">No moc3 file loaded. Load a file to map body parts.</div>
  {/if}
</div>

<style>
  .panel {
    display: flex;
    flex-direction: column;
    gap: 12px;
  }

  .header {
    display: flex;
    align-items: center;
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

  .table {
    display: grid;
    grid-template-rows: auto 1fr;
    border: var(--ui-border-width) solid var(--ui-border);
    border-radius: var(--ui-radius);
    overflow: hidden;
    background: color-mix(in srgb, var(--ui-surface) 92%, transparent);
    height: 100%;
  }

  .table-head {
    display: grid;
    grid-template-columns: minmax(220px, 1.2fr) minmax(360px, 2fr) 120px;
    gap: 12px;
    padding: 10px 12px;
    background: var(--ui-muted);
    font-weight: 700;
  }

  .table-body {
    display: flex;
    flex-direction: column;
    gap: 12px;
    padding: 10px 12px;
    overflow: auto;
  }

  .row {
    display: grid;
    grid-template-columns: minmax(220px, 1.2fr) minmax(360px, 2fr) 120px;
    gap: 12px;
    align-items: flex-start;
    border-bottom: 1px solid var(--ui-border);
    padding-bottom: 10px;
  }

  .row:last-child {
    border-bottom: none;
  }

  .id {
    font-weight: 600;
    word-break: break-all;
  }

  .tags {
    display: flex;
    flex-wrap: wrap;
    gap: 6px 10px;
  }

  label {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    padding: 4px 8px;
    border-radius: 999px;
    border: 1px solid var(--ui-border);
    font-size: 11px;
    cursor: pointer;
    background: color-mix(in srgb, var(--ui-surface) 92%, transparent);
  }

  label.active {
    background: color-mix(in srgb, var(--ui-primary) 14%, var(--ui-surface));
    border-color: color-mix(in srgb, var(--ui-primary) 60%, var(--ui-border));
  }

  label input {
    accent-color: var(--ui-primary);
  }

  .actions {
    display: flex;
    justify-content: flex-end;
  }

  .ui-btn {
    border: var(--ui-border-width) solid var(--ui-border);
    background: var(--ui-muted);
    color: var(--ui-text);
    padding: 6px 10px;
    border-radius: var(--ui-radius);
    cursor: pointer;
    font-weight: 600;
    display: inline-flex;
    gap: 6px;
    align-items: center;
  }

  .ui-btn.ghost {
    background: transparent;
  }

  .placeholder {
    color: var(--ui-text-muted);
    padding: 12px;
    border: 1px dashed var(--ui-border);
    border-radius: var(--ui-radius);
  }
</style>
