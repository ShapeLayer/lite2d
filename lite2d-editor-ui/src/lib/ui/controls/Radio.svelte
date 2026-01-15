<script lang="ts">
  import { createEventDispatcher } from 'svelte';

  const dispatch = createEventDispatcher();
  let { name = 'radio', value = '', selected = '', label = '', disabled = false } = $props();

  const onChange = () => dispatch('change', value);
</script>

<label class="radio" class:disabled={disabled}>
  <input type="radio" name={name} value={value} checked={selected === value} {disabled} onchange={onChange} />
  <span class="dot"></span>
  <span class="label">{label}</span>
</label>

<style>
  .radio {
    display: inline-flex;
    align-items: center;
    gap: 8px;
    cursor: pointer;
    color: var(--ui-text);
  }

  .radio.disabled {
    opacity: 0.6;
    cursor: not-allowed;
  }

  input {
    display: none;
  }

  .dot {
    width: 16px;
    height: 16px;
    border-radius: 50%;
    border: var(--ui-border-width) solid var(--ui-border);
    background: var(--ui-muted);
    position: relative;
  }

  input:checked + .dot::after {
    content: '';
    position: absolute;
    top: 3px;
    left: 3px;
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: var(--ui-primary);
  }

  .label {
    font-size: 13px;
  }
</style>
