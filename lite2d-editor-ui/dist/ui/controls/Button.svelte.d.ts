declare const Button: import("svelte").Component<{
    variant?: string;
    disabled?: boolean;
    block?: boolean;
    label?: string;
}, {}, "">;
type Button = ReturnType<typeof Button>;
export default Button;
