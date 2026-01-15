<script lang="ts">
  import { menuBar, ui } from '../store';
  import type { MenuGroup, MenuItem } from '../types';

  let openMenu: string | null = $state(null);
  let menus = $state<MenuGroup[]>([]);

  $effect(() => {
    const unsub = menuBar.subscribe((value) => (menus = value));
    return () => unsub();
  });

  const toggleMenu = (id: string) => {
    openMenu = openMenu === id ? null : id;
  };

  const handleItem = (item: MenuItem) => {
    if (item.enabled === false) return;
    item.onSelect?.();
    openMenu = null;
  };
</script>

{#if menus.length > 0}
  <nav class="menu-bar" onmouseleave={() => (openMenu = null)}>
    {#each menus as menu}
      <div class="menu">
        <button class:active={openMenu === menu.id} onclick={() => toggleMenu(menu.id)}>{menu.label}</button>
        {#if openMenu === menu.id}
          <div class="menu-list">
            {#each menu.items as item}
              {#if item.separator}
                <div class="separator"></div>
              {:else}
                <button class="item" class:disabled={item.enabled === false} onclick={() => handleItem(item)}>
                  <span>{item.label}</span>
                  {#if item.submenu?.length}
                    <span class="arrow">▶</span>
                  {/if}
                </button>
                {#if item.submenu?.length}
                  <div class="submenu">
                    {#each item.submenu as child}
                      {#if child.separator}
                        <div class="separator"></div>
                      {:else}
                        <button class="item" class:disabled={child.enabled === false} onclick={() => handleItem(child)}>
                          <span>{child.label}</span>
                        </button>
                      {/if}
                    {/each}
                  </div>
                {/if}
              {/if}
            {/each}
          </div>
        {/if}
      </div>
    {/each}
  </nav>
{/if}

<style>
  .menu-bar {
    display: flex;
    align-items: center;
    gap: 6px;
    padding: 6px 12px;
    background: var(--ui-surface);
    border-bottom: var(--ui-border-width) solid var(--ui-border);
    box-shadow: 0 4px 16px rgba(0, 0, 0, 0.12);
    position: sticky;
    top: 0;
    z-index: 5;
  }

  .menu {
    position: relative;
  }

  .menu > button {
    background: transparent;
    border: none;
    color: var(--ui-text);
    font-weight: 600;
    padding: 6px 10px;
    border-radius: var(--ui-radius);
    cursor: pointer;
    transition: background 120ms ease;
  }

  .menu > button.active,
  .menu > button:hover {
    background: var(--ui-highlight);
  }

  .menu-list {
    position: absolute;
    top: calc(100% + 6px);
    left: 0;
    background: var(--ui-surface);
    border: var(--ui-border-width) solid var(--ui-border);
    border-radius: var(--ui-radius);
    padding: 6px;
    min-width: 180px;
    box-shadow: 0 10px 30px rgba(0, 0, 0, 0.2);
    z-index: 20;
  }

  .item {
    width: 100%;
    display: flex;
    align-items: center;
    justify-content: space-between;
    background: transparent;
    border: none;
    color: var(--ui-text);
    padding: 8px 10px;
    border-radius: var(--ui-radius);
    cursor: pointer;
    text-align: left;
  }

  .item:hover:not(.disabled) {
    background: var(--ui-highlight);
  }

  .item.disabled {
    opacity: 0.5;
    cursor: not-allowed;
  }

  .separator {
    height: 1px;
    background: var(--ui-border);
    margin: 6px 0;
  }

  .submenu {
    margin-left: 10px;
    border-left: var(--ui-border-width) solid var(--ui-border);
    padding-left: 10px;
  }

  .arrow {
    font-size: 11px;
    color: var(--ui-text-muted);
  }
</style>
