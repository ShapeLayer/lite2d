<script lang="ts">
  import { Button, Select, TextInput } from 'lite2d-editor-ui';
  import earcut from 'earcut';
  import { formatUvPreview, parseMoc3, type Moc3Drawable, type Moc3Model } from '$lib/moc3/parse';

  const sampleUrl = new URL('../../live2d-assets/mao_pro/mao_pro.moc3.json', import.meta.url).href;
  const sampleTextureUrl = new URL('../../live2d-assets/mao_pro/kei_basic_free.2048/texture_00.png', import.meta.url).href;
  const canvasSize = 512;

  let fileInput: HTMLInputElement | null = null;
  let textureInput: HTMLInputElement | null = null;
  let canvasEl: HTMLCanvasElement | null = null;
  let textureImage: HTMLImageElement | null = null;
  let model: Moc3Model | null = null;
  let selectedMeshId = '';
  let selectedMesh: Moc3Drawable | null = null;
  let uvPreview: { index: number; u: number; v: number }[] = [];
  let errorMessage = '';
  let statusMessage = 'Drop a .moc3.json file or load the bundled sample.';
  let meshFilter = '';
  let textureUrl = '';
  let textureName = '';
  let textureStatus = 'No texture loaded';
  let uvBounds: { minU: number; maxU: number; minV: number; maxV: number } | null = null;
  let uvTriangles: [number, number][][] = [];
  let previewTrigger = '';
  let viewStateTrigger = '';
  let lastModelUrl: string | null = null;
  let zoom = 1;
  const minZoom = 0.35;
  const maxZoom = 4;
  let panX = 0;
  let panY = 0;
  let isPanning = false;
  let pointerId: number | null = null;
  let lastPointer: { x: number; y: number } | null = null;

  const loadFromText = (text: string, name: string, sourceUrl?: string) => {
    try {
      const json = JSON.parse(text);
      model = parseMoc3(json, name);
      selectedMeshId = model.drawables[0]?.id ?? '';
      errorMessage = '';
      statusMessage = `${name} loaded (${model.drawables.length} drawables)`;
      textureName = '';
      lastModelUrl = sourceUrl ?? null;
      tryLoadModelTexture();
    } catch (error) {
      console.error(error);
      errorMessage = 'Failed to parse moc3 JSON. Check file format.';
      model = null;
      selectedMeshId = '';
      textureStatus = 'No texture loaded';
      lastModelUrl = null;
    }
  };

  const handleFileChange = async (event: Event) => {
    const input = event.target as HTMLInputElement;
    const files = input.files;
    if (!files?.length) return;

    const mocFile = Array.from(files).find((f) => f.name.endsWith('.moc3.json') || f.name.endsWith('.json'));
    if (mocFile) {
      const text = await mocFile.text();
      loadFromText(text, mocFile.name);
      // Try to load texture from the same selection
      loadTextureFromFileList(files);
    }
    input.value = '';
  };

  const handleDrop = async (event: DragEvent) => {
    event.preventDefault();
    const files = event.dataTransfer?.files;
    if (!files?.length) return;

    const mocFile = Array.from(files).find((f) => f.name.endsWith('.moc3.json') || f.name.endsWith('.json'));
    if (mocFile) {
      const text = await mocFile.text();
      loadFromText(text, mocFile.name);
    }

    loadTextureFromFileList(files);
  };

  const loadSample = async () => {
    try {
      const response = await fetch(sampleUrl);
      const text = await response.text();
      loadFromText(text, 'mao_pro.moc3.json', sampleUrl);
      setTexture(sampleTextureUrl, 'texture_00.png');
    } catch (error) {
      console.error(error);
      errorMessage = 'Sample asset could not be loaded. Check dev server static files.';
    }
  };

  const meshRows = () => {
    if (!model) return [];
    return model.drawables
      .filter((mesh) => mesh.id.toLowerCase().includes(meshFilter.trim().toLowerCase()))
      .map((mesh) => ({
        id: mesh.id,
        vertices: mesh.vertexCount,
        uvs: mesh.uvs.length,
        draw: mesh.textureIndex
      }));
  };

  const selectMesh = (id: string) => {
    selectedMeshId = id;
    meshFilter = id;
  };

  const setTexture = (url: string, name?: string) => {
    if (!url) return;
    const img = new Image();
    img.crossOrigin = 'anonymous';
    img.onload = () => {
      textureImage = img;
      textureUrl = url;
      textureName = name ?? url;
      textureStatus = `Texture loaded (${img.width}x${img.height})`;
      renderPreview();
    };
    img.onerror = () => {
      textureStatus = 'Failed to load texture. Select manually.';
    };
    img.src = url;
  };

  const tryLoadModelTexture = () => {
    if (!model?.texture) {
      textureStatus = 'No texture declared in moc3';
      return;
    }
    // If the moc3 came from a URL, try resolving relative to it; otherwise fall back to bundled assets.
    if (lastModelUrl) {
      try {
        const base = lastModelUrl.substring(0, lastModelUrl.lastIndexOf('/') + 1);
        setTexture(new URL(model.texture, base).href, model.texture);
        return;
      } catch (error) {
        console.warn('Relative texture resolution failed', error);
      }
    }

    // Bundled sample fallback
    const assumed = new URL(`../../live2d-assets/mao_pro/${model.texture}`, import.meta.url).href;
    setTexture(assumed, model.texture);
  };

  const handleTextureFile = async (event: Event) => {
    const input = event.target as HTMLInputElement;
    const file = input.files?.[0];
    if (!file) return;
    const url = URL.createObjectURL(file);
    setTexture(url, file.name);
    input.value = '';
  };

  const loadTextureFromFileList = (files: FileList | File[]) => {
    const fileArray = Array.from(files || []);
    if (!fileArray.length) return false;

    let textureFile: File | undefined;
    if (model?.texture) {
      textureFile = fileArray.find((f) => f.name === model.texture);
    }
    if (!textureFile) {
      textureFile = fileArray.find((f) => f.type.startsWith('image/'));
    }
    if (!textureFile) return false;

    const url = URL.createObjectURL(textureFile);
    setTexture(url, textureFile.name);
    return true;
  };
  
  const buildTriangles = (uvs: [number, number][]) => {
    const coords: number[] = [];
    const points: [number, number][] = [];
    for (const [u, v] of uvs) {
      if (!Number.isFinite(u) || !Number.isFinite(v)) continue;
      coords.push(u, v);
      points.push([u, v]);
    }
    if (points.length < 3) return [];
    const indices = earcut(coords);
    const triangles: [number, number][][] = [];
    for (let i = 0; i < indices.length; i += 3) {
      const a = points[indices[i]];
      const b = points[indices[i + 1]];
      const c = points[indices[i + 2]];
      if (!a || !b || !c) continue;
      triangles.push([a, b, c]);
    }
    return triangles;
  };

  const clampZoom = (value: number) => Math.min(maxZoom, Math.max(minZoom, value));

  const uvToCanvas = ([u, v]: [number, number]) => ({
    x: u * canvasSize,
    // Live2D UV origin is top-left; canvas origin is also top-left, but our preview overlays the texture drawn top-down.
    // To match the exported UV layout, flip V.
    y: (1 - v) * canvasSize
  });

  const updateZoom = (target: number, center?: { x: number; y: number }) => {
    const clamped = clampZoom(target);
    if (center && canvasEl) {
      const rect = canvasEl.getBoundingClientRect();
      const offsetX = center.x - rect.left - canvasSize / 2 - panX;
      const offsetY = center.y - rect.top - canvasSize / 2 - panY;
      const factor = clamped / zoom;
      panX -= offsetX * (factor - 1);
      panY -= offsetY * (factor - 1);
    }
    zoom = clamped;
  };

  const zoomIn = () => updateZoom(zoom * 1.25);
  const zoomOut = () => updateZoom(zoom / 1.25);
  const resetView = () => {
    zoom = 1;
    panX = 0;
    panY = 0;
  };

  const handleZoomInput = (event: Event) => {
    const value = Number((event.target as HTMLInputElement).value);
    updateZoom(value);
  };

  const handleCanvasWheel = (event: WheelEvent) => {
    event.preventDefault();
    const factor = event.deltaY < 0 ? 1.15 : 0.85;
    updateZoom(zoom * factor, { x: event.clientX, y: event.clientY });
  };

  const handlePointerDown = (event: PointerEvent) => {
    isPanning = true;
    pointerId = event.pointerId;
    lastPointer = { x: event.clientX, y: event.clientY };
    canvasEl?.setPointerCapture(event.pointerId);
  };

  const handlePointerMove = (event: PointerEvent) => {
    if (!isPanning || event.pointerId !== pointerId || !lastPointer) return;
    const dx = event.clientX - lastPointer.x;
    const dy = event.clientY - lastPointer.y;
    panX += dx;
    panY += dy;
    lastPointer = { x: event.clientX, y: event.clientY };
  };

  const handlePointerUp = (event: PointerEvent) => {
    if (pointerId === event.pointerId) {
      canvasEl?.releasePointerCapture(event.pointerId);
    }
    isPanning = false;
    pointerId = null;
    lastPointer = null;
  };

  const renderPreview = () => {
    if (!canvasEl) return;
    const ctx = canvasEl.getContext('2d');
    if (!ctx) return;

    ctx.clearRect(0, 0, canvasSize, canvasSize);

    ctx.fillStyle = '#0f172a';
    ctx.fillRect(0, 0, canvasSize, canvasSize);

    ctx.save();
    ctx.translate(canvasSize / 2 + panX, canvasSize / 2 + panY);
    ctx.scale(zoom, zoom);
    ctx.translate(-canvasSize / 2, -canvasSize / 2);

    ctx.strokeStyle = 'rgba(255,255,255,0.08)';
    ctx.lineWidth = 1;
    for (let i = 0; i <= canvasSize; i += canvasSize / 8) {
      ctx.beginPath();
      ctx.moveTo(i, 0);
      ctx.lineTo(i, canvasSize);
      ctx.stroke();

      ctx.beginPath();
      ctx.moveTo(0, i);
      ctx.lineTo(canvasSize, i);
      ctx.stroke();
    }

    if (textureImage) {
      ctx.drawImage(textureImage, 0, 0, canvasSize, canvasSize);
    }

    if (!selectedMesh) {
      ctx.restore();
      return;
    }

    ctx.fillStyle = 'rgba(255,76,164,0.9)';
    const pointRadius = 2.5 / zoom; // keep screen-space size constant while zooming
    selectedMesh.uvs.forEach((uv) => {
      const { x, y } = uvToCanvas(uv);
      ctx.beginPath();
      ctx.arc(x, y, pointRadius, 0, Math.PI * 2);
      ctx.fill();
    });

    if (selectedMesh.uvs.length) {
      const xs = selectedMesh.uvs.map((uv) => uvToCanvas(uv).x);
      const ys = selectedMesh.uvs.map((uv) => uvToCanvas(uv).y);
      const minU = Math.min(...xs);
      const maxU = Math.max(...xs);
      const minV = Math.min(...ys);
      const maxV = Math.max(...ys);

      ctx.strokeStyle = 'rgba(88,166,255,0.9)';
      ctx.lineWidth = 2;
      ctx.strokeRect(minU, minV, maxU - minU, maxV - minV);
    }

    ctx.strokeStyle = 'rgba(88,166,255,0.5)';
    ctx.lineWidth = 0.8;
    uvTriangles.forEach((triangle) => {
      if (triangle.length !== 3) return;
      ctx.beginPath();
      triangle.forEach((uv, index) => {
        const { x, y } = uvToCanvas(uv);
        if (index === 0) {
          ctx.moveTo(x, y);
        } else {
          ctx.lineTo(x, y);
        }
      });
      ctx.closePath();
      ctx.stroke();
    });

    ctx.restore();
  };

  $: selectedMeshId = model && !selectedMeshId && model.drawables.length ? model.drawables[0].id : selectedMeshId;
  $: selectedMesh = model?.drawables.find((mesh) => mesh.id === selectedMeshId) ?? null;
  $: uvPreview = selectedMesh ? formatUvPreview(selectedMesh.uvs, 28) : [];
  $: uvTriangles = selectedMesh ? buildTriangles(selectedMesh.uvs) : [];
  $: previewTrigger = `${selectedMeshId}-${textureUrl}-${uvTriangles.length}-${textureImage ? 1 : 0}`;
  $: viewStateTrigger = `${previewTrigger}-${zoom.toFixed(3)}-${panX.toFixed(1)}-${panY.toFixed(1)}`;
  $: if (canvasEl) {
    viewStateTrigger;
    renderPreview();
  }

  $: uvBounds = selectedMesh?.uvs.length
    ? selectedMesh.uvs.reduce(
        (acc, [u, v]) => ({
          minU: Math.min(acc.minU, u),
          maxU: Math.max(acc.maxU, u),
          minV: Math.min(acc.minV, v),
          maxV: Math.max(acc.maxV, v)
        }),
        { minU: Infinity, maxU: -Infinity, minV: Infinity, maxV: -Infinity }
      )
    : null;

  $: totalVertices = model ? model.drawables.reduce((sum, d) => sum + d.vertexCount, 0) : 0;
  $: totalUvs = model ? model.drawables.reduce((sum, d) => sum + d.uvs.length, 0) : 0;
