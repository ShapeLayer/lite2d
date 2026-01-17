import { get, writable } from 'svelte/store';
import { parseMoc3, type Moc3Model } from './parse';

export type Moc3State = {
  model: Moc3Model | null;
  textureImage: HTMLImageElement | null;
  textureUrl: string;
  textureName: string;
  textureStatus: string;
  errorMessage: string;
  statusMessage: string;
  selectedMeshId: string;
  selectedMeshIds: string[];
  renderOrder: string[];
  hiddenDrawables: Record<string, boolean>;
  faceParts: Record<string, string[]>;
  lastModelUrl: string | null;
};

const initialState: Moc3State = {
  model: null,
  textureImage: null,
  textureUrl: '',
  textureName: '',
  textureStatus: 'No texture loaded',
  errorMessage: '',
  statusMessage: 'Drop a .moc3.json file or load the bundled sample.',
  selectedMeshId: '',
  selectedMeshIds: [],
  renderOrder: [],
  hiddenDrawables: {},
  faceParts: {},
  lastModelUrl: null
};

export const moc3State = writable<Moc3State>(initialState);

const getSettingsKey = (name: string) => `lite2d-editor:renderSettings:${name}`;
const getFacePartsKey = (name: string) => `lite2d-editor:faceParts:${name}`;

const FACE_PART_TAGS = [
  'eye_left',
  'eye_right',
  'eye_white_left',
  'eye_white_right',
  'eye_ball_left',
  'eye_ball_right',
  'eyelid_left',
  'eyelid_right',
  'eye',
  'brow_left',
  'brow_right',
  'mouth',
  'lip_upper',
  'lip_lower',
  'tongue',
  'teeth',
  'nose',
  'cheek_left',
  'cheek_right',
  'face',
  'face_motion',
  'head',
  'neck',
  'neck_seam',
  'jaw_seam',
  'shoulder_left',
  'shoulder_right',
  'torso',
  'chest',
  'body',
  'hair',
  'hair_front',
  'hair_back',
  'hair_side',
  'ear_left',
  'ear_right'
];

