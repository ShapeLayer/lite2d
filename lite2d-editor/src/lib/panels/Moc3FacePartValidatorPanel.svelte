<script lang="ts">
  import { Select, TextInput } from 'lite2d-editor-ui';
  import { AlertTriangle, CheckCircle2, Plus, XCircle } from '@lucide/svelte';
  import { FACE_PART_TAGS, moc3Actions, moc3State } from '../moc3/store';

  let addTargets: Record<string, string> = {};
  let addFilters: Record<string, string> = {};

  $: drawableOptions = ($moc3State.model?.drawables ?? []).map((drawable) => ({
    label: drawable.id,
    value: drawable.id
  }));

  const groupedTags = {
    left: FACE_PART_TAGS.filter((tag) => tag.endsWith('_left')),
    right: FACE_PART_TAGS.filter((tag) => tag.endsWith('_right')),
    other: FACE_PART_TAGS.filter((tag) => !tag.endsWith('_left') && !tag.endsWith('_right'))
  };

  $: facePartsSnapshot = $moc3State.faceParts;
  $: assignedByTag = (() => {
    const map: Record<string, string[]> = {};
    const entries = Object.entries(facePartsSnapshot ?? {});
    FACE_PART_TAGS.forEach((tag) => {
      map[tag] = entries.filter(([, tags]) => tags.includes(tag)).map(([meshId]) => meshId);
    });
    return map;
  })();

  $: availableOptionsMap = (() => {
    const map: Record<string, { label: string; value: string }[]> = {};
    FACE_PART_TAGS.forEach((tag) => {
      const assigned = new Set(assignedByTag[tag] ?? []);
      const query = (addFilters[tag] ?? '').trim().toLowerCase();
      map[tag] = drawableOptions.filter((opt) => {
        if (assigned.has(opt.value)) return false;
        if (!query) return true;
        return opt.label.toLowerCase().includes(query) || opt.value.toLowerCase().includes(query);
      });
    });
    return map;
  })();

  const selectKey = (tag: string) => {
    const query = (addFilters[tag] ?? '').trim();
    const count = (availableOptionsMap[tag] ?? []).length;
    return `${tag}-${query}-${count}`;
  };

  const addSelected = (tag: string) => {
    const meshId = addTargets[tag];
    if (!meshId) return;
    moc3Actions.addFacePartTag(meshId, tag);
    addTargets = { ...addTargets, [tag]: '' };
  };
</script>

