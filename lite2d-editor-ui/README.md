# GUI Editor UI System

Dockable panel system inspired by Unity/Photoshop, built with SvelteKit + TypeScript. Includes tabs, splitters, floating windows, menu bar, themes, and reusable controls (button, checkbox, radio, select, slider, text input, tree view, table view).

## Quick start

```sh
npm install
npm run dev -- --open
```

Open the demo page to drag panels, dock to edges, create floating windows, and reopen panels from the Window menu.

## Core API

Available from `$lib`:

- `PanelSurface` – renders the dockspace, drop zones, and floating windows.
- `ui.registerPanel(panel)` – register `{ id, title, component, minWidth?, minHeight? }`.
- `ui.openPanel(id)` / `ui.closePanel(id)` – show or hide a panel.
- `ui.toggleWindowMode(id)` – toggle between docked and floating.
- `ui.dockPanel(id, zone)` – dock to `left|right|top|bottom|center` of the root.
- `ui.attachPanelToTabs(id, tabsId)` – move a panel into a specific tab stack.
- `ui.registerMenu(menu)` – add menu groups/items; the Window menu can list panels.
- Themes: `ui.setTheme('dark' | 'light')`, exported `darkTheme`/`lightTheme` and `activeTheme` store.

## Controls

Reusable components exported from `$lib`:

`Button`, `Checkbox`, `Radio`, `Select`, `Slider`, `TextInput`, `TreeView`, `TableView`.

## Demo panels

Found under `src/lib/demo` and registered in [`src/routes/+page.svelte`](src/routes/+page.svelte): Scene, Hierarchy, Inspector, Console, and Project.
