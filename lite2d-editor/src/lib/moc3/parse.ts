export type Moc3Canvas = {
  width: number;
  height: number;
  pixelsPerUnit: number;
};

export type Moc3Drawable = {
  id: string;
  blendMode: number;
  indexCount: number;
  indices: number[];
  opacity: number;
  parentPartIndex: number;
  positions: [number, number][];
  textureIndex: number;
  uvs: [number, number][];
  vertexCount: number;
};

export type Moc3Model = {
  name: string;
  canvas: Moc3Canvas;
  drawables: Moc3Drawable[];
};

const asTupleArray = (value: unknown): [number, number][] => {
  if (!Array.isArray(value)) return [];
  return value
    .filter((item) => Array.isArray(item) && item.length >= 2)
    .map((item) => [Number(item[0]), Number(item[1])] as [number, number]);
};

const extractTupleArray = (value: unknown): [number, number][] => {
  if (!value) return [];
  if (Array.isArray(value)) return asTupleArray(value);
  if (typeof value === 'object') return asTupleArray((value as { values?: unknown }).values);
  return [];
};

const extractNumberArray = (value: unknown): number[] => {
  if (!Array.isArray(value)) return [];
  return value.map((v) => Number(v)).filter((n) => Number.isFinite(n));
};

export const parseMoc3 = (json: unknown, name: string): Moc3Model => {
  const data = (json ?? {}) as Record<string, unknown>;

  const canvasRaw = (data.canvas as Record<string, unknown>) ?? {};
  const canvas: Moc3Canvas = {
    width: typeof canvasRaw.width === 'number' ? canvasRaw.width : 0,
    height: typeof canvasRaw.height === 'number' ? canvasRaw.height : 0,
    pixelsPerUnit: typeof canvasRaw.pixels_per_unit === 'number' ? canvasRaw.pixels_per_unit : 0
  };

  const drawablesRaw = Array.isArray(data.drawables) ? data.drawables : [];
  const drawables: Moc3Drawable[] = drawablesRaw.map((drawable, index) => {
    const d = drawable as Record<string, unknown>;
    const positions = extractTupleArray(d.positions);
    const uvs = extractTupleArray(d.uvs);
    const indices = extractNumberArray(d.indices);

    const inferredVertexCount = Math.max(positions.length, uvs.length);

    return {
      id: typeof d.id === 'string' && d.id.trim().length > 0 ? d.id : `Drawable${index}`,
      blendMode: typeof d.blend_mode === 'number' ? d.blend_mode : 0,
      indexCount: typeof d.index_count === 'number' ? d.index_count : indices.length,
      indices,
      opacity: typeof d.opacity === 'number' ? d.opacity : 1,
      parentPartIndex: typeof d.parent_part_index === 'number' ? d.parent_part_index : -1,
      positions,
      textureIndex: typeof d.texture_index === 'number' ? d.texture_index : 0,
      uvs,
      vertexCount: typeof d.vertex_count === 'number' ? d.vertex_count : inferredVertexCount
    };
  });

  return {
    name,
    canvas,
    drawables
  };
};

export const formatUvPreview = (uvs: [number, number][], limit = 20) =>
  uvs.slice(0, limit).map(([u, v], index) => ({ index, u, v }));