<div class="panel">
  {#if $moc3State.model}
    <div class="summary">
      <div>
        <div class="label">Face Part Validator</div>
        <div class="value">{$moc3State.model.drawables.length} drawables</div>
      </div>
      <div class="summary-status">
        {#if FACE_PART_TAGS.some((tag) => (assignedByTag[tag] ?? []).length === 0)}
          <AlertTriangle size={16} />
          <span>Missing parts</span>
        {:else}
          <CheckCircle2 size={16} />
          <span>All parts mapped</span>
        {/if}
      </div>
    </div>

    <div class="parts">
      {#each ['left', 'right', 'other'] as groupName}
        {@const tags = groupedTags[groupName as 'left' | 'right' | 'other']}
        <div class="group">
          <div class="group-title">{groupName.toUpperCase()}</div>
          <div class="group-grid">
            {#each tags as tag}
              {#key tag}
                {@const assigned = assignedByTag[tag] ?? []}
                <div class:missing={assigned.length === 0} class="part-card">
                  <div class="part-header">
                    <div class="part-header-subtitle">
                      <div class="part-name">{tag}</div>
                      <div class="part-meta">
                        {#if assigned.length === 0}
                          <span class="warning">No meshes</span>
                        {:else}
                          <span>{assigned.length} mesh</span>
                        {/if}
                      </div>
                    </div>
                    <button type="button" class="ui-btn ghost" on:click={() => moc3Actions.clearFacePartTag(tag)}>
                      <XCircle size={12} />
                      Clear
                    </button>
                  </div>

                  <div class="assigned-list">
                    {#if assigned.length === 0}
                      <div class="placeholder">Assign meshes.</div>
                    {:else}
                      {#each assigned as meshId}
                        <div class="assigned-item">
                          <span class="mesh-id" title={meshId}>{meshId}</span>
                          <button
                            type="button"
                            class="ui-btn ghost"
                            title="Remove"
                            on:click={() => moc3Actions.removeFacePartTag(meshId, tag)}
                          >
                            <XCircle size={12} />
                            Remove
                          </button>
                        </div>
                      {/each}
                    {/if}
                  </div>

                  <div class="add-row">
                    <TextInput
                      class="search-input"
                      label="Search"
                      placeholder="Type to filter"
                      value={addFilters[tag] ?? ''}
                      on:change={(event) => (addFilters = { ...addFilters, [tag]: event.detail ?? '' })}
                    />
                    {#key selectKey(tag)}
                      <Select
                        label="Add"
                        options={availableOptionsMap[tag] ?? []}
                        value={addTargets[tag] ?? ''}
                        on:change={(event) => {
                          const nextValue = event.detail ?? '';
                          addTargets = { ...addTargets, [tag]: nextValue };
                          if (nextValue) {
                            moc3Actions.addFacePartTag(nextValue, tag);
                            addTargets = { ...addTargets, [tag]: '' };
                          }
                        }}
                      />
                    {/key}
                    <button type="button" class="ui-btn" on:click={() => addSelected(tag)}>
                      <Plus size={12} />
                      Add
                    </button>
                  </div>
                </div>
              {/key}
            {/each}
          </div>
        </div>
      {/each}
    </div>
  {:else}
    <div class="placeholder">No moc3 file loaded. Load a file to validate face parts.</div>
  {/if}
</div>

<style>
  .panel {
    display: flex;
    flex-direction: column;
    gap: 12px;
  }

  .summary {
    display: flex;
    justify-content: space-between;
    align-items: center;
    gap: 12px;
  }

  .summary-status {
    display: inline-flex;
    align-items: center;
    gap: 4px;
    font-size: 11px;
    padding: 4px 8px;
    border-radius: 999px;
    border: 1px solid var(--ui-border);
    background: color-mix(in srgb, var(--ui-surface) 92%, transparent);
  }

  .label {
    color: var(--ui-text-muted);
    font-size: 11px;
  }

  .value {
    color: var(--ui-text);
    font-weight: 700;
  }

  .parts {
    display: grid;
    gap: 10px;
    overflow: auto;
  }

  .group {
    display: flex;
    flex-direction: column;
    gap: 6px;
  }

  .group-title {
    font-size: 11px;
    font-weight: 700;
    color: var(--ui-text-muted);
  }

  .group-grid {
    display: grid;
    gap: 8px;
  }

  .part-card {
    border: var(--ui-border-width) solid var(--ui-border);
    border-radius: var(--ui-radius);
    padding: 8px;
    background: color-mix(in srgb, var(--ui-surface) 92%, transparent);
    display: flex;
    flex-direction: column;
    gap: 6px;
  }

  .part-card.missing {
    border-color: color-mix(in srgb, var(--ui-danger) 55%, var(--ui-border));
    background: color-mix(in srgb, var(--ui-danger) 8%, var(--ui-surface));
  }

  .part-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    gap: 10px;
  }

  .part-header-subtitle {
    display: flex;
    flex-direction: row;
    gap: 4px;
    justify-content: center;
    align-items: center;
  }

  .part-name {
    font-weight: 700;
    font-size: 12px;
  }

  .part-meta {
    color: var(--ui-text-muted);
    font-size: 11px;
  }

  .warning {
    color: var(--ui-danger);
    font-weight: 600;
  }

  .assigned-list {
    display: flex;
    flex-direction: column;
    gap: 4px;
  }

  .assigned-item {
    display: flex;
    justify-content: space-between;
    align-items: center;
    gap: 8px;
    padding: 0 6px;
    border-radius: var(--ui-radius);
    border: 1px solid var(--ui-border);
    background: color-mix(in srgb, var(--ui-surface) 96%, transparent);
  }

  .mesh-id {
    font-weight: normal;
    font-size: 10pt;
    word-break: break-all;
  }

  .select select {
    padding: 4px 6px;
  }

  .add-row {
    display: grid;
    grid-template-columns: minmax(140px, 1fr) minmax(140px, 1fr) auto;
    gap: 8px;
    align-items: end;
  }

  .search-input :global(input) {
    padding: 4px 6px;
  }

  .ui-btn {
    border: var(--ui-border-width) solid var(--ui-border);
    background: var(--ui-muted);
    color: var(--ui-text);
    padding: 4px 8px;
    border-radius: var(--ui-radius);
    cursor: pointer;
    font-weight: 600;
    font-size: 11px;
    display: inline-flex;
    gap: 4px;
    align-items: center;
  }

  .ui-btn.ghost {
    background: transparent;
  }

  .placeholder {
    color: var(--ui-text-muted);
    padding: 8px;
    border: 1px dashed var(--ui-border);
    border-radius: var(--ui-radius);
    font-size: 11px;
  }
</style>
