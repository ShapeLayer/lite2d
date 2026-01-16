<script lang="ts">
  import { createEventDispatcher, onDestroy, onMount } from 'svelte';
  import type { Moc3Drawable, Moc3Model } from '../moc3/parse';

  export let model: Moc3Model | null = null;
  export let textureImage: HTMLImageElement | null = null;
  export let renderOrder: string[] = [];
  export let hiddenDrawables: Record<string, boolean> = {};
  export let status = 'No render yet';

  const dispatch = createEventDispatcher<{ select: { id: string } }>();

  let canvasEl: HTMLCanvasElement | null = null;
  let canvasWidth = 512;
  let canvasHeight = 512;
  let dpr = 1;
  let cacheCanvas: HTMLCanvasElement | OffscreenCanvas | null = null;
  let cacheCtx: CanvasRenderingContext2D | OffscreenCanvasRenderingContext2D | null = null;
  let cacheDpr = 1;
  let cacheDirty = true;
  let aspectStyle = 'aspect-ratio: 1 / 1;';
  let hoverId: string | null = null;
  let hoverLabel: { x: number; y: number } | null = null;
  let panX = 0;
  let panY = 0;
  let zoom = 1;
  const minZoom = 0.35;
  const maxZoom = 4;
  let isPanning = false;
  let pointerId: number | null = null;
  let lastPointer: { x: number; y: number } | null = null;
  let renderRaf = 0;
  let pickRaf = 0;
  let pendingPick: { x: number; y: number; screenX: number; screenY: number } | null = null;
  let isCanvasVisible = true;
  let resizeObserver: ResizeObserver | null = null;
  let intersectionObserver: IntersectionObserver | null = null;

  let drawableMap = new Map<string, Moc3Drawable>();
  let drawableBounds = new Map<string, { minX: number; maxX: number; minY: number; maxY: number }>();

  const ensureCanvasResolution = () => {
    if (!canvasEl) return;
    dpr = window.devicePixelRatio || 1;
    const rect = canvasEl.getBoundingClientRect();
    const nextCssWidth = Math.max(1, Math.round(rect.width));
    const nextCssHeight = Math.max(1, Math.round(rect.height));
    if (nextCssWidth !== canvasWidth) canvasWidth = nextCssWidth;
    if (nextCssHeight !== canvasHeight) canvasHeight = nextCssHeight;
    const nextWidth = Math.round(canvasWidth * dpr);
    const nextHeight = Math.round(canvasHeight * dpr);
    if (canvasEl.width !== nextWidth) canvasEl.width = nextWidth;
    if (canvasEl.height !== nextHeight) canvasEl.height = nextHeight;
    if (cacheDpr !== dpr || !cacheCanvas || (cacheCanvas as HTMLCanvasElement).width !== nextWidth || (cacheCanvas as HTMLCanvasElement).height !== nextHeight) {
      ensureCacheCanvas();
      cacheDirty = true;
    }
  };

  const ensureCacheCanvas = () => {
    if (typeof OffscreenCanvas !== 'undefined') {
      cacheCanvas = new OffscreenCanvas(Math.round(canvasWidth * dpr), Math.round(canvasHeight * dpr));
    } else {
      const fallback = document.createElement('canvas');
      fallback.width = Math.round(canvasWidth * dpr);
      fallback.height = Math.round(canvasHeight * dpr);
      cacheCanvas = fallback;
    }
    cacheDpr = dpr;
    cacheCtx = (cacheCanvas as HTMLCanvasElement | OffscreenCanvas).getContext('2d');
    if (cacheCtx) {
      cacheCtx.setTransform(dpr, 0, 0, dpr, 0, 0);
    }
  };

  const scheduleRender = () => {
    if (!canvasEl || !isCanvasVisible) return;
    if (renderRaf) return;
    renderRaf = requestAnimationFrame(() => {
      renderRaf = 0;
      render();
    });
  };

  const schedulePick = (point: { x: number; y: number; screenX: number; screenY: number } | null) => {
    if (!point || !canvasEl || !isCanvasVisible) return;
    pendingPick = point;
    if (pickRaf) return;
    pickRaf = requestAnimationFrame(() => {
      pickRaf = 0;
      if (!pendingPick) return;
      const id = pickDrawableAt(pendingPick);
      hoverId = id;
      hoverLabel = id ? { x: pendingPick.screenX, y: pendingPick.screenY } : null;
      pendingPick = null;
      scheduleRender();
    });
  };

  const clampZoom = (value: number) => Math.min(maxZoom, Math.max(minZoom, value));

  const updateZoom = (target: number, center?: { x: number; y: number }) => {
    const clamped = clampZoom(target);
    if (center && canvasEl) {
      const rect = canvasEl.getBoundingClientRect();
      const offsetX = center.x - rect.left - canvasWidth / 2 - panX;
      const offsetY = center.y - rect.top - canvasHeight / 2 - panY;
      const factor = clamped / zoom;
      panX -= offsetX * (factor - 1);
      panY -= offsetY * (factor - 1);
    }
    zoom = clamped;
    scheduleRender();
  };

  const handleWheel = (event: WheelEvent) => {
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
    if (isPanning && event.pointerId === pointerId && lastPointer) {
      const dx = event.clientX - lastPointer.x;
      const dy = event.clientY - lastPointer.y;
      panX += dx;
      panY += dy;
      lastPointer = { x: event.clientX, y: event.clientY };
      scheduleRender();
      return;
    }

    const point = getCanvasPoint(event);
    if (!point || !model) {
      hoverId = null;
      hoverLabel = null;
      return;
    }

    schedulePick(point);
  };

  const handlePointerUp = (event: PointerEvent) => {
    if (pointerId === event.pointerId) {
      canvasEl?.releasePointerCapture(event.pointerId);
    }
    isPanning = false;
    pointerId = null;
    lastPointer = null;
  };

  const handlePointerLeave = () => {
    hoverId = null;
    hoverLabel = null;
    isPanning = false;
    pointerId = null;
    lastPointer = null;
    scheduleRender();
  };

  const handleClick = (event: MouseEvent) => {
    const point = getCanvasPoint(event);
    if (!point) return;
    const id = pickDrawableAt(point);
    if (id) {
      dispatch('select', { id });
    }
  };

  const getCanvasPoint = (event: PointerEvent | MouseEvent) => {
    if (!canvasEl) return null;
    const rect = canvasEl.getBoundingClientRect();
    const scaleX = canvasWidth / rect.width;
    const scaleY = canvasHeight / rect.height;
    const rawX = (event.clientX - rect.left) * scaleX;
    const rawY = (event.clientY - rect.top) * scaleY;
    const localX = (rawX - canvasWidth / 2 - panX) / zoom + canvasWidth / 2;
    const localY = (rawY - canvasHeight / 2 - panY) / zoom + canvasHeight / 2;
    return {
      x: localX,
      y: localY,
      screenX: event.clientX - rect.left,
      screenY: event.clientY - rect.top
    };
  };

  const pointInTriangle = (
    p: { x: number; y: number },
    a: { x: number; y: number },
    b: { x: number; y: number },
    c: { x: number; y: number }
  ) => {
    const v0x = c.x - a.x;
    const v0y = c.y - a.y;
    const v1x = b.x - a.x;
    const v1y = b.y - a.y;
    const v2x = p.x - a.x;
    const v2y = p.y - a.y;

    const dot00 = v0x * v0x + v0y * v0y;
    const dot01 = v0x * v1x + v0y * v1y;
    const dot02 = v0x * v2x + v0y * v2y;
    const dot11 = v1x * v1x + v1y * v1y;
    const dot12 = v1x * v2x + v1y * v2y;

    const denom = dot00 * dot11 - dot01 * dot01;
    if (Math.abs(denom) < 1e-6) return false;
    const invDenom = 1 / denom;
    const u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    const v = (dot00 * dot12 - dot01 * dot02) * invDenom;
    return u >= 0 && v >= 0 && u + v <= 1;
  };

  const positionToCanvas = ([x, y]: [number, number], modelWidth: number, modelHeight: number) => ({
    x: (x / modelWidth) * canvasWidth + canvasWidth / 2,
    y: canvasHeight / 2 - (y / modelHeight) * canvasHeight
  });

  const getViewBounds = () => {
    const centerX = canvasWidth / 2;
    const centerY = canvasHeight / 2;
    const minX = (0 - centerX - panX) / zoom + centerX;
    const maxX = (canvasWidth - centerX - panX) / zoom + centerX;
    const minY = (0 - centerY - panY) / zoom + centerY;
    const maxY = (canvasHeight - centerY - panY) / zoom + centerY;
    return { minX, maxX, minY, maxY };
  };

  const pickDrawableAt = (point: { x: number; y: number; screenX: number; screenY: number }) => {
    if (!model) return null;
    const order = renderOrder.length ? renderOrder : model.drawables.map((drawable) => drawable.id).reverse();
    const viewBounds = getViewBounds();

    for (let o = 0; o < order.length; o += 1) {
      const id = order[o];
      if (hiddenDrawables[id]) continue;
      const bounds = drawableBounds.get(id);
      if (bounds) {
        if (bounds.maxX < viewBounds.minX || bounds.minX > viewBounds.maxX) continue;
        if (bounds.maxY < viewBounds.minY || bounds.minY > viewBounds.maxY) continue;
      }
      const drawable = drawableMap.get(id);
      if (!drawable || !drawable.positions.length || !drawable.indices.length || !drawable.uvs.length) continue;
      if (typeof drawable.opacity === 'number' && drawable.opacity <= 0.001) continue;

      for (let i = 0; i < drawable.indices.length; i += 3) {
        const i0 = drawable.indices[i];
        const i1 = drawable.indices[i + 1];
        const i2 = drawable.indices[i + 2];
        if (i0 == null || i1 == null || i2 == null) continue;
        const p0Raw = drawable.positions[i0];
        const p1Raw = drawable.positions[i1];
        const p2Raw = drawable.positions[i2];
        if (!p0Raw || !p1Raw || !p2Raw) continue;

        const p0 = positionToCanvas(p0Raw, model.canvas.width, model.canvas.height);
        const p1 = positionToCanvas(p1Raw, model.canvas.width, model.canvas.height);
        const p2 = positionToCanvas(p2Raw, model.canvas.width, model.canvas.height);

        if (pointInTriangle(point, p0, p1, p2)) {
          return id;
        }
      }
    }

    return null;
  };

  const drawTexturedTriangle = (
    ctx: CanvasRenderingContext2D,
    image: HTMLImageElement,
    p0: { x: number; y: number },
    p1: { x: number; y: number },
    p2: { x: number; y: number },
    uv0: [number, number],
    uv1: [number, number],
    uv2: [number, number]
  ) => {
    const sx0 = uv0[0] * image.width;
    const sy0 = (1 - uv0[1]) * image.height;
    const sx1 = uv1[0] * image.width;
    const sy1 = (1 - uv1[1]) * image.height;
    const sx2 = uv2[0] * image.width;
    const sy2 = (1 - uv2[1]) * image.height;

    const denom = sx0 * (sy1 - sy2) + sx1 * (sy2 - sy0) + sx2 * (sy0 - sy1);
    if (Math.abs(denom) < 1e-6) return;

    const a = (p0.x * (sy1 - sy2) + p1.x * (sy2 - sy0) + p2.x * (sy0 - sy1)) / denom;
    const b = (p0.y * (sy1 - sy2) + p1.y * (sy2 - sy0) + p2.y * (sy0 - sy1)) / denom;
    const c = (p0.x * (sx2 - sx1) + p1.x * (sx0 - sx2) + p2.x * (sx1 - sx0)) / denom;
    const d = (p0.y * (sx2 - sx1) + p1.y * (sx0 - sx2) + p2.y * (sx1 - sx0)) / denom;
    const e =
      (p0.x * (sx1 * sy2 - sx2 * sy1) +
        p1.x * (sx2 * sy0 - sx0 * sy2) +
        p2.x * (sx0 * sy1 - sx1 * sy0)) /
      denom;
    const f =
      (p0.y * (sx1 * sy2 - sx2 * sy1) +
        p1.y * (sx2 * sy0 - sx0 * sy2) +
        p2.y * (sx0 * sy1 - sx1 * sy0)) /
      denom;

    ctx.save();
    ctx.beginPath();
    ctx.moveTo(p0.x, p0.y);
    ctx.lineTo(p1.x, p1.y);
    ctx.lineTo(p2.x, p2.y);
    ctx.closePath();
    ctx.clip();
    ctx.transform(a, b, c, d, e, f);
    ctx.drawImage(image, 0, 0);
    ctx.restore();
  };

  const render = () => {
    if (!canvasEl) return;
    const ctx = canvasEl.getContext('2d', { alpha: false, desynchronized: true });
    if (!ctx) return;

    ensureCanvasResolution();
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.clearRect(0, 0, canvasWidth, canvasHeight);
    ctx.fillStyle = '#0f172a';
    ctx.fillRect(0, 0, canvasWidth, canvasHeight);

    if (!model || !textureImage) {
      status = model ? 'Texture missing' : 'No model loaded';
      return;
    }

    if (!cacheCanvas || !cacheCtx) {
      ensureCacheCanvas();
    }
    if (cacheDirty && cacheCtx && cacheCanvas) {
      cacheCtx.setTransform(dpr, 0, 0, dpr, 0, 0);
      cacheCtx.clearRect(0, 0, canvasWidth, canvasHeight);
      const order = renderOrder.length ? renderOrder : model.drawables.map((drawable) => drawable.id).reverse();
      const drawableMapLocal = drawableMap;

      [...order].reverse().forEach((id) => {
        const drawable = drawableMapLocal.get(id);
        if (!drawable || hiddenDrawables[id]) return;
        if (!drawable.positions.length || !drawable.indices.length || !drawable.uvs.length) return;

        cacheCtx.globalAlpha = typeof drawable.opacity === 'number' ? drawable.opacity : 1;
        for (let i = 0; i < drawable.indices.length; i += 3) {
          const i0 = drawable.indices[i];
          const i1 = drawable.indices[i + 1];
          const i2 = drawable.indices[i + 2];
          if (i0 == null || i1 == null || i2 == null) continue;
          const p0Raw = drawable.positions[i0];
          const p1Raw = drawable.positions[i1];
          const p2Raw = drawable.positions[i2];
          const uv0 = drawable.uvs[i0];
          const uv1 = drawable.uvs[i1];
          const uv2 = drawable.uvs[i2];
          if (!p0Raw || !p1Raw || !p2Raw || !uv0 || !uv1 || !uv2) continue;

          const p0 = positionToCanvas(p0Raw, model.canvas.width, model.canvas.height);
          const p1 = positionToCanvas(p1Raw, model.canvas.width, model.canvas.height);
          const p2 = positionToCanvas(p2Raw, model.canvas.width, model.canvas.height);

          drawTexturedTriangle(cacheCtx, textureImage, p0, p1, p2, uv0, uv1, uv2);
        }
      });

      cacheCtx.globalAlpha = 1;
      cacheDirty = false;
    }

    ctx.save();
    ctx.translate(canvasWidth / 2 + panX, canvasHeight / 2 + panY);
    ctx.scale(zoom, zoom);
    ctx.translate(-canvasWidth / 2, -canvasHeight / 2);

    if (cacheCanvas) {
      ctx.drawImage(cacheCanvas as CanvasImageSource, 0, 0, canvasWidth, canvasHeight);
    }

    if (hoverId) {
      const hovered = drawableMap.get(hoverId);
      if (hovered && !hiddenDrawables[hoverId] && hovered.positions.length && hovered.indices.length) {
        ctx.save();
        ctx.globalAlpha = 1;
        ctx.strokeStyle = 'rgba(255, 255, 255, 0.9)';
        ctx.lineWidth = 1;
        ctx.beginPath();
        for (let i = 0; i < hovered.indices.length; i += 3) {
          const i0 = hovered.indices[i];
          const i1 = hovered.indices[i + 1];
          const i2 = hovered.indices[i + 2];
          const p0Raw = hovered.positions[i0];
          const p1Raw = hovered.positions[i1];
          const p2Raw = hovered.positions[i2];
          if (!p0Raw || !p1Raw || !p2Raw) continue;
          const p0 = positionToCanvas(p0Raw, model.canvas.width, model.canvas.height);
          const p1 = positionToCanvas(p1Raw, model.canvas.width, model.canvas.height);
          const p2 = positionToCanvas(p2Raw, model.canvas.width, model.canvas.height);
          ctx.moveTo(p0.x, p0.y);
          ctx.lineTo(p1.x, p1.y);
          ctx.lineTo(p2.x, p2.y);
          ctx.closePath();
        }
        ctx.stroke();
        ctx.restore();
      }
    }

    ctx.restore();
    ctx.globalAlpha = 1;
    const order = renderOrder.length ? renderOrder : model.drawables.map((drawable) => drawable.id).reverse();
    status = `Rendered ${order.length} drawables (cached)`;
  };

  const rebuildDrawableBounds = () => {
    drawableBounds = new Map();
    if (!model) return;
    model.drawables.forEach((drawable) => {
      if (!drawable.positions.length) return;
      let minX = Infinity;
      let maxX = -Infinity;
      let minY = Infinity;
      let maxY = -Infinity;
      drawable.positions.forEach((position) => {
        if (!position) return;
        const p = positionToCanvas(position, model.canvas.width, model.canvas.height);
        minX = Math.min(minX, p.x);
        maxX = Math.max(maxX, p.x);
        minY = Math.min(minY, p.y);
        maxY = Math.max(maxY, p.y);
      });
      if (Number.isFinite(minX) && Number.isFinite(minY)) {
        drawableBounds.set(drawable.id, { minX, maxX, minY, maxY });
      }
    });
  };

  onMount(() => {
    if (canvasEl) {
      ensureCanvasResolution();
      resizeObserver = new ResizeObserver(() => {
        ensureCanvasResolution();
        rebuildDrawableBounds();
        scheduleRender();
      });
      resizeObserver.observe(canvasEl);

      intersectionObserver = new IntersectionObserver(
        (entries) => {
          const visible = entries.some((entry) => entry.isIntersecting);
          if (visible !== isCanvasVisible) {
            isCanvasVisible = visible;
            if (isCanvasVisible) scheduleRender();
          }
        },
        { threshold: 0.05 }
      );
      intersectionObserver.observe(canvasEl);
    }
  });

  onDestroy(() => {
    if (renderRaf) cancelAnimationFrame(renderRaf);
    if (pickRaf) cancelAnimationFrame(pickRaf);
    resizeObserver?.disconnect();
    intersectionObserver?.disconnect();
  });

  $: drawableMap = new Map<string, Moc3Drawable>(model?.drawables.map((drawable) => [drawable.id, drawable]) ?? []);
  $: if (model) {
    rebuildDrawableBounds();
  }

  $: cacheTrigger = `${model?.name ?? 'none'}-${textureImage ? 1 : 0}-${renderOrder.join(',')}-${Object.keys(hiddenDrawables)
    .filter((id) => hiddenDrawables[id])
    .join(',')}-${canvasWidth}x${canvasHeight}-${dpr.toFixed(2)}`;
  $: if (cacheTrigger) {
    cacheDirty = true;
  }

  $: aspectStyle = model ? `aspect-ratio: ${model.canvas.width} / ${model.canvas.height};` : 'aspect-ratio: 1 / 1;';

  $: renderTrigger = `${cacheTrigger}-${hoverId ?? 'none'}-${panX.toFixed(1)}-${panY.toFixed(1)}-${zoom.toFixed(3)}`;
  $: if (canvasEl) {
    renderTrigger;
    scheduleRender();
  }
