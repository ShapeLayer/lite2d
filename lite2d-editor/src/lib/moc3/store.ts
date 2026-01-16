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
  renderOrder: string[];
  hiddenDrawables: Record<string, boolean>;
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
  renderOrder: [],
  hiddenDrawables: {},
  lastModelUrl: null
};

export const moc3State = writable<Moc3State>(initialState);

const getSettingsKey = (name: string) => `lite2d-editor:renderSettings:${name}`;

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
    setState({
      model,
      renderOrder: order,
      hiddenDrawables: hidden,
      selectedMeshId: model.drawables[0]?.id ?? '',
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
  setState({ selectedMeshId: id });
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
  toggleVisibility,
  showAllDrawables,
  resetRenderSettings,
  moveDrawable,
  moveDrawableTo,
  exportRenderSettings,
  importRenderSettingsFromText,
  importRenderSettingsFromFile
};
