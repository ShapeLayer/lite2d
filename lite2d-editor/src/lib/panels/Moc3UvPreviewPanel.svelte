<script lang="ts">
  import { Select } from 'lite2d-editor-ui';
  import { ScanEye, ZoomIn, ZoomOut } from '@lucide/svelte';
  import earcut from 'earcut';
  import { moc3Actions, moc3State } from '../moc3/store';
  import { formatUvPreview, type Moc3Drawable } from '../moc3/parse';

  const canvasSize = 512;
  let canvasEl: HTMLCanvasElement | null = null;
  let textureInput: HTMLInputElement | null = null;

  let selectedMesh: Moc3Drawable | null = null;
  let selectedMeshId = '';
  let uvPreview: { index: number; u: number; v: number }[] = [];
  let uvTriangles: [number, number][][] = [];
  let uvBounds: { minU: number; maxU: number; minV: number; maxV: number } | null = null;

  let zoom = 1;
  const minZoom = 0.35;
  const maxZoom = 4;
  let panX = 0;
  let panY = 0;
  let isPanning = false;
  let pointerId: number | null = null;
  let lastPointer: { x: number; y: number } | null = null;
  let isWheelDrag = false;

  const clampZoom = (value: number) => Math.min(maxZoom, Math.max(minZoom, value));

  const uvToCanvas = ([u, v]: [number, number]) => ({
    x: u * canvasSize,
    y: (1 - v) * canvasSize
  });

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

  const pointInTriangle = (p: [number, number], a: [number, number], b: [number, number], c: [number, number]) => {
    const sign = (p1: [number, number], p2: [number, number], p3: [number, number]) =>
      (p1[0] - p3[0]) * (p2[1] - p3[1]) - (p2[0] - p3[0]) * (p1[1] - p3[1]);
    const d1 = sign(p, a, b);
    const d2 = sign(p, b, c);
    const d3 = sign(p, c, a);
    const hasNeg = d1 < 0 || d2 < 0 || d3 < 0;
    const hasPos = d1 > 0 || d2 > 0 || d3 > 0;
    return !(hasNeg && hasPos);
  };

  const triArea = (a: [number, number], b: [number, number], c: [number, number]) =>
    Math.abs((b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0])) * 0.5;

  const handlePointerDown = (event: PointerEvent) => {
    if (event.button === 1) {
      event.preventDefault();
      isPanning = true;
      isWheelDrag = true;
      pointerId = event.pointerId;
      lastPointer = { x: event.clientX, y: event.clientY };
      canvasEl?.setPointerCapture(event.pointerId);
      return;
    }
    if (event.button !== 0) return;
    if (!canvasEl) return;
    const rect = canvasEl.getBoundingClientRect();
    const screenX = event.clientX - rect.left;
    const screenY = event.clientY - rect.top;
    const c = canvasSize / 2;
    const canvasX = (screenX - c - panX) / zoom + c;
    const canvasY = (screenY - c - panY) / zoom + c;
    const u = canvasX / canvasSize;
    const v = 1 - canvasY / canvasSize;

    const drawables = $moc3State.model?.drawables ?? [];
    let best: Moc3Drawable | null = null;
    let bestArea = Infinity;
    for (const drawable of drawables) {
      if (!drawable.uvs?.length) continue;
      const triangles = buildTriangles(drawable.uvs);
      if (!triangles.length) continue;
      for (const tri of triangles) {
        if (tri.length !== 3) continue;
        if (!pointInTriangle([u, v], tri[0], tri[1], tri[2])) continue;
        const area = triArea(tri[0], tri[1], tri[2]);
        if (area < bestArea) {
          bestArea = area;
          best = drawable;
        }
      }
    }

    if (best) {
      moc3Actions.selectMesh(best.id);
    }
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
    isWheelDrag = false;
    pointerId = null;
    lastPointer = null;
  };

  const focusOnBounds = (bounds: { minU: number; maxU: number; minV: number; maxV: number }) => {
    const minX = bounds.minU * canvasSize;
    const maxX = bounds.maxU * canvasSize;
    const minY = (1 - bounds.maxV) * canvasSize;
    const maxY = (1 - bounds.minV) * canvasSize;
    const width = Math.max(1, maxX - minX);
    const height = Math.max(1, maxY - minY);
    const pad = 0.85;
    const targetZoom = clampZoom((canvasSize * pad) / Math.max(width, height));
    zoom = targetZoom;
    const centerX = (minX + maxX) * 0.5;
    const centerY = (minY + maxY) * 0.5;
    const c = canvasSize / 2;
    panX = -(centerX - c) * zoom;
    panY = -(centerY - c) * zoom;
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

    if ($moc3State.textureImage) {
      ctx.drawImage($moc3State.textureImage, 0, 0, canvasSize, canvasSize);
    }

    if (!selectedMesh) {
      ctx.restore();
      return;
    }

    ctx.fillStyle = 'rgba(255,76,164,0.9)';
    const pointRadius = 2.5 / zoom;
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

  const handleMeshSelectChange = (event: CustomEvent<string>) => {
    moc3Actions.selectMesh(event.detail ?? $moc3State.selectedMeshId ?? '');
  };

  const handleTextureFile = async (event: Event) => {
    const input = event.target as HTMLInputElement;
    const file = input.files?.[0];
    if (!file) return;
    moc3Actions.setTextureFromFile(file);
    input.value = '';
  };

  $: selectedMeshId = $moc3State.selectedMeshId;
  $: selectedMesh = $moc3State.model?.drawables.find((mesh) => mesh.id === selectedMeshId) ?? null;
  $: uvPreview = selectedMesh ? formatUvPreview(selectedMesh.uvs, 28) : [];
  $: uvTriangles = selectedMesh ? buildTriangles(selectedMesh.uvs) : [];
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


  $: previewTrigger = `${selectedMeshId}-${$moc3State.textureUrl}-${uvTriangles.length}-${$moc3State.textureImage ? 1 : 0}`;
  $: viewStateTrigger = `${previewTrigger}-${zoom.toFixed(3)}-${panX.toFixed(1)}-${panY.toFixed(1)}`;
  $: if (canvasEl) {
    viewStateTrigger;
    renderPreview();
  }
</script>

<div class="panel">
  {#if $moc3State.model}
    <div class="uv-header">
      <div>
        <div class="label">UV Preview</div>
        <div class="value">{selectedMesh?.id ?? 'Select a mesh'}</div>
      </div>
      <Select
        label="Mesh"
        options={$moc3State.model.drawables.map((mesh) => ({ label: mesh.id, value: mesh.id }))}
        value={selectedMeshId}
        on:change={handleMeshSelectChange}
      />
    </div>

    <div class="texture-bar">
      <div>
        <div class="label">Texture source</div>
        <div class="value">{$moc3State.textureName || 'Select texture image'}</div>
        <div class="status muted">{$moc3State.textureStatus}</div>
      </div>
      <div class="texture-actions">
        <button type="button" class="ui-btn ghost" onclick={() => textureInput?.click()}>
          Open texture
        </button>
        <button type="button" class="ui-btn ghost" onclick={moc3Actions.tryLoadModelTexture}>
          Reload model texture
        </button>
      </div>
      <input class="hidden" type="file" accept="image/*" bind:this={textureInput} onchange={handleTextureFile} />
    </div>

    <div class="view-controls">
      <div class="zoom-row">
        <span class="zoom-label">Zoom</span>
        <input type="range" min={minZoom} max={maxZoom} step="0.05" value={zoom} oninput={handleZoomInput} />
        <span class="zoom-value">{(zoom * 100).toFixed(0)}%</span>
        <div class="zoom-icons">
          <button type="button" class="icon-btn" onclick={zoomOut} title="Zoom out" aria-label="Zoom out">
            <ZoomOut size={16} />
          </button>
          <button type="button" class="icon-btn" onclick={zoomIn} title="Zoom in" aria-label="Zoom in">
            <ZoomIn size={16} />
          </button>
          <button type="button" class="icon-btn" onclick={resetView} title="Reset zoom" aria-label="Reset zoom">
            <ScanEye size={16} />
          </button>
        </div>
      </div>
      <div class="view-hint">Middle-click drag to pan, wheel to zoom, left-click to select.</div>
    </div>

    <div class="uv-canvas-wrapper">
      <canvas
        bind:this={canvasEl}
        width={canvasSize}
        height={canvasSize}
        tabindex="0"
        onwheel={handleCanvasWheel}
        onpointerdown={handlePointerDown}
        onpointermove={handlePointerMove}
        onpointerup={handlePointerUp}
        onpointerleave={handlePointerUp}
        class:grabbing={isWheelDrag}
      ></canvas>
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
  {:else}
    <div class="placeholder">No moc3 file loaded. Load a file to inspect UVs.</div>
  {/if}
</div>

<style>
  .panel {
    display: flex;
    flex-direction: column;
    gap: 12px;
  }

  .label {
    color: var(--ui-text-muted);
    font-size: 12px;
  }

  .value {
    color: var(--ui-text);
    font-weight: 700;
  }

  .status {
    color: var(--ui-text-muted);
    font-size: 13px;
  }

  .status.muted {
    color: var(--ui-text-muted);
    font-size: 12px;
  }

  .uv-header {
    display: flex;
    align-items: flex-end;
    justify-content: space-between;
    gap: 12px;
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

  .view-controls {
    display: grid;
    gap: 6px;
    padding: 8px 10px;
    border-radius: var(--ui-radius);
    border: var(--ui-border-width) solid var(--ui-border);
    background: color-mix(in srgb, var(--ui-muted) 70%, transparent);
  }

  .zoom-row {
    display: grid;
    grid-template-columns: auto 1fr auto auto;
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

  .zoom-icons {
    display: flex;
    gap: 6px;
  }

  .icon-btn {
    border: var(--ui-border-width) solid var(--ui-border);
    background: transparent;
    color: var(--ui-text);
    width: 28px;
    height: 28px;
    border-radius: 8px;
    display: inline-flex;
    align-items: center;
    justify-content: center;
    cursor: pointer;
  }

  .icon-btn:hover {
    background: var(--ui-highlight);
    border-color: color-mix(in srgb, var(--ui-primary) 60%, var(--ui-border));
  }

  .view-hint {
    font-size: 12px;
    color: var(--ui-text-muted);
  }

  .uv-canvas-wrapper {
    display: flex;
    flex-direction: column;
    gap: 8px;
    flex: 1;
    min-height: 0;
  }

  canvas {
    border: var(--ui-border-width) solid var(--ui-border);
    border-radius: var(--ui-radius);
    background: color-mix(in srgb, var(--ui-surface) 96%, transparent);
    width: 100%;
    height: auto;
    max-height: 100%;
    aspect-ratio: 1 / 1;
  }

  canvas.grabbing {
    cursor: grabbing;
  }

  .legend {
    display: grid;
    gap: 4px;
    font-size: 12px;
    color: var(--ui-text-muted);
  }

  .legend-note {
    color: var(--ui-text-muted);
    font-size: 12px;
  }

  .uv-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(140px, 1fr));
    gap: 6px;
    max-height: 140px;
    overflow: auto;
    border: var(--ui-border-width) solid var(--ui-border);
    border-radius: var(--ui-radius);
    padding: 6px;
    background: color-mix(in srgb, var(--ui-surface) 94%, transparent);
  }

  .uv-cell {
    border: var(--ui-border-width) solid var(--ui-border);
    border-radius: var(--ui-radius);
    padding: 6px 8px;
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

  .hidden {
    display: none;
  }

  .placeholder {
    color: var(--ui-text-muted);
    padding: 12px;
    border-radius: var(--ui-radius);
    background: color-mix(in srgb, var(--ui-surface) 92%, transparent);
  }
</style>
