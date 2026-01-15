<script lang="ts">
  import { createEventDispatcher } from 'svelte';

  const dispatch = createEventDispatcher();
  let { label = '', placeholder = '', value = $bindable('') } = $props();

  const onInput = (event: Event) => {
    const target = event.target as HTMLInputElement;
    value = target.value;
    dispatch('change', value);
  };
</script>

<label class="text-input">
  <span class="label">{label}</span>
  <input type="text" bind:value placeholder={placeholder} oninput={onInput} />
</label>

<style>
  .text-input {
    display: flex;
    flex-direction: column;
    gap: 6px;
    color: var(--ui-text);
    font-size: 13px;
  }

  .label {
    color: var(--ui-text-muted);
  }

  input {
    background: var(--ui-surface);
    border: var(--ui-border-width) solid var(--ui-border);
    color: var(--ui-text);
    padding: 8px 10px;
    border-radius: var(--ui-radius);
    outline: none;
  }

  input:focus {
    border-color: var(--ui-primary);
    box-shadow: 0 0 0 2px color-mix(in srgb, var(--ui-primary) 30%, transparent);
  }
</style>
