<script lang="ts">
  type TreeNode = {
    id: string;
    label: string;
    children?: TreeNode[];
    open?: boolean;
  };

  let { nodes = [] as TreeNode[] } = $props();

  const toggle = (node: TreeNode) => {
    node.open = !node.open;
    nodes = [...nodes];
  };
</script>

<div class="tree">
  {#each nodes as node (node.id)}
    <div class="row">
      {#if node.children?.length}
        <button class="caret" onclick={() => toggle(node)}>{node.open ? 'v' : '>'}</button>
      {:else}
        <span class="leaf">-</span>
      {/if}
      <span>{node.label}</span>
    </div>
    {#if node.open && node.children?.length}
      <div class="children">
        <svelte:self nodes={node.children} />
      </div>
    {/if}
  {/each}
</div>

<style>
  .tree {
    display: flex;
    flex-direction: column;
    gap: 4px;
    font-size: 13px;
    color: var(--ui-text);
  }

  .row {
    display: flex;
    align-items: center;
    gap: 6px;
  }

  .caret {
    background: transparent;
    border: none;
    color: var(--ui-text-muted);
    cursor: pointer;
  }

  .leaf {
    color: var(--ui-text-muted);
  }

  .children {
    margin-left: 18px;
    border-left: 1px solid var(--ui-border);
    padding-left: 10px;
  }
</style>
