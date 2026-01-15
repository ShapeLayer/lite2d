<script lang="ts">
  import type { LayoutNode } from '../types';
  import SplitView from './SplitView.svelte';
  import TabView from './TabView.svelte';

  type Props = { node: LayoutNode };
  let { node } = $props();
</script>

{#if node.kind === 'split'}
  <SplitView node={node} />
{:else if node.kind === 'tabs'}
  <TabView node={node} />
{/if}
