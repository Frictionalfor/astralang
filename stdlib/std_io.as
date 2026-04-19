// AstraLang Standard Library — std::io v1.0
// Status: stable
// Requires: bytecode v1.0+

pub mod io {

    pub fn println_str(s: str) -> void {
        println(s);
    }

    pub fn println_i64(n: i64) -> void {
        println(n);
    }

    pub fn println_f64(f: f64) -> void {
        println(f);
    }

    pub fn println_bool(b: bool) -> void {
        if b {
            println("true");
        } else {
            println("false");
        }
    }

    pub fn print_str(s: str) -> void {
        print(s);
    }

    pub fn print_i64(n: i64) -> void {
        print(n);
    }

    pub fn newline() -> void {
        println("");
    }
}