</script>

<div class="panel">
  <div class="toolbar">
    <div class="actions">
      <Button variant="primary" label="Open .moc3.json" on:click={() => fileInput?.click()} />
      <Button variant="ghost" label="Load bundled sample" on:click={loadSample} />
      <Button variant="ghost" label="Open texture image" on:click={() => textureInput?.click()} />
      <Button variant="ghost" label="Use model texture" on:click={tryLoadModelTexture} />
    </div>
    <div class="status">{errorMessage ? errorMessage : statusMessage}</div>
    <input class="hidden" type="file" multiple accept=".json,.moc3.json" bind:this={fileInput} on:change={handleFileChange} />
    <input class="hidden" type="file" accept="image/*" bind:this={textureInput} on:change={handleTextureFile} />
  </div>

  <div
    class="dropzone"
    on:dragover|preventDefault
    on:drop={handleDrop}
    aria-label="Drop moc3 file"
  >
    <div>
      <p>Drop a Live2D .moc3.json file here</p>
      <p class="hint">UVs and mesh stats will be extracted client-side.</p>
    </div>
  </div>

  {#if model}
    <div class="meta">
      <div class="meta-card">
        <div class="label">File</div>
        <div class="value">{model.name}</div>
        <div class="label">Texture</div>
        <div class="value">{textureName || 'N/A'}</div>
        <div class="status muted">{textureStatus}</div>
      </div>
      <div class="meta-card">
        <div class="label">Canvas</div>
        <div class="value">{model.canvas.width.toFixed(3)} × {model.canvas.height.toFixed(3)}</div>
        <div class="label">Pixels / Unit</div>
        <div class="value">{model.canvas.pixelsPerUnit.toFixed(2)}</div>
      </div>
      <div class="meta-card">
        <div class="label">Drawables</div>
        <div class="value">{model.drawables.length}</div>
        <div class="label">Totals (verts / uvs)</div>
        <div class="value">{totalVertices} / {totalUvs}</div>
      </div>
    </div>

    <div class="content">
      <div class="table-wrapper">
        <div class="table-header">
          <div>
            <div class="label">Drawables</div>
            <div class="value">Click a row to preview UVs</div>
          </div>
          <TextInput
            label="Filter by id"
            bind:value={meshFilter}
            placeholder="Type id to focus"
            on:input={(event) => {
              const value = (event.target as HTMLInputElement).value;
              meshFilter = value;
              const first = model?.drawables.find((mesh) => mesh.id.toLowerCase().includes(value.toLowerCase()));
              if (first) selectMesh(first.id);
            }}
          />
        </div>
        <div class="mesh-table">
          <div class="mesh-header">
            <span>ID</span>
            <span>Vertices</span>
            <span>UVs</span>
            <span>Tex</span>
          </div>
          <div class="mesh-body">
            {#each meshRows() as row}
              <button class:selected={row.id === selectedMeshId} on:click={() => selectMesh(row.id)}>
                <span title={row.id}>{row.id}</span>
                <span>{row.vertices}</span>
                <span>{row.uvs}</span>
                <span>{row.draw}</span>
              </button>
            {/each}
          </div>
        </div>
      </div>

      <div class="uv-panel">
        <div class="uv-header">
          <div>
            <div class="label">UV Preview</div>
            <div class="value">{selectedMesh?.id ?? 'Select a mesh'}</div>
          </div>
          <Select
            label="Mesh"
            options={(model.drawables.map((mesh) => ({ label: mesh.id, value: mesh.id })))}
            bind:value={selectedMeshId}
            on:change={(event) => selectMesh((event as CustomEvent<string>).detail ?? selectedMeshId ?? '')}
          />
        </div>

        <div class="texture-bar">
          <div>
            <div class="label">Texture source</div>
            <div class="value">{textureName || 'Select texture image'}</div>
            <div class="status muted">{textureStatus}</div>
          </div>
          <div class="texture-actions">
            <Button variant="ghost" label="Open texture" on:click={() => textureInput?.click()} />
            <Button variant="ghost" label="Reload model texture" on:click={tryLoadModelTexture} />
          </div>
        </div>

        <div class="view-controls">
          <div class="zoom-row">
            <span class="zoom-label">Zoom</span>
            <input type="range" min={minZoom} max={maxZoom} step="0.05" value={zoom} on:input={handleZoomInput} />
            <span class="zoom-value">{(zoom * 100).toFixed(0)}%</span>
          </div>
          <div class="view-buttons">
            <Button variant="ghost" label="Zoom In" on:click={zoomIn} />
            <Button variant="ghost" label="Zoom Out" on:click={zoomOut} />
            <Button variant="ghost" label="Reset View" on:click={resetView} />
          </div>
          <div class="view-hint">Drag canvas to pan and wheel to zoom around the cursor.</div>
        </div>

        <div class="uv-canvas-wrapper">
          <canvas
            bind:this={canvasEl}
            width={canvasSize}
            height={canvasSize}
            tabindex="0"
            on:wheel={handleCanvasWheel}
            on:pointerdown={handlePointerDown}
            on:pointermove={handlePointerMove}
            on:pointerup={handlePointerUp}
            on:pointerleave={handlePointerUp}
            class:grabbing={isPanning}
          />
          <div class="legend">
            {#if uvBounds}
              <div>U: {uvBounds.minU.toFixed(3)} – {uvBounds.maxU.toFixed(3)}</div>
              <div>V: {uvBounds.minV.toFixed(3)} – {uvBounds.maxV.toFixed(3)}</div>
            {:else}
              <div>No UV data</div>
            {/if}
            <div class="legend-note">Pink: UV points, Blue: bounding box</div>
          </div>
        </div>

        {#if selectedMesh}
          <div class="uv-grid">
            {#each uvPreview as uv}
              <div class="uv-cell">
                <div class="uv-index">#{uv.index}</div>
                <div class="uv-values">{uv.u.toFixed(4)}, {uv.v.toFixed(4)}</div>
              </div>
            {/each}
          </div>
        {:else}
          <div class="placeholder">Select a mesh to view UV coordinates.</div>
        {/if}
      </div>
    </div>
  {:else}
    <div class="placeholder">No moc3 file loaded. Use the buttons above to start.</div>
  {/if}
</div>

<style>
  .panel {
    display: flex;
    flex-direction: column;
    gap: 12px;
    height: 100%;
  }

  .toolbar {
    display: flex;
    align-items: center;
    gap: 12px;
    justify-content: space-between;
  }

  .actions {
    display: flex;
    gap: 10px;
  }

  .status {
    color: var(--ui-text-muted);
    font-size: 13px;
  }

  .status.muted {
    color: var(--ui-text-muted);
    font-size: 12px;
  }

  .hidden {
    display: none;
  }

  .dropzone {
    border: 1px dashed var(--ui-border);
    border-radius: var(--ui-radius);
    padding: 16px;
    background: color-mix(in srgb, var(--ui-surface) 92%, transparent);
    text-align: center;
    color: var(--ui-text);
  }

  .dropzone .hint {
    color: var(--ui-text-muted);
    margin: 4px 0 0 0;
    font-size: 12px;
  }

  .meta {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
    gap: 10px;
  }

  .meta-card {
    border: var(--ui-border-width) solid var(--ui-border);
    border-radius: var(--ui-radius);
    padding: 10px 12px;
    background: color-mix(in srgb, var(--ui-surface) 94%, transparent);
    display: grid;
    grid-template-columns: 1fr;
    gap: 4px;
  }

  .label {
    color: var(--ui-text-muted);
    font-size: 12px;
  }

  .value {
    color: var(--ui-text);
    font-weight: 700;
  }

  .content {
    display: grid;
    grid-template-columns: 1fr 1.5fr;
    gap: 12px;
    height: 100%;
    min-height: 520px;
  }

  .table-wrapper {
    display: flex;
    flex-direction: column;
    gap: 10px;
    border: var(--ui-border-width) solid var(--ui-border);
    border-radius: var(--ui-radius);
    padding: 10px;
    background: color-mix(in srgb, var(--ui-surface) 92%, transparent);
  }

  .table-header {
    display: flex;
    align-items: flex-end;
    justify-content: space-between;
    gap: 12px;
  }

  .mesh-table {
    display: flex;
    flex-direction: column;
    border: var(--ui-border-width) solid var(--ui-border);
    border-radius: var(--ui-radius);
    overflow: hidden;
  }

  .mesh-header {
    display: grid;
    grid-template-columns: 1fr 90px 70px 70px;
    padding: 8px 10px;
    background: var(--ui-muted);
    color: var(--ui-text);
    font-weight: 700;
    gap: 8px;
  }

  .mesh-body {
    max-height: 280px;
    overflow: auto;
  }

  .mesh-body button {
    width: 100%;
    display: grid;
    grid-template-columns: 1fr 90px 70px 70px;
    gap: 8px;
    padding: 8px 10px;
    border: none;
    background: color-mix(in srgb, var(--ui-surface) 94%, transparent);
    color: var(--ui-text);
    text-align: left;
    cursor: pointer;
    border-bottom: var(--ui-border-width) solid var(--ui-border);
  }

  .mesh-body button:hover {
    background: color-mix(in srgb, var(--ui-primary) 10%, var(--ui-surface) 92%);
  }

  .mesh-body button.selected {
    background: color-mix(in srgb, var(--ui-primary) 14%, var(--ui-surface) 90%);
    border-color: color-mix(in srgb, var(--ui-primary) 40%, var(--ui-border));
  }

  .uv-panel {
    border: var(--ui-border-width) solid var(--ui-border);
    border-radius: var(--ui-radius);
    padding: 10px;
    background: color-mix(in srgb, var(--ui-surface) 94%, transparent);
    display: flex;
    flex-direction: column;
    gap: 10px;
    min-height: 520px;
  }

  .uv-header {
    display: flex;
    align-items: flex-end;
    justify-content: space-between;
    gap: 12px;
  }

  .view-controls {
    display: grid;
    gap: 10px;
    padding: 8px 10px;
    border-radius: var(--ui-radius);
    border: var(--ui-border-width) solid var(--ui-border);
    background: color-mix(in srgb, var(--ui-muted) 70%, transparent);
  }

  .zoom-row {
    display: grid;
    grid-template-columns: auto 1fr auto;
    gap: 8px;
    align-items: center;
  }

  .zoom-label,
  .zoom-value {
    color: var(--ui-text);
    font-size: 12px;
  }

  .zoom-value {
    font-weight: 700;
  }

  .view-controls input[type='range'] {
    width: 100%;
    accent-color: var(--ui-primary);
  }

  .view-buttons {
    display: flex;
    gap: 8px;
    flex-wrap: wrap;
  }

  .view-hint {
    font-size: 12px;
    color: var(--ui-text-muted);
  }

  .texture-bar {
    display: flex;
    justify-content: space-between;
    align-items: center;
    gap: 12px;
    padding: 8px 10px;
    border: var(--ui-border-width) solid var(--ui-border);
    border-radius: var(--ui-radius);
    background: color-mix(in srgb, var(--ui-muted) 70%, transparent);
  }

  .texture-actions {
    display: flex;
    gap: 8px;
  }

  .uv-canvas-wrapper {
    display: grid;
    grid-template-columns: auto 1fr;
    gap: 12px;
    align-items: center;
  }

  canvas {
    border: var(--ui-border-width) solid var(--ui-border);
    border-radius: var(--ui-radius);
    background: #0f172a;
  }

  .legend {
    display: flex;
    flex-direction: column;
    gap: 6px;
    color: var(--ui-text);
    font-size: 13px;
  }

  .legend-note {
    color: var(--ui-text-muted);
    font-size: 12px;
  }

  .uv-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(140px, 1fr));
    gap: 8px;
  }

  .uv-cell {
    border: var(--ui-border-width) solid var(--ui-border);
    border-radius: var(--ui-radius);
    padding: 8px 10px;
    background: color-mix(in srgb, var(--ui-muted) 60%, transparent);
  }

  .uv-index {
    color: var(--ui-text-muted);
    font-size: 12px;
  }

  .uv-values {
    color: var(--ui-text);
    font-family: 'SFMono-Regular', ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, 'Liberation Mono', 'Courier New', monospace;
    font-size: 12px;
  }

  .placeholder {
    color: var(--ui-text-muted);
    padding: 12px;
    border-radius: var(--ui-radius);
    background: color-mix(in srgb, var(--ui-surface) 92%, transparent);
  }
</style>
