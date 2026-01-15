<script lang="ts">
  import { TreeView } from '$lib';

  const nodes = [
    {
      id: 'root',
      label: 'Root',
      open: true,
      children: [
        { id: 'camera', label: 'Camera' },
        {
          id: 'player',
          label: 'Player',
          open: true,
          children: [
            { id: 'mesh', label: 'Mesh' },
            { id: 'collider', label: 'Collider' }
          ]
        },
        { id: 'lights', label: 'Lights', children: [{ id: 'sun', label: 'Sun' }] }
      ]
    }
  ];
</script>

<div class="panel">
  <TreeView nodes={nodes} />
</div>

<style>
  .panel {
    height: 100%;
    overflow: auto;
  }
</style>
