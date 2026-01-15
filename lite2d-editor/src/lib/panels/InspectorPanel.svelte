<script lang="ts">
  import { Checkbox, Select, Slider, TextInput } from 'lite2d-editor-ui';

  let name = 'Main Camera';
  let blendMode = 'alpha';
  let opacity = 84;
  let locked = false;
</script>

<div class="inspector">
  <div class="section">
    <div class="section-title">Identity</div>
    <TextInput label="Name" bind:value={name} />
    <Select
      label="Blend"
      options={[
        { label: 'Alpha', value: 'alpha' },
        { label: 'Add', value: 'add' },
        { label: 'Multiply', value: 'multiply' }
      ]}
      bind:value={blendMode}
    />
  </div>

  <div class="section">
    <div class="section-title">Rendering</div>
    <Slider label="Opacity" min={0} max={100} bind:value={opacity} />
    <Checkbox label="Lock Transform" bind:checked={locked} />
  </div>

  <div class="section note">
    <p>These controls are live Svelte components coming from the shared UI kit.</p>
  </div>
</div>

<style>
  .inspector {
    display: flex;
    flex-direction: column;
    gap: 14px;
  }

  .section {
    display: flex;
    flex-direction: column;
    gap: 10px;
    padding: 12px;
    border-radius: var(--ui-radius);
    background: color-mix(in srgb, var(--ui-surface) 92%, transparent);
    border: var(--ui-border-width) solid var(--ui-border);
  }

  .section-title {
    font-weight: 700;
    color: var(--ui-text);
  }

  .note {
    color: var(--ui-text-muted);
    font-size: 13px;
  }
</style>
