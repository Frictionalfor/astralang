mod vm;
mod loader;
mod debugger;

use std::env;

const RUNTIME_VERSION: &str = "1.2.0";

fn print_usage() {
    eprintln!("astra {RUNTIME_VERSION} — AstraLang runtime");
    eprintln!("usage: astra <bytecode.asc> [options]");
    eprintln!("options:");
    eprintln!("  --trace              print every instruction to stderr");
    eprintln!("  --stack-limit <n>    set max stack depth (default: 8192)");
    eprintln!("  --debug              enable breakpoint/step debugger");
    eprintln!("  --version            print runtime version");
}

fn main() {
    let args: Vec<String> = env::args().collect();

    if args.iter().any(|a| a == "--version") {
        println!("astra {RUNTIME_VERSION}");
        return;
    }

    if args.len() < 2 {
        print_usage();
        std::process::exit(1);
    }

    let path = &args[1];

    let trace       = args.iter().any(|a| a == "--trace");
    let debug_mode  = args.iter().any(|a| a == "--debug");
    let stack_limit = args.windows(2)
        .find(|w| w[0] == "--stack-limit")
        .and_then(|w| w[1].parse::<usize>().ok())
        .unwrap_or(8192);

    let loaded = match loader::load_module(path) {
        Ok(m)  => m,
        Err(e) => {
            eprintln!("[ERROR 600] {} — {}", path, e);
            std::process::exit(1);
        }
    };

    if trace {
        let ver = loaded.version;
        eprintln!("[astra] loaded {} (bytecode v{}.{}, flags=0x{:02x})",
            path, ver >> 16, ver & 0xffff, loaded.flags);
    }

    let mut vm = vm::VM::new(loaded.module, trace, stack_limit, debug_mode);

    if let Err(e) = vm.run() {
        eprintln!("[runtime error] {}", e);
        vm.print_stack_trace();
        std::process::exit(1);
    }
}