const applyRenderSettings = (nextModel: Moc3Model) => {
  const defaultOrder = nextModel.drawables.map((drawable) => drawable.id).reverse();
  let order = [...defaultOrder];
  let hidden: Record<string, boolean> = {};

  if (typeof localStorage === 'undefined') {
    return { order, hidden };
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

  return { order, hidden };
};

const persistRenderSettings = () => {
  const { model, renderOrder, hiddenDrawables } = get(moc3State);
  if (!model) return;
  if (typeof localStorage === 'undefined') return;
  const hidden = Object.keys(hiddenDrawables).filter((id) => hiddenDrawables[id]);
  const payload = {
    order: renderOrder,
    hidden
  };
  try {
    localStorage.setItem(getSettingsKey(model.name), JSON.stringify(payload));
  } catch (error) {
    console.warn('Failed to persist render settings', error);
  }
};

const applyFacePartsSettings = (nextModel: Moc3Model) => {
  const faceParts: Record<string, string[]> = {};
  if (typeof localStorage === 'undefined') return faceParts;
  try {
    const stored = localStorage.getItem(getFacePartsKey(nextModel.name));
    if (stored) {
      const parsed = JSON.parse(stored) as { mesh_parts?: Record<string, string[]> };
      const drawableIds = new Set(nextModel.drawables.map((drawable) => drawable.id));
      if (parsed?.mesh_parts && typeof parsed.mesh_parts === 'object') {
        Object.entries(parsed.mesh_parts).forEach(([meshId, tags]) => {
          if (!drawableIds.has(meshId)) return;
          if (!Array.isArray(tags)) return;
          const filtered = tags.filter((tag) => FACE_PART_TAGS.includes(tag));
          if (filtered.length) faceParts[meshId] = filtered;
        });
      }
    }
  } catch (error) {
    console.warn('Failed to load face parts settings', error);
  }
  return faceParts;
};


const persistFacePartsSettings = () => {
  const { model, faceParts } = get(moc3State);
  if (!model) return;
  if (typeof localStorage === 'undefined') return;
  const payload = { mesh_parts: faceParts };
  try {
    localStorage.setItem(getFacePartsKey(model.name), JSON.stringify(payload));
  } catch (error) {
    console.warn('Failed to persist face parts settings', error);
  }
};


const setState = (patch: Partial<Moc3State>) => {
  moc3State.update((state) => ({ ...state, ...patch }));
};

const setTexture = (url: string, name?: string) => {
  if (!url) return;
  const img = new Image();
  img.crossOrigin = 'anonymous';
  img.onload = () => {
    setState({
      textureImage: img,
      textureUrl: url,
      textureName: name ?? url,
      textureStatus: `Texture loaded (${img.width}x${img.height})`
    });
  };
  img.onerror = () => {
    setState({ textureStatus: 'Failed to load texture. Select manually.' });
  };
  img.src = url;
};

const tryLoadModelTexture = () => {
  const { model, lastModelUrl } = get(moc3State);
  if (!model?.texture) {
    setState({ textureStatus: 'No texture declared in moc3' });
    return;
  }

  if (lastModelUrl) {
    try {
      const base = lastModelUrl.substring(0, lastModelUrl.lastIndexOf('/') + 1);
      setTexture(new URL(model.texture, base).href, model.texture);
      return;
    } catch (error) {
      console.warn('Relative texture resolution failed', error);
    }
  }

  const assumed = new URL(`../../live2d-assets/mao_pro/${model.texture}`, import.meta.url).href;
  setTexture(assumed, model.texture);
};

const loadFromText = (text: string, name: string, sourceUrl?: string) => {
  try {
    const json = JSON.parse(text);
    const model = parseMoc3(json, name);
    const { order, hidden } = applyRenderSettings(model);
    const faceParts = applyFacePartsSettings(model);
    setState({
      model,
      renderOrder: order,
      hiddenDrawables: hidden,
      faceParts,
      selectedMeshId: model.drawables[0]?.id ?? '',
      selectedMeshIds: model.drawables[0]?.id ? [model.drawables[0].id] : [],
      errorMessage: '',
      statusMessage: `${name} loaded (${model.drawables.length} drawables)`,
      textureName: '',
      textureImage: null,
      textureUrl: '',
      textureStatus: 'No texture loaded',
      lastModelUrl: sourceUrl ?? null
    });
    tryLoadModelTexture();
  } catch (error) {
    console.error(error);
    setState({
      errorMessage: 'Failed to parse moc3 JSON. Check file format.',
      model: null,
      selectedMeshId: '',
      selectedMeshIds: [],
      textureStatus: 'No texture loaded',
      lastModelUrl: null
    });
  }
};

const loadTextureFromFileList = (files: FileList | File[]) => {
  const { model } = get(moc3State);
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

const setTextureFromFile = (file: File) => {
  if (!file) return;
  const url = URL.createObjectURL(file);
  setTexture(url, file.name);
};

const setTextureFromUrl = (url: string, name?: string) => {
  if (!url) return;
  setTexture(url, name ?? url);
};

const selectMesh = (id: string) => {
  setState({ selectedMeshId: id, selectedMeshIds: id ? [id] : [] });
};

const selectMeshes = (ids: string[]) => {
  const unique = Array.from(new Set(ids)).filter(Boolean);
  setState({ selectedMeshId: unique[0] ?? '', selectedMeshIds: unique });
};

const toggleVisibility = (id: string) => {
  moc3State.update((state) => ({
    ...state,
    hiddenDrawables: { ...state.hiddenDrawables, [id]: !state.hiddenDrawables[id] }
  }));
  persistRenderSettings();
};

const showAllDrawables = () => {
  setState({ hiddenDrawables: {} });
  persistRenderSettings();
};

const resetRenderSettings = () => {
  const { model } = get(moc3State);
  if (!model) return;
  setState({
    renderOrder: model.drawables.map((drawable) => drawable.id).reverse(),
    hiddenDrawables: {}
  });
  persistRenderSettings();
};

const toggleFacePart = (meshId: string, tag: string) => {
  if (!FACE_PART_TAGS.includes(tag)) return;
  moc3State.update((state) => {
    const current = new Set(state.faceParts[meshId] ?? []);
    if (current.has(tag)) current.delete(tag);
    else current.add(tag);
    const next = { ...state.faceParts };
    if (current.size) next[meshId] = Array.from(current);
    else delete next[meshId];
    return { ...state, faceParts: next };
  });
  persistFacePartsSettings();
};

const addFacePartTag = (meshId: string, tag: string) => {
  if (!FACE_PART_TAGS.includes(tag)) return;
  moc3State.update((state) => {
    const current = new Set(state.faceParts[meshId] ?? []);
    current.add(tag);
    return { ...state, faceParts: { ...state.faceParts, [meshId]: Array.from(current) } };
  });
  persistFacePartsSettings();
};

const removeFacePartTag = (meshId: string, tag: string) => {
  if (!FACE_PART_TAGS.includes(tag)) return;
  moc3State.update((state) => {
    const current = new Set(state.faceParts[meshId] ?? []);
    current.delete(tag);
    const next = { ...state.faceParts };
    if (current.size) next[meshId] = Array.from(current);
    else delete next[meshId];
    return { ...state, faceParts: next };
  });
  persistFacePartsSettings();
};

const clearFacePartTag = (tag: string) => {
  if (!FACE_PART_TAGS.includes(tag)) return;
  moc3State.update((state) => {
    const next = { ...state.faceParts };
    Object.keys(next).forEach((meshId) => {
      const tags = new Set(next[meshId] ?? []);
      tags.delete(tag);
      if (tags.size) next[meshId] = Array.from(tags);
      else delete next[meshId];
    });
    return { ...state, faceParts: next };
  });
  persistFacePartsSettings();
};

const clearFacePartsForMesh = (meshId: string) => {
  moc3State.update((state) => {
    if (!state.faceParts[meshId]) return state;
    const next = { ...state.faceParts };
    delete next[meshId];
    return { ...state, faceParts: next };
  });
  persistFacePartsSettings();
};


const exportFaceParts = () => {
  const { model, faceParts } = get(moc3State);
  if (!model) return false;
  const facePartsMap: Record<string, string[]> = {};
  Object.entries(faceParts).forEach(([meshId, tags]) => {
    tags.forEach((tag) => {
      if (!facePartsMap[tag]) facePartsMap[tag] = [];
      facePartsMap[tag].push(meshId);
    });
  });
  const payload = {
    model: model.name,
    face_parts: facePartsMap
  };
  const blob = new Blob([JSON.stringify(payload, null, 2)], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const baseName = model.name.replace(/\.[^/.]+$/, '');
  const anchor = document.createElement('a');
  anchor.href = url;
  anchor.download = `${baseName || 'moc3'}.parts.json`;
  anchor.click();
  setTimeout(() => URL.revokeObjectURL(url), 0);
  return true;
};


const applyFacePartsPayload = (payload: {
  model?: string;
  face_parts?: Record<string, string[]>;
  body_parts?: Record<string, string[]>;
  seam_parts?: Record<string, string[]>;
}) => {
  const { model } = get(moc3State);
  if (!model) return false;
  const drawableIds = new Set(model.drawables.map((drawable) => drawable.id));
  const nextFace: Record<string, string[]> = {};
  const applyMap = (parts?: Record<string, string[]>) => {
    if (!parts || typeof parts !== 'object') return;
    Object.entries(parts).forEach(([tag, ids]) => {
      if (!FACE_PART_TAGS.includes(tag) || !Array.isArray(ids)) return;
      ids.forEach((meshId) => {
        if (!drawableIds.has(meshId)) return;
        const current = nextFace[meshId] ?? [];
        if (!current.includes(tag)) current.push(tag);
        nextFace[meshId] = current;
      });
    });
  };
  applyMap(payload.face_parts);
  applyMap(payload.body_parts);
  applyMap(payload.seam_parts);
  setState({ faceParts: nextFace });
  persistFacePartsSettings();
  return true;
};


const importFacePartsFromText = (text: string) => {
  try {
    const payload = JSON.parse(text) as { model?: string; face_parts?: Record<string, string[]> };
    return applyFacePartsPayload(payload);
  } catch (error) {
    console.warn('Failed to import face parts', error);
    return false;
  }
};


const importFacePartsFromFile = async (file: File) => {
  if (!file) return false;
  try {
    const text = await file.text();
    return importFacePartsFromText(text);
  } catch (error) {
    console.warn('Failed to read face parts file', error);
    return false;
  }
};


const moveDrawable = (id: string, delta: number) => {
  const { model, renderOrder } = get(moc3State);
  if (!model) return;
  const order = renderOrder.length ? [...renderOrder] : model.drawables.map((drawable) => drawable.id);
  const index = order.indexOf(id);
  if (index < 0) return;
  const nextIndex = index + delta;
  if (nextIndex < 0 || nextIndex >= order.length) return;
  const [removed] = order.splice(index, 1);
  order.splice(nextIndex, 0, removed);
  setState({ renderOrder: order });
  persistRenderSettings();
};

const moveDrawableTo = (id: string, targetIndex: number) => {
  const { model, renderOrder } = get(moc3State);
  if (!model) return;
  const order = renderOrder.length ? [...renderOrder] : model.drawables.map((drawable) => drawable.id);
  const index = order.indexOf(id);
  if (index < 0) return;
  const clampedIndex = Math.max(0, Math.min(order.length - 1, targetIndex));
  order.splice(index, 1);
  order.splice(clampedIndex, 0, id);
  setState({ renderOrder: order });
  persistRenderSettings();
};

const exportRenderSettings = () => {
  const { model, renderOrder, hiddenDrawables } = get(moc3State);
  if (!model) return false;
  const order = renderOrder.length ? renderOrder : model.drawables.map((drawable) => drawable.id).reverse();
  const hidden = Object.keys(hiddenDrawables).filter((id) => hiddenDrawables[id]);
  const payload = {
    model: model.name,
    order,
    hidden,
    version: 2
  };
  const blob = new Blob([JSON.stringify(payload, null, 2)], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const baseName = model.name.replace(/\.[^/.]+$/, '');
  const anchor = document.createElement('a');
  anchor.href = url;
  anchor.download = `${baseName || 'moc3'}.render-settings.json`;
  anchor.click();
  setTimeout(() => URL.revokeObjectURL(url), 0);
  return true;
};

const applyRenderSettingsPayload = (payload: { model?: string; order?: string[]; hidden?: string[]; version?: number }) => {
  const { model } = get(moc3State);
  if (!model) return false;
  const defaultOrder = model.drawables.map((drawable) => drawable.id).reverse();
  let order = [...defaultOrder];
  if (Array.isArray(payload.order)) {
    const filtered = payload.order.filter((id) => defaultOrder.includes(id));
    order = payload.version === 2 ? filtered : filtered;
    defaultOrder.forEach((id) => {
      if (!order.includes(id)) order.push(id);
    });
  }
  const hidden: Record<string, boolean> = {};
  if (Array.isArray(payload.hidden)) {
    payload.hidden.forEach((id) => {
      if (defaultOrder.includes(id)) hidden[id] = true;
    });
  }
  setState({ renderOrder: order, hiddenDrawables: hidden });
  persistRenderSettings();
  return true;
};

const importRenderSettingsFromText = (text: string) => {
  try {
    const payload = JSON.parse(text) as { model?: string; order?: string[]; hidden?: string[]; version?: number };
    return applyRenderSettingsPayload(payload);
  } catch (error) {
    console.warn('Failed to import render settings', error);
    return false;
  }
};

const importRenderSettingsFromFile = async (file: File) => {
  if (!file) return false;
  try {
    const text = await file.text();
    return importRenderSettingsFromText(text);
  } catch (error) {
    console.warn('Failed to read render settings file', error);
    return false;
  }
};

export const moc3Actions = {
  loadFromText,
  loadTextureFromFileList,
  setTextureFromFile,
  setTextureFromUrl,
  tryLoadModelTexture,
  selectMesh,
  selectMeshes,
  toggleVisibility,
  showAllDrawables,
  resetRenderSettings,
  toggleFacePart,
  addFacePartTag,
  removeFacePartTag,
  clearFacePartTag,
  clearFacePartsForMesh,
  moveDrawable,
  moveDrawableTo,
  exportRenderSettings,
  importRenderSettingsFromText,
  importRenderSettingsFromFile,
  exportFaceParts,
  importFacePartsFromText,
  importFacePartsFromFile
};

export { FACE_PART_TAGS };
