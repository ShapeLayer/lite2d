<script lang="ts">
  import { createEventDispatcher } from 'svelte';

  type Props = {
    variant?: 'primary' | 'ghost' | 'danger';
    disabled?: boolean;
    block?: boolean;
    label?: string;
  };

  export interface $$Events {
    click: MouseEvent;
  }

  const dispatch = createEventDispatcher<$$Events>();
  let { variant = 'primary', disabled = false, block = false, label = '' } = $props();

  const handleClick = (event: MouseEvent) => {
    if (disabled) return;
    dispatch('click', event);
  };
</script>

<button class={`btn ${variant} ${block ? 'block' : ''}`} disabled={disabled} onclick={handleClick}>
  {label}
</button>

<style>
  .btn {
    border: var(--ui-border-width) solid var(--ui-border);
    background: var(--ui-muted);
    color: var(--ui-text);
    padding: 8px 12px;
    border-radius: var(--ui-radius);
    cursor: pointer;
    font-weight: 600;
    transition: transform 120ms ease, box-shadow 120ms ease, background 120ms ease;
    width: auto;
  }

  .btn.primary {
    background: var(--ui-primary);
    border-color: color-mix(in srgb, var(--ui-primary) 70%, var(--ui-border) 30%);
    color: white;
  }

  .btn.ghost {
    background: transparent;
  }

  .btn.danger {
    background: color-mix(in srgb, var(--ui-danger) 80%, #000 5%);
    border-color: color-mix(in srgb, var(--ui-danger) 70%, var(--ui-border) 30%);
    color: white;
  }

  .btn:hover:not(:disabled) {
    transform: translateY(-1px);
    box-shadow: 0 10px 20px rgba(0, 0, 0, 0.15);
  }

  .btn:disabled {
    opacity: 0.6;
    cursor: not-allowed;
  }

  .btn.block {
    width: 100%;
  }
</style>
