import { derived, writable } from 'svelte/store';
import {
  addPanelToTabs,
  dockPanelAtRoot,
  layoutContainsPanel,
  makeTabs,
  movePanelToTabs,
  removePanelFromLayout,
    setActiveTab,
    setSplitSizes
} from './layout';
import type { DockZone, LayoutNode, MenuGroup, PanelRegistration, PanelWindow, Theme } from './types';

export type UIState = {
  registry: Record<string, PanelRegistration>;
  layout: LayoutNode | null;
  windows: PanelWindow[];
  draggingPanelId: string | null;
  menuBar: MenuGroup[];
  theme: Theme;
  nextZ: number;
};

export const darkTheme: Theme = {
  name: 'dark',
  colors: {
    bg: '#13171c',
    surface: '#1d232c',
    muted: '#242c37',
    border: '#2f3845',
    primary: '#58a6ff',
    text: '#e5ecf5',
    textMuted: '#9aa5b5',
    danger: '#f47067',
    highlight: '#304155'
  },
  fontFamily: '"Space Grotesk", "Segoe UI", system-ui, -apple-system, sans-serif',
  radius: 8,
  spacing: 8,
  borderWidth: 1
};

export const lightTheme: Theme = {
  name: 'light',
  colors: {
    bg: '#f4f6fb',
    surface: '#ffffff',
    muted: '#e8edf5',
    border: '#cdd6e5',
    primary: '#2563eb',
    text: '#0f172a',
    textMuted: '#475569',
    danger: '#dc2626',
    highlight: '#cfd9ee'
  },
  fontFamily: '"Space Grotesk", "Segoe UI", system-ui, -apple-system, sans-serif',
  radius: 8,
  spacing: 8,
  borderWidth: 1
};

