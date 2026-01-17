<script lang="ts">
  import { onMount } from 'svelte';
  import { activeTheme, PanelSurface, ui } from 'lite2d-editor-ui';
  import type { MenuItem, PanelRegistration } from 'lite2d-editor-ui';

  import Moc3DrawablesPanel from '$lib/panels/Moc3DrawablesPanel.svelte';
  import Moc3FacePartValidatorPanel from '$lib/panels/Moc3FacePartValidatorPanel.svelte';
  import Moc3FilePanel from '$lib/panels/Moc3FilePanel.svelte';
  import Moc3RenderPreviewPanel from '$lib/panels/Moc3RenderPreviewPanel.svelte';
  import Moc3UvPreviewPanel from '$lib/panels/Moc3UvPreviewPanel.svelte';
  import { moc3Actions } from '$lib/moc3/store';


  let themeName: 'dark' | 'light' = 'dark';
  let fileInput: HTMLInputElement | null = null;
  let textureInput: HTMLInputElement | null = null;
  let renderSettingsInput: HTMLInputElement | null = null;
  let facePartsInput: HTMLInputElement | null = null;

  const sampleUrl = new URL('../live2d-assets/mao_pro/mao_pro.moc3.json', import.meta.url).href;
  const sampleTextureUrl = new URL('../live2d-assets/mao_pro/kei_basic_free.2048/texture_00.png', import.meta.url).href;

  const panels: PanelRegistration[] = [
    { id: 'moc3-file', title: 'MOC3 File', component: Moc3FilePanel, minWidth: 360, minHeight: 240 },
    { id: 'moc3-drawables', title: 'Drawables', component: Moc3DrawablesPanel, minWidth: 360, minHeight: 320 },
    { id: 'moc3-face-parts', title: 'Parts Index', component: Moc3FacePartValidatorPanel, minWidth: 420, minHeight: 320 },
    { id: 'moc3-render', title: 'Render Preview', component: Moc3RenderPreviewPanel, minWidth: 360, minHeight: 320 },
    { id: 'moc3-uv', title: 'UV Preview', component: Moc3UvPreviewPanel, minWidth: 420, minHeight: 360 }
  ];

  const windowMenuItems = (): MenuItem[] =>
    panels.map((panel, order) => ({ id: panel.id, label: panel.title, order, onSelect: () => ui.openPanel(panel.id) }));

  const setupLayout = () => {
    const nextTheme = themeName;

    ui.reset();
    ui.setTheme(nextTheme);
    panels.forEach((panel) => ui.registerPanel(panel));

    ui.registerMenu({
      id: 'file',
      label: 'File',
      order: 0,
      items: [
        { id: 'open-moc3', label: 'Open .moc3.json', onSelect: () => fileInput?.click() },
        // { id: 'load-sample', label: 'Load bundled sample', onSelect: () => loadSample() },
        { id: 'open-texture', label: 'Open texture image', onSelect: () => textureInput?.click() },
        { id: 'use-model-texture', label: 'Use model texture', onSelect: () => moc3Actions.tryLoadModelTexture() },
        { id: 'save-render-settings', label: 'Save render settings', onSelect: () => moc3Actions.exportRenderSettings() },
        { id: 'load-render-settings', label: 'Load render settings', onSelect: () => renderSettingsInput?.click() },
        { id: 'save-face-parts', label: 'Save parts', onSelect: () => moc3Actions.exportFaceParts() },
        { id: 'load-face-parts', label: 'Load parts', onSelect: () => facePartsInput?.click() }
      ]
    });
    ui.registerMenu({
      id: 'view',
      label: 'View',
      order: 1,
      items: [
        { id: 'theme-dark', label: 'Dark theme', onSelect: () => ui.setTheme('dark') },
        { id: 'theme-light', label: 'Switch to light mode', onSelect: () => ui.setTheme('light') },
        { id: 'reset-layout', label: 'Reset layout', onSelect: () => setupLayout() }
      ]
    });
    ui.registerMenu({ id: 'window', label: 'Window', order: 2, items: windowMenuItems() });

    ui.dockPanel('moc3-render', 'center');
    ui.dockPanel('moc3-drawables', 'left');
    ui.dockPanel('moc3-face-parts', 'left');
    ui.dockPanel('moc3-uv', 'right');
    ui.dockPanel('moc3-file', 'top');
  };

  onMount(() => {
    setupLayout();
  });

  $: themeName = $activeTheme.name as 'dark' | 'light';

  const handleFileChange = async (event: Event) => {
    const input = event.target as HTMLInputElement;
    const files = input.files;
    if (!files?.length) return;

    const mocFile = Array.from(files).find((f) => f.name.endsWith('.moc3.json') || f.name.endsWith('.json'));
    if (mocFile) {
      const text = await mocFile.text();
      moc3Actions.loadFromText(text, mocFile.name);
      moc3Actions.loadTextureFromFileList(files);
    }
    input.value = '';
  };

  const handleTextureFile = async (event: Event) => {
    const input = event.target as HTMLInputElement;
    const file = input.files?.[0];
    if (!file) return;
    moc3Actions.setTextureFromFile(file);
    input.value = '';
  };

  const handleRenderSettingsFile = async (event: Event) => {
    const input = event.target as HTMLInputElement;
    const file = input.files?.[0];
    if (!file) return;
    await moc3Actions.importRenderSettingsFromFile(file);
    input.value = '';
  };

  const handleFacePartsFile = async (event: Event) => {
    const input = event.target as HTMLInputElement;
    const file = input.files?.[0];
    if (!file) return;
    await moc3Actions.importFacePartsFromFile(file);
    input.value = '';
  };


  const loadSample = async () => {
    try {
      const response = await fetch(sampleUrl);
      const text = await response.text();
      moc3Actions.loadFromText(text, 'mao_pro.moc3.json', sampleUrl);
      moc3Actions.setTextureFromUrl(sampleTextureUrl, 'texture_00.png');
    } catch (error) {
      console.error(error);
    }
  };

