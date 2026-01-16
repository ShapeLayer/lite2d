<script lang="ts">
  import { Select, TextInput } from 'lite2d-editor-ui';
  import { CircleDot, Eye, EyeOff, GripHorizontal, Grid2x2, Image as ImageIcon } from '@lucide/svelte';
  import earcut from 'earcut';
  import { onDestroy, onMount, tick } from 'svelte';
  import RenderPreview from './RenderPreview.svelte';
  import { formatUvPreview, parseMoc3, type Moc3Drawable, type Moc3Model } from '../moc3/parse';

  const sampleUrl = new URL('../../live2d-assets/mao_pro/mao_pro.moc3.json', import.meta.url).href;
  const sampleTextureUrl = new URL('../../live2d-assets/mao_pro/kei_basic_free.2048/texture_00.png', import.meta.url).href;
  let canvasSize = 512;
  let dpr = 1;
  let previewRaf = 0;
  let uvResizeObserver: ResizeObserver | null = null;

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
  let renderOrder: string[] = [];
  let hiddenDrawables: Record<string, boolean> = {};
  let renderStatus = 'No render yet';
  let draggingId: string | null = null;
  let dragOverId: string | null = null;
  let rowElements: Record<string, HTMLDivElement> = {};

  const handleFilterInput = (event: Event) => {
    const value = (event.currentTarget as HTMLInputElement | null)?.value ?? '';
    meshFilter = value;
    const first = model?.drawables.find((mesh) => mesh.id.toLowerCase().includes(value.toLowerCase()));
    if (first) selectMesh(first.id);
  };

  const handleMeshSelectChange = (event: CustomEvent<string>) => {
    selectMesh(event.detail ?? selectedMeshId ?? '');
  };

  const registerRow = (node: HTMLDivElement, id: string) => {
    rowElements[id] = node;
    return {
      update(nextId: string) {
        if (nextId !== id) {
          delete rowElements[id];
          id = nextId;
          rowElements[id] = node;
        }
      },
      destroy() {
        delete rowElements[id];
      }
    };
  };

  const getSettingsKey = (name: string) => `lite2d-editor:renderSettings:${name}`;

  const applyRenderSettings = (nextModel: Moc3Model) => {
    const defaultOrder = nextModel.drawables.map((drawable) => drawable.id).reverse();
    let order = [...defaultOrder];
    let hidden: Record<string, boolean> = {};

    if (typeof localStorage === 'undefined') {
      renderOrder = order;
      hiddenDrawables = hidden;
      return;
    }

    try {
      const stored = localStorage.getItem(getSettingsKey(nextModel.name));
      if (stored) {
        const parsed = JSON.parse(stored) as { order?: string[]; hidden?: string[]; version?: number };
        if (Array.isArray(parsed.order)) {
          const storedOrder = parsed.order.filter((id) => defaultOrder.includes(id));
          order = parsed.version === 2 ? storedOrder : storedOrder.reverse();
          defaultOrder.forEach((id) => {
            if (!order.includes(id)) order.push(id);
          });
        }
        if (Array.isArray(parsed.hidden)) {
          parsed.hidden.forEach((id) => {
            if (defaultOrder.includes(id)) hidden[id] = true;
          });
        }
      }
    } catch (error) {
      console.warn('Failed to load render settings', error);
    }

    renderOrder = order;
    hiddenDrawables = hidden;
  };

  const persistRenderSettings = () => {
    if (!model) return;
    if (typeof localStorage === 'undefined') return;
    const hidden = Object.keys(hiddenDrawables).filter((id) => hiddenDrawables[id]);
    const payload = {
      order: renderOrder,
      hidden,
    };
    try {
      localStorage.setItem(getSettingsKey(model.name), JSON.stringify(payload));
    } catch (error) {
      console.warn('Failed to persist render settings', error);
    }
  };

  const toggleVisibility = (id: string) => {
    hiddenDrawables = { ...hiddenDrawables, [id]: !hiddenDrawables[id] };
    persistRenderSettings();
  };

  const showAllDrawables = () => {
    hiddenDrawables = {};
    persistRenderSettings();
  };

  const resetRenderSettings = () => {
    if (!model) return;
    renderOrder = model.drawables.map((drawable) => drawable.id).reverse();
    hiddenDrawables = {};
    persistRenderSettings();
  };

  const moveDrawable = (id: string, delta: number) => {
    if (!model) return;
    const order = renderOrder.length ? [...renderOrder] : model.drawables.map((drawable) => drawable.id);
    const index = order.indexOf(id);
    if (index < 0) return;
    const nextIndex = index + delta;
    if (nextIndex < 0 || nextIndex >= order.length) return;
    const [removed] = order.splice(index, 1);
    order.splice(nextIndex, 0, removed);
    renderOrder = order;
    persistRenderSettings();
  };

  const moveDrawableTo = (id: string, targetIndex: number) => {
    if (!model) return;
    const order = renderOrder.length ? [...renderOrder] : model.drawables.map((drawable) => drawable.id);
    const index = order.indexOf(id);
    if (index < 0) return;
    const clampedIndex = Math.max(0, Math.min(order.length - 1, targetIndex));
    order.splice(index, 1);
    order.splice(clampedIndex, 0, id);
    renderOrder = order;
    persistRenderSettings();
  };

  const handleOrderDragStart = (event: DragEvent, id: string) => {
    draggingId = id;
    event.dataTransfer?.setData('text/plain', id);
    event.dataTransfer?.setDragImage(event.currentTarget as Element, 12, 12);
    event.dataTransfer?.setData('application/x-lite2d-drawable', id);
  };

  const handleOrderDragOver = (event: DragEvent) => {
    event.preventDefault();
    if (event.dataTransfer) {
      event.dataTransfer.dropEffect = 'move';
    }
    const target = event.currentTarget as HTMLElement | null;
    const targetId = target?.dataset?.rowId ?? null;
    dragOverId = targetId;
  };

  const handleOrderDrop = (event: DragEvent, id: string) => {
    event.preventDefault();
    const dragged = event.dataTransfer?.getData('application/x-lite2d-drawable') || draggingId;
    draggingId = null;
    dragOverId = null;
    if (!dragged || !model) return;
    const order = renderOrder.length ? renderOrder : model.drawables.map((drawable) => drawable.id);
    const targetIndex = order.indexOf(id);
    moveDrawableTo(dragged, targetIndex);
  };

  const handleOrderDragEnd = () => {
    draggingId = null;
    dragOverId = null;
  };


  const loadFromText = (text: string, name: string, sourceUrl?: string) => {
    try {
      const json = JSON.parse(text);
      model = parseMoc3(json, name);
      applyRenderSettings(model);
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

  let meshRowsData: {
    id: string;
    vertices: number;
    uvs: number;
    draw: number;
    visible: boolean;
    orderIndex: number;
    orderTotal: number;
  }[] = [];

  $: meshRowsData = (() => {
    if (!model) return [];
    const drawables = new Map(model.drawables.map((drawable) => [drawable.id, drawable]));
    const order = renderOrder.length ? renderOrder : model.drawables.map((drawable) => drawable.id);
    const filter = meshFilter.trim().toLowerCase();
    return order
      .map((id) => drawables.get(id))
      .filter((drawable): drawable is Moc3Drawable => Boolean(drawable))
      .filter((drawable) => drawable.id.toLowerCase().includes(filter))
      .map((drawable, index) => ({
        id: drawable.id,
        vertices: drawable.vertexCount,
        uvs: drawable.uvs.length,
        draw: drawable.textureIndex,
        visible: !hiddenDrawables[drawable.id],
        orderIndex: renderOrder.length ? renderOrder.indexOf(drawable.id) : index,
        orderTotal: renderOrder.length ? renderOrder.length : model.drawables.length
      }));
  })();

  const selectMesh = (id: string) => {
    selectedMeshId = id;
    tick().then(() => {
      const row = rowElements[id];
      row?.scrollIntoView({ block: 'nearest', inline: 'nearest' });
    });
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
    schedulePreviewRender();
  };

  const handlePointerUp = (event: PointerEvent) => {
    if (pointerId === event.pointerId) {
      canvasEl?.releasePointerCapture(event.pointerId);
    }
    isPanning = false;
    pointerId = null;
    lastPointer = null;
  };

  const ensurePreviewCanvasResolution = () => {
    if (!canvasEl) return;
    dpr = window.devicePixelRatio || 1;
    const rect = canvasEl.getBoundingClientRect();
    const size = Math.max(1, Math.round(Math.min(rect.width, rect.height)));
    if (size !== canvasSize) {
      canvasSize = size;
    }
    const nextWidth = Math.round(canvasSize * dpr);
    const nextHeight = Math.round(canvasSize * dpr);
    if (canvasEl.width !== nextWidth) canvasEl.width = nextWidth;
    if (canvasEl.height !== nextHeight) canvasEl.height = nextHeight;
  };

  const schedulePreviewRender = () => {
    if (!canvasEl) return;
    if (previewRaf) return;
    previewRaf = requestAnimationFrame(() => {
      previewRaf = 0;
      renderPreview();
    });
  };

  const renderPreview = () => {
    if (!canvasEl) return;
    const ctx = canvasEl.getContext('2d');
    if (!ctx) return;

    ensurePreviewCanvasResolution();
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);

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
    schedulePreviewRender();
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

  onMount(() => {
    if (!canvasEl) return;
    ensurePreviewCanvasResolution();
    uvResizeObserver = new ResizeObserver(() => {
      ensurePreviewCanvasResolution();
      schedulePreviewRender();
    });
    uvResizeObserver.observe(canvasEl);
  });

  onDestroy(() => {
    if (previewRaf) cancelAnimationFrame(previewRaf);
    uvResizeObserver?.disconnect();
  });
</script>

<div class="panel">
  <div class="toolbar">
    <div class="actions">
      <button type="button" class="ui-btn primary" onclick={() => fileInput?.click()}>
        Open .moc3.json
      </button>
      <button type="button" class="ui-btn ghost" onclick={loadSample}>
        Load bundled sample
      </button>
      <button type="button" class="ui-btn ghost" onclick={() => textureInput?.click()}>
        Open texture image
      </button>
      <button type="button" class="ui-btn ghost" onclick={tryLoadModelTexture}>
        Use model texture
      </button>
    </div>
    <div class="status">{errorMessage ? errorMessage : statusMessage}</div>
    <input class="hidden" type="file" multiple accept=".json,.moc3.json" bind:this={fileInput} onchange={handleFileChange} />
    <input class="hidden" type="file" accept="image/*" bind:this={textureInput} onchange={handleTextureFile} />
  </div>

    <div
      class="dropzone"
      role="button"
      tabindex="0"
      ondragover={event => event.preventDefault()}
      ondrop={handleDrop}
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
          </div>
          <TextInput
            label="Filter by id"
            bind:value={meshFilter}
            placeholder="Type id to focus"
            oninput={handleFilterInput}
          />
        </div>
        <div class="mesh-table">
          <div class="mesh-header">
            <span class="col-drag">Drag</span>
            <span class="col-visible">Visible</span>
            <span class="col-id">ID</span>
            <span class="header-icon col-verts" title="Vertices"><CircleDot size={14} /></span>
            <span class="header-icon col-uvs" title="UVs"><Grid2x2 size={14} /></span>
            <span class="header-icon col-tex" title="Texture"><ImageIcon size={14} /></span>
            <span class="col-order">Order</span>
          </div>
          <div class="mesh-body">
            {#each meshRowsData as row}
              <div
                class={`mesh-row ${row.id === selectedMeshId ? 'selected' : ''} ${row.id === draggingId ? 'dragging' : ''} ${row.id === dragOverId ? 'drag-over' : ''}`}
                role="row"
                tabindex="0"
                draggable="true"
                data-row-id={row.id}
                ondragstart={(event) => handleOrderDragStart(event, row.id)}
                ondragover={handleOrderDragOver}
                ondrop={(event) => handleOrderDrop(event, row.id)}
                ondragend={handleOrderDragEnd}
                use:registerRow={row.id}
              >
                <div class={`drag-handle col-drag ${row.id === draggingId ? 'active' : ''}`} aria-label="Drag to reorder">
                  <GripHorizontal size={16} />
                </div>
                <button
                  class={`visibility-button col-visible ${row.visible ? 'visible' : 'hidden'}`}
                  onclick={(event) => {
                    event.stopPropagation();
                    toggleVisibility(row.id)
                  }}
                  aria-label={row.visible ? 'Hide layer' : 'Show layer'}
                >
                  {#if row.visible}
                    <Eye size={16} />
                  {:else}
                    <EyeOff size={16} />
                  {/if}
                </button>
                <button class="mesh-select col-id" onclick={() => selectMesh(row.id)} title={row.id}>
                  {row.id}
                </button>
                <span class="col-verts">{row.vertices}</span>
                <span class="col-uvs">{row.uvs}</span>
                <span class="col-tex">{row.draw}</span>
                <div class="order-controls col-order">
                  <button
                    onclick={(event) => {
                      event.stopPropagation();
                      moveDrawable(row.id, -1);
                    }}
                    disabled={row.orderIndex === 0}
                  >
                    Up
                  </button>
                  <button
                    onclick={(event) => {
                      event.stopPropagation();
                      moveDrawable(row.id, 1);
                    }}
                    disabled={row.orderIndex === row.orderTotal - 1}
                  >
                    Down
                  </button>
                </div>
              </div>
            {/each}
          </div>
        </div>
      </div>

      <div class="uv-panel">
        <div class="render-panel">
          <div class="render-header">
            <div>
              <div class="label">Render Preview</div>
              <div class="value">{textureName || 'Texture not loaded'}</div>
              <div class="status muted">{renderStatus}</div>
            </div>
            <div class="render-actions">
              <button type="button" class="ui-btn ghost" onclick={showAllDrawables}>
                Show all
              </button>
              <button type="button" class="ui-btn ghost" onclick={resetRenderSettings}>
                Reset order
              </button>
            </div>
          </div>
          <RenderPreview
            bind:status={renderStatus}
            {model}
            {canvasSize}
          />
        </div>
        <div class="uv-header">
          <div>
            <div class="label">UV Preview</div>
            <div class="value">{selectedMesh?.id ?? 'Select a mesh'}</div>
          </div>
          <Select
            label="Mesh"
            options={(model.drawables.map((mesh) => ({ label: mesh.id, value: mesh.id })))}
            bind:value={selectedMeshId}
            onchange={handleMeshSelectChange}
          />
        </div>

        <div class="texture-bar">
          <div>
            <div class="label">Texture source</div>
            <div class="value">{textureName || 'Select texture image'}</div>
            <div class="status muted">{textureStatus}</div>
          </div>
          <div class="texture-actions">
            <button type="button" class="ui-btn ghost" onclick={() => textureInput?.click()}>
              Open texture
            </button>
            <button type="button" class="ui-btn ghost" onclick={tryLoadModelTexture}>
              Reload model texture
            </button>
          </div>
        </div>

        <div class="view-controls">
          <div class="zoom-row">
            <span class="zoom-label">Zoom</span>
            <input type="range" min={minZoom} max={maxZoom} step="0.05" value={zoom} oninput={handleZoomInput} />
            <span class="zoom-value">{(zoom * 100).toFixed(0)}%</span>
            <div class="view-buttons">
              <button type="button" class="ui-btn ghost" onclick={zoomIn}>Zoom In</button>
              <button type="button" class="ui-btn ghost" onclick={zoomOut}>Zoom Out</button>
              <button type="button" class="ui-btn ghost" onclick={resetView}>Reset View</button>
            </div>
          </div>
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
            class:grabbing={isPanning}
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

  .ui-btn {
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

  .ui-btn.primary {
    background: var(--ui-primary);
    border-color: color-mix(in srgb, var(--ui-primary) 70%, var(--ui-border) 30%);
    color: white;
  }

  .ui-btn.ghost {
    background: transparent;
  }

  .ui-btn.danger {
    background: color-mix(in srgb, var(--ui-danger) 80%, #000 5%);
    border-color: color-mix(in srgb, var(--ui-danger) 70%, var(--ui-border) 30%);
    color: white;
  }

  .ui-btn:hover:not(:disabled) {
    transform: translateY(-1px);
    box-shadow: 0 10px 20px rgba(0, 0, 0, 0.15);
  }

  .ui-btn:disabled {
    opacity: 0.6;
    cursor: not-allowed;
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
    gap: 5px;
    border: var(--ui-border-width) solid var(--ui-border);
    border-radius: var(--ui-radius);
    padding: 5px;
    background: color-mix(in srgb, var(--ui-surface) 92%, transparent);
    font-size: 10pt;
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
    overflow: visible;
  }

  .mesh-header {
    display: grid;
    grid-template-columns: 34px 40px minmax(200px, 1fr) 40px 40px 40px 80px;
    grid-template-areas: 'drag visible id verts uvs tex order';
    padding: 6px 8px;
    background: var(--ui-muted);
    color: var(--ui-text);
    font-weight: 700;
    gap: 8px;
    align-items: center;
    min-width: 640px;
    font-size: inherit;
  }

  .mesh-header .header-icon {
    display: inline-flex;
    align-items: center;
    justify-content: center;
  }

  .mesh-body {
    max-height: 280px;
    overflow-y: auto;
    overflow-x: auto;
    scrollbar-gutter: stable both-edges;
    scrollbar-width: auto;
    scrollbar-color: color-mix(in srgb, var(--ui-primary) 70%, var(--ui-text)) color-mix(in srgb, var(--ui-surface) 70%, transparent);
  }

  .mesh-body::-webkit-scrollbar {
    width: 12px;
    height: 12px;
  }

  .mesh-body::-webkit-scrollbar-track {
    background: color-mix(in srgb, var(--ui-surface) 80%, transparent);
  }

  .mesh-body::-webkit-scrollbar-thumb {
    background: color-mix(in srgb, var(--ui-primary) 70%, var(--ui-text) 30%);
    border-radius: 999px;
    border: 2px solid color-mix(in srgb, var(--ui-surface) 70%, transparent);
  }

  .mesh-row {
    width: 100%;
    display: grid;
    grid-template-columns: 34px 40px minmax(100px, 1fr) 40px 40px 40px 80px;
    grid-template-areas: 'drag visible id verts uvs tex order';
    gap: 8px;
    padding: 6px 8px;
    border-bottom: var(--ui-border-width) solid var(--ui-border);
    background: color-mix(in srgb, var(--ui-surface) 94%, transparent);
    color: var(--ui-text);
    align-items: center;
    font-size: inherit;
    min-width: 640px;
  }

  .col-drag {
    grid-area: drag;
  }

  .col-visible {
    grid-area: visible;
  }

  .col-id {
    grid-area: id;
  }

  .col-verts {
    grid-area: verts;
  }

  .col-uvs {
    grid-area: uvs;
  }

  .col-tex {
    grid-area: tex;
  }

  .col-order {
    grid-area: order;
  }

  .mesh-row:hover {
    background: color-mix(in srgb, var(--ui-primary) 10%, var(--ui-surface) 92%);
  }

  .mesh-row.selected {
    background: color-mix(in srgb, var(--ui-primary) 14%, var(--ui-surface) 90%);
    border-color: color-mix(in srgb, var(--ui-primary) 40%, var(--ui-border));
  }

  .mesh-row.dragging {
    opacity: 0.6;
  }

  .mesh-row.drag-over {
    box-shadow: inset 0 2px 0 color-mix(in srgb, var(--ui-primary) 70%, transparent);
  }

  .mesh-select {
    border: none;
    background: transparent;
    color: inherit;
    text-align: left;
    font: inherit;
    padding: 0;
    cursor: pointer;
    white-space: normal;
    word-break: break-all;
  }

  .visibility-button,
  .order-controls button {
    border: var(--ui-border-width) solid var(--ui-border);
    border-radius: 6px;
    background: color-mix(in srgb, var(--ui-surface) 92%, transparent);
    color: var(--ui-text);
    padding: 0;
    font-size: 0.9em;
    cursor: pointer;
  }

  .visibility-button {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    width: 30px;
    height: 24px;
    justify-content: center;
  }

  .visibility-button.hidden {
    opacity: 0.6;
  }

  .mesh-visibility:disabled,
  .order-controls button:disabled {
    opacity: 0.4;
    cursor: not-allowed;
  }

  .order-controls {
    display: flex;
    gap: 6px;
  }

  .order-controls button {
    padding: 1px 6px;
    font-size: 0.85em;
  }

  .drag-handle {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    width: 30px;
    height: 24px;
    border: var(--ui-border-width) solid var(--ui-border);
    border-radius: 6px;
    background: color-mix(in srgb, var(--ui-surface) 92%, transparent);
    color: var(--ui-text-muted);
    font-size: 14px;
    cursor: grab;
    user-select: none;
  }

  .drag-handle.active {
    cursor: grabbing;
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

  .render-panel {
    display: flex;
    flex-direction: column;
    gap: 8px;
    padding: 10px;
    border: var(--ui-border-width) solid var(--ui-border);
    border-radius: var(--ui-radius);
    background: color-mix(in srgb, var(--ui-muted) 70%, transparent);
  }


  .render-header {
    display: flex;
    align-items: flex-end;
    justify-content: space-between;
    gap: 12px;
  }

  .render-actions {
    display: flex;
    gap: 8px;
    flex-wrap: wrap;
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

  .legend {
    display: grid;
    gap: 6px;
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
