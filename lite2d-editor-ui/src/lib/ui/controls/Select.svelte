<script lang="ts">
  import { createEventDispatcher } from 'svelte';

  const dispatch = createEventDispatcher();
  let { options = [] as { label: string; value: string }[], value: initialValue = '', label = 'Select' } = $props();
  let value = $state(initialValue);

  const onChange = (event: Event) => {
    const target = event.target as HTMLSelectElement;
    value = target.value;
    dispatch('change', value);
  };
</script>

<label class="select">
  <span class="label">{label}</span>
  <select bind:value onchange={onChange}>
    {#each options as option}
      <option value={option.value}>{option.label}</option>
    {/each}
  </select>
</label>

<style>
  .select {
    display: flex;
    flex-direction: column;
    gap: 6px;
    color: var(--ui-text);
    font-size: 13px;
  }

  select {
    background: var(--ui-surface);
    border: var(--ui-border-width) solid var(--ui-border);
    color: var(--ui-text);
    padding: 8px 10px;
    border-radius: var(--ui-radius);
    outline: none;
  }
</style>
