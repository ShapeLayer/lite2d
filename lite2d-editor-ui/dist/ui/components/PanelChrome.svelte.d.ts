interface $$__sveltets_2_IsomorphicComponent<Props extends Record<string, any> = any, Events extends Record<string, any> = any, Slots extends Record<string, any> = any, Exports = {}, Bindings = string> {
    new (options: import('svelte').ComponentConstructorOptions<Props>): import('svelte').SvelteComponent<Props, Events, Slots> & {
        $$bindings?: Bindings;
    } & Exports;
    (internal: unknown, props: Props & {
        $$events?: Events;
        $$slots?: Slots;
    }): Exports & {
        $set?: any;
        $on?: any;
    };
    z_$$bindings?: Bindings;
}
declare const PanelChrome: $$__sveltets_2_IsomorphicComponent<{
    title: any;
    panelId: any;
    isWindow?: boolean;
    showMenu?: boolean;
}, {
    close: CustomEvent<void>;
    toggle: CustomEvent<void>;
    menu: CustomEvent<void>;
    dragstart: CustomEvent<{
        event: DragEvent;
        panelId: string;
    }>;
    pointerdown: CustomEvent<PointerEvent>;
}, {}, {}, "">;
type PanelChrome = InstanceType<typeof PanelChrome>;
export default PanelChrome;
