<script lang="ts">
  import { TableView } from '..';

  const columns = [
    { key: 'name', label: 'Name', width: 220 },
    { key: 'type', label: 'Type', width: 120 },
    { key: 'size', label: 'Size', width: 90 }
  ];

  const rows = [
    { name: 'Meshes/Ship.fbx', type: 'FBX', size: '1.2 MB' },
    { name: 'Textures/Ship_BaseColor.png', type: 'PNG', size: '640 KB' },
    { name: 'Materials/Ship.mat', type: 'Material', size: '1 KB' },
    { name: 'Scripts/Player.cs', type: 'C#', size: '3 KB' }
  ];
</script>

<TableView {columns} {rows} />