</script>

<div class="render-canvas-wrapper" style={aspectStyle}>
  <canvas
    bind:this={canvasEl}
    width={canvasWidth}
    height={canvasHeight}
    onwheel={handleWheel}
    onpointerdown={handlePointerDown}
    onpointermove={handlePointerMove}
    onpointerup={handlePointerUp}
    onpointerleave={handlePointerLeave}
    onclick={handleClick}
  ></canvas>
  {#if hoverId && hoverLabel}
    <div class="render-hover-label" style={`left: ${hoverLabel.x}px; top: ${hoverLabel.y}px;`}>
      {hoverId}
    </div>
  {/if}
</div>

<style>
  .render-canvas-wrapper {
    position: relative;
    width: 100%;
    max-width: 100%;
    aspect-ratio: 1 / 1;
  }

  canvas {
    position: absolute;
    inset: 0;
    width: 100%;
    height: 100%;
    display: block;
  }

  .render-hover-label {
    position: absolute;
    transform: translate(8px, -22px);
    padding: 4px 8px;
    border-radius: 6px;
    background: rgba(15, 23, 42, 0.9);
    color: #fff;
    font-size: 11px;
    pointer-events: none;
    white-space: nowrap;
    border: 1px solid rgba(255, 255, 255, 0.15);
    box-shadow: 0 6px 18px rgba(0, 0, 0, 0.35);
  }
</style>
