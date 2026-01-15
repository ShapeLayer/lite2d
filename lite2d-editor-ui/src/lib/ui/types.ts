import type { ComponentType } from 'svelte';

export type Direction = 'horizontal' | 'vertical';
export type DockZone = 'left' | 'right' | 'top' | 'bottom' | 'center';

export type PanelRegistration = {
  id: string;
  title: string;
  component: ComponentType<any> | any;
  icon?: string;
  minWidth?: number;
  minHeight?: number;
};

export type PanelWindow = {
  panelId: string;
  x: number;
  y: number;
  width: number;
  height: number;
  z: number;
};

export type TabsNode = {
  kind: 'tabs';
  id: string;
  tabs: string[];
  activeTabId: string;
};

export type SplitNode = {
  kind: 'split';
  id: string;
  direction: Direction;
  sizes: number[];
  children: LayoutNode[];
};

export type LayoutNode = TabsNode | SplitNode;

export type MenuItem = {
  id: string;
  label: string;
  separator?: boolean;
  enabled?: boolean;
  onSelect?: () => void;
  submenu?: MenuItem[];
  order?: number;
};

export type MenuGroup = {
  id: string;
  label: string;
  items: MenuItem[];
  order?: number;
};

export type Theme = {
  name: string;
  colors: Record<string, string>;
  fontFamily: string;
  radius: number;
  spacing: number;
  borderWidth: number;
};