</script>

<svelte:head>
  <title>Lite2D Editor</title>
</svelte:head>

<main class="page">
  <input class="hidden" type="file" multiple accept=".json,.moc3.json" bind:this={fileInput} onchange={handleFileChange} />
  <input class="hidden" type="file" accept="image/*" bind:this={textureInput} onchange={handleTextureFile} />
  <input class="hidden" type="file" accept="application/json,.json" bind:this={renderSettingsInput} onchange={handleRenderSettingsFile} />
  <input class="hidden" type="file" accept="application/json,.json" bind:this={facePartsInput} onchange={handleFacePartsFile} />
  <section class="surface-wrapper">
    <PanelSurface />
  </section>
</main>

<style>
  :global(body) {
    margin: 0;
    background: var(--ui-bg);
    color: var(--ui-text);
    font-family: var(--ui-font);
  }

  :global(*) {
    scrollbar-width: thin;
    scrollbar-color: var(--ui-border) transparent;
  }

  :global(*::-webkit-scrollbar) {
    width: 10px;
    height: 10px;
  }

  :global(*::-webkit-scrollbar-track) {
    background: transparent;
  }

  :global(*::-webkit-scrollbar-thumb) {
    background: color-mix(in srgb, var(--ui-border) 80%, transparent);
    border-radius: 999px;
    border: 2px solid transparent;
    background-clip: content-box;
  }

  :global(*::-webkit-scrollbar-thumb:hover) {
    background: color-mix(in srgb, var(--ui-primary) 60%, var(--ui-border));
    background-clip: content-box;
  }

  .page {
    display: flex;
    flex-direction: column;
    gap: 12px;
    position: fixed;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    min-height: 100vh;
    box-sizing: border-box;
    background: var(--ui-bg);
  }

  .surface-wrapper {
    flex: 1;
    min-height: 0;
    height: 100%;
    border-radius: var(--ui-radius);
    overflow: hidden;
    border: var(--ui-border-width) solid var(--ui-border);
    box-shadow: 0 18px 40px rgba(0, 0, 0, 0.2);
  }

  .hidden {
    display: none;
  }
</style>
