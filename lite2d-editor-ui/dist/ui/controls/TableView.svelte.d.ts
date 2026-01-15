declare const TableView: import("svelte").Component<{
    columns?: {
        key: string;
        label: string;
        width?: number;
    }[];
    rows?: Record<string, string | number>[];
}, {}, "">;
type TableView = ReturnType<typeof TableView>;
export default TableView;
