<script lang="ts">
  import { createEventDispatcher } from 'svelte';

  const dispatch = createEventDispatcher();
  let { checked = $bindable(false), label = '', disabled = false } = $props();

  const onChange = () => dispatch('change', checked);
</script>

<label class="checkbox" class:disabled={disabled}>
  <input type="checkbox" bind:checked {disabled} onchange={onChange} />
  <span class="box"></span>
  <span class="label">{label}</span>
</label>

<style>
  .checkbox {
    display: inline-flex;
    align-items: center;
    gap: 8px;
    cursor: pointer;
    color: var(--ui-text);
  }

  .checkbox.disabled {
    opacity: 0.6;
    cursor: not-allowed;
  }

  input {
    display: none;
  }

  .box {
    width: 16px;
    height: 16px;
    border: var(--ui-border-width) solid var(--ui-border);
    border-radius: 4px;
    background: var(--ui-muted);
    position: relative;
  }

  input:checked + .box::after {
    content: '';
    position: absolute;
    top: 2px;
    left: 5px;
    width: 4px;
    height: 8px;
    border: solid var(--ui-primary);
    border-width: 0 2px 2px 0;
    transform: rotate(45deg);
  }

  .label {
    font-size: 13px;
  }
</style>
