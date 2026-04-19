// AstraLang Standard Library — string module

pub mod string {
    extern fn str_len(s: str)              -> i64;
    extern fn str_concat(a: str, b: str)   -> str;
    extern fn str_slice(s: str, from: i64, to: i64) -> str;
    extern fn str_eq(a: str, b: str)       -> bool;
    extern fn str_contains(s: str, sub: str) -> bool;
    extern fn int_to_str(n: i64)           -> str;
    extern fn float_to_str(f: f64)         -> str;
    extern fn bool_to_str(b: bool)         -> str;
}
