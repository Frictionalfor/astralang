// AstraLang Standard Library — io module
// Provides: print, println, eprintln, read_line (extern bridge)

pub mod io {
    extern fn print_str(s: str) -> void;
    extern fn print_i64(n: i64) -> void;
    extern fn print_f64(f: f64) -> void;
    extern fn read_line() -> str;
    extern fn flush() -> void;

    pub fn println_str(s: str) -> void {
        println(s);
    }

    pub fn println_i64(n: i64) -> void {
        println(n);
    }

    pub fn println_f64(f: f64) -> void {
        println(f);
    }
}
