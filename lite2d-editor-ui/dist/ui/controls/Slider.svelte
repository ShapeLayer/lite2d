<script lang="ts">
  import { createEventDispatcher } from 'svelte';

  const dispatch = createEventDispatcher();
  let { min = 0, max = 100, step = 1, value = $bindable(0), label = '' } = $props();

  const onInput = (event: Event) => {
    const target = event.target as HTMLInputElement;
    value = Number(target.value);
    dispatch('change', value);
  };
</script>

<label class="slider">
  <div class="header">
    <span>{label}</span>
    <span class="value">{value}</span>
  </div>
  <input type="range" bind:value {min} {max} {step} oninput={onInput} />
</label>

<style>
  .slider {
    display: flex;
    flex-direction: column;
    gap: 6px;
    color: var(--ui-text);
    font-size: 13px;
  }

  .header {
    display: flex;
    justify-content: space-between;
    color: var(--ui-text-muted);
  }

  .value {
    color: var(--ui-text);
    font-variant-numeric: tabular-nums;
  }

  input[type='range'] {
    accent-color: var(--ui-primary);
  }
</style>