const createUI = () => {
  const initial: UIState = {
    registry: {},
    layout: null,
    windows: [],
    draggingPanelId: null,
    menuBar: [],
    theme: darkTheme,
    nextZ: 10
  };

  const { subscribe, update, set } = writable<UIState>(initial);

  const applyTheme = (theme: Theme) => {
    const root = document.documentElement;
    root.style.setProperty('--ui-bg', theme.colors.bg);
    root.style.setProperty('--ui-surface', theme.colors.surface);
    root.style.setProperty('--ui-muted', theme.colors.muted);
    root.style.setProperty('--ui-border', theme.colors.border);
    root.style.setProperty('--ui-primary', theme.colors.primary);
    root.style.setProperty('--ui-text', theme.colors.text);
    root.style.setProperty('--ui-text-muted', theme.colors.textMuted);
    root.style.setProperty('--ui-danger', theme.colors.danger);
    root.style.setProperty('--ui-highlight', theme.colors.highlight);
    root.style.setProperty('--ui-font', theme.fontFamily);
    root.style.setProperty('--ui-radius', `${theme.radius}px`);
    root.style.setProperty('--ui-spacing', `${theme.spacing}px`);
    root.style.setProperty('--ui-border-width', `${theme.borderWidth}px`);
  };

  if (typeof document !== 'undefined') {
    applyTheme(initial.theme);
  }

  const registerPanel = (panel: PanelRegistration) => {
    update((state) => {
      state.registry[panel.id] = panel;
      if (!state.layout) {
        state.layout = makeTabs(panel.id);
      }
      return { ...state };
    });
  };

  const openPanel = (panelId: string) => {
    update((state) => {
      if (!state.registry[panelId]) return state;
      if (!layoutContainsPanel(state.layout, panelId)) {
        state.layout = addPanelToTabs(state.layout, panelId);
      }
      return { ...state };
    });
  };

  const closePanel = (panelId: string) => {
    update((state) => {
      state.layout = removePanelFromLayout(state.layout, panelId);
      state.windows = state.windows.filter((w) => w.panelId !== panelId);
      return { ...state };
    });
  };

  const toggleWindowMode = (panelId: string) => {
    update((state) => {
      const existing = state.windows.find((w) => w.panelId === panelId);
      if (existing) {
        state.windows = state.windows.filter((w) => w.panelId !== panelId);
        state.layout = addPanelToTabs(state.layout, panelId);
        return { ...state };
      }

      state.layout = removePanelFromLayout(state.layout, panelId);
      state.windows = [
        ...state.windows,
        {
          panelId,
          x: 140,
          y: 140,
          width: 420,
          height: 320,
          z: state.nextZ + 1
        }
      ];
      state.nextZ += 1;
      return { ...state };
    });
  };

  const moveWindow = (panelId: string, dx: number, dy: number) => {
    update((state) => {
      state.windows = state.windows.map((w) =>
        w.panelId === panelId ? { ...w, x: w.x + dx, y: w.y + dy } : w
      );
      return { ...state };
    });
  };

  const resizeWindow = (panelId: string, dx: number, dy: number, minWidth = 240, minHeight = 160) => {
    update((state) => {
      state.windows = state.windows.map((w) => {
        if (w.panelId !== panelId) return w;
        return {
          ...w,
          width: Math.max(minWidth, w.width + dx),
          height: Math.max(minHeight, w.height + dy)
        };
      });
      return { ...state };
    });
  };

  const focusWindow = (panelId: string) => {
    update((state) => {
      state.nextZ += 1;
      state.windows = state.windows.map((w) =>
        w.panelId === panelId ? { ...w, z: state.nextZ } : w
      );
      return { ...state };
    });
  };

  const dockPanel = (panelId: string, zone: DockZone) => {
    update((state) => {
      state.windows = state.windows.filter((w) => w.panelId !== panelId);
      const cleanedLayout = removePanelFromLayout(state.layout, panelId);
      state.layout = dockPanelAtRoot(cleanedLayout, panelId, zone);
      return { ...state };
    });
  };

  const attachPanelToTabs = (panelId: string, tabsId: string) => {
    update((state) => {
      state.windows = state.windows.filter((w) => w.panelId !== panelId);
      if (state.layout) {
        state.layout = movePanelToTabs(state.layout, panelId, tabsId);
      } else {
        state.layout = makeTabs(panelId);
      }
      return { ...state };
    });
  };

  const setDragging = (panelId: string | null) => {
    if (panelId === null) {
      console.log('setDragging(null) called from:');
      console.trace();  // 스택 트레이스 출력
    }
    update((state) => ({ ...state, draggingPanelId: panelId }));
  };
  
  const setActiveTabId = (tabsId: string, panelId: string) => {
    update((state) => ({ ...state, layout: state.layout ? setActiveTab(state.layout, tabsId, panelId) : state.layout }));
  };

  const updateSplitSizes = (splitId: string, sizes: number[]) => {
    update((state) => ({ ...state, layout: state.layout ? setSplitSizes(state.layout, splitId, sizes) : state.layout }));
  };

  const registerMenu = (menu: MenuGroup) => {
    update((state) => {
      const existingIndex = state.menuBar.findIndex((m) => m.id === menu.id);
      if (existingIndex !== -1) {
        state.menuBar[existingIndex] = menu;
      } else {
        state.menuBar.push(menu);
      }
      state.menuBar.sort((a, b) => (a.order ?? 0) - (b.order ?? 0));
      return { ...state };
    });
  };

  const setTheme = (name: 'dark' | 'light') => {
    update((state) => {
      const next = name === 'light' ? lightTheme : darkTheme;
      if (typeof document !== 'undefined') {
        applyTheme(next);
      }
      return { ...state, theme: next };
    });
  };

  const reset = () => set(initial);

  return {
    subscribe,
    state: { subscribe },
    registerPanel,
    openPanel,
    closePanel,
    toggleWindowMode,
    moveWindow,
    resizeWindow,
    focusWindow,
    dockPanel,
    attachPanelToTabs,
    setDragging,
    setActiveTabId,
    updateSplitSizes,
    registerMenu,
    setTheme,
    reset,
    darkTheme,
    lightTheme
  };
};

export const ui = createUI();

export const windowedPanels = derived(ui, ($ui) => $ui.windows);
export const layoutTree = derived(ui, ($ui) => $ui.layout);
export const menuBar = derived(ui, ($ui) => $ui.menuBar);
export const activeTheme = derived(ui, ($ui) => $ui.theme);
