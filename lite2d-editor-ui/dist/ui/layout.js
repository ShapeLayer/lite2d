let nodeCounter = 0;
const nextId = (prefix) => `${prefix}-${++nodeCounter}`;
export const makeTabs = (tabId) => ({
    kind: 'tabs',
    id: nextId('tabs'),
    tabs: [tabId],
    activeTabId: tabId
});
export const makeSplit = (direction, children, sizes) => {
    const count = children.length;
    const defaultSizes = Array.from({ length: count }, () => 1 / count);
    return {
        kind: 'split',
        id: nextId('split'),
        direction,
        sizes: sizes ?? defaultSizes,
        children
    };
};
export const layoutContainsPanel = (node, panelId) => {
    if (!node)
        return false;
    if (node.kind === 'tabs') {
        return node.tabs.includes(panelId);
    }
    return node.children.some((child) => layoutContainsPanel(child, panelId));
};
export const removePanelFromLayout = (node, panelId) => {
    if (!node)
        return null;
    if (node.kind === 'tabs') {
        const remaining = node.tabs.filter((id) => id !== panelId);
        if (remaining.length === 0)
            return null;
        return { ...node, tabs: remaining, activeTabId: remaining.includes(node.activeTabId) ? node.activeTabId : remaining[0] };
    }
    const nextChildren = node.children
        .map((child) => removePanelFromLayout(child, panelId))
        .filter((child) => !!child);
    if (nextChildren.length === 0)
        return null;
    if (nextChildren.length === 1)
        return nextChildren[0];
    const total = node.sizes.reduce((acc, size, index) => acc + (nextChildren[index] ? size : 0), 0);
    const sizes = node.sizes
        .map((size, index) => (nextChildren[index] ? size / total : 0))
        .filter((size) => size > 0);
    return { ...node, children: nextChildren, sizes };
};
export const addPanelToTabs = (node, panelId) => {
    if (!node)
        return makeTabs(panelId);
    if (node.kind === 'tabs') {
        if (node.tabs.includes(panelId))
            return node;
        return { ...node, tabs: [...node.tabs, panelId], activeTabId: panelId };
    }
    return { ...node, children: [node.children[0], addPanelToTabs(node.children[1] ?? null, panelId), ...node.children.slice(2)] };
};
export const dockPanelAtRoot = (node, panelId, zone) => {
    if (zone === 'center') {
        return addPanelToTabs(node, panelId);
    }
    if (!node) {
        // Nothing docked yet; just create a single tabs container for this panel.
        return makeTabs(panelId);
    }
    const newTabs = makeTabs(panelId);
    const direction = zone === 'left' || zone === 'right' ? 'horizontal' : 'vertical';
    // If the root is already a split in the same direction, insert the new pane into it
    if (node && node.kind === 'split' && node.direction === direction) {
        const isBefore = zone === 'left' || zone === 'top';
        const scaled = node.sizes.map((size) => size * 0.65);
        const children = isBefore ? [newTabs, ...node.children] : [...node.children, newTabs];
        const sizes = isBefore ? [0.35, ...scaled] : [...scaled, 0.35];
        const total = sizes.reduce((acc, size) => acc + size, 0) || 1;
        return { ...node, children, sizes: sizes.map((size) => size / total) };
    }
    const first = zone === 'left' || zone === 'top' ? newTabs : node ?? newTabs;
    const second = zone === 'left' || zone === 'top' ? node ?? newTabs : newTabs;
    const sizes = zone === 'left' || zone === 'top' ? [0.35, 0.65] : [0.65, 0.35];
    return makeSplit(direction, [first, second], sizes);
};
const replaceTabsNode = (node, tabsId, replacement) => {
    if (node.kind === 'tabs') {
        return node.id === tabsId ? replacement : node;
    }
    return { ...node, children: node.children.map((child) => replaceTabsNode(child, tabsId, replacement)) };
};
export const dockPanelAtTabs = (node, panelId, targetTabsId, zone) => {
    if (zone === 'center') {
        return insertIntoTabs(node, targetTabsId, panelId);
    }
    const target = findTabsNode(node, targetTabsId);
    if (!target)
        return dockPanelAtRoot(node, panelId, zone);
    const newTabs = makeTabs(panelId);
    const direction = zone === 'left' || zone === 'right' ? 'horizontal' : 'vertical';
    const isBefore = zone === 'left' || zone === 'top';
    const sizes = isBefore ? [0.35, 0.65] : [0.65, 0.35];
    const split = makeSplit(direction, isBefore ? [newTabs, target] : [target, newTabs], sizes);
    return replaceTabsNode(node, targetTabsId, split);
};
export const replaceTab = (node, targetTabId, newTabId) => {
    if (node.kind === 'tabs') {
        if (!node.tabs.includes(targetTabId))
            return node;
        const nextTabs = node.tabs.filter((tab) => tab !== targetTabId);
        nextTabs.push(newTabId);
        return { ...node, tabs: nextTabs, activeTabId: newTabId };
    }
    return { ...node, children: node.children.map((child) => replaceTab(child, targetTabId, newTabId)) };
};
export const movePanelToTabs = (node, sourceId, targetTabsId) => {
    const cleaned = removePanelFromLayout(node, sourceId);
    const target = findTabsNode(cleaned, targetTabsId);
    if (!target)
        return cleaned ?? makeTabs(sourceId);
    const updated = insertIntoTabs(cleaned, targetTabsId, sourceId);
    return updated;
};
export const findTabsNode = (node, tabsId) => {
    if (!node)
        return null;
    if (node.kind === 'tabs' && node.id === tabsId)
        return node;
    if (node.kind === 'split') {
        for (const child of node.children) {
            const found = findTabsNode(child, tabsId);
            if (found)
                return found;
        }
    }
    return null;
};
export const insertIntoTabs = (node, targetTabsId, panelId) => {
    if (node.kind === 'tabs' && node.id === targetTabsId) {
        if (node.tabs.includes(panelId))
            return node;
        return { ...node, tabs: [...node.tabs, panelId], activeTabId: panelId };
    }
    if (node.kind === 'split') {
        return { ...node, children: node.children.map((child) => insertIntoTabs(child, targetTabsId, panelId)) };
    }
    return node;
};
export const setActiveTab = (node, tabsId, panelId) => {
    if (node.kind === 'tabs' && node.id === tabsId) {
        return { ...node, activeTabId: panelId };
    }
    if (node.kind === 'split') {
        return { ...node, children: node.children.map((child) => setActiveTab(child, tabsId, panelId)) };
    }
    return node;
};
export const setSplitSizes = (node, splitId, sizes) => {
    if (node.kind === 'split' && node.id === splitId) {
        const total = sizes.reduce((acc, size) => acc + size, 0) || 1;
        return { ...node, sizes: sizes.map((size) => Math.max(0.1, size / total)) };
    }
    if (node.kind === 'split') {
        return { ...node, children: node.children.map((child) => setSplitSizes(child, splitId, sizes)) };
    }
    return node;
};
