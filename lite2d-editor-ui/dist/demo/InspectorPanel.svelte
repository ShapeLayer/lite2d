<script lang="ts">
  import { Checkbox, Slider, TextInput } from '..';

  let name = 'Player';
  let visible = true;
  let roughness = 0.42;
  let metallic = 0.12;
</script>

<div class="panel">
  <TextInput label="Name" bind:value={name} />
  <Checkbox label="Visible" bind:checked={visible} />
  <Slider label="Roughness" min={0} max={1} step={0.01} bind:value={roughness} />
  <Slider label="Metallic" min={0} max={1} step={0.01} bind:value={metallic} />
</div>

<style>
  .panel {
    display: grid;
  }
</style>
