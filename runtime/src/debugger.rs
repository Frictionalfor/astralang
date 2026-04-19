/// AstraLang bytecode-level debugger
///
/// Activated with --debug flag. Supports:
///   b <fn> <ip>   — set breakpoint at function:instruction
///   s             — step one instruction
///   c             — continue until next breakpoint
///   stack         — print value stack
///   locals        — print current frame locals
///   bt            — print call stack (backtrace)
///   q             — quit

use std::collections::HashSet;
use std::io::{self, BufRead, Write};

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct Breakpoint {
    pub func_idx: usize,
    pub ip:       usize,
}

pub struct Debugger {
    pub breakpoints: HashSet<Breakpoint>,
    pub stepping:    bool,
}

impl Debugger {
    pub fn new() -> Self {
        Debugger {
            breakpoints: HashSet::new(),
            stepping:    true, // start in step mode
        }
    }

    /// Called before each instruction. Returns true if execution should pause.
    pub fn should_pause(&self, func_idx: usize, ip: usize) -> bool {
        if self.stepping { return true; }
        self.breakpoints.contains(&Breakpoint { func_idx, ip })
    }

    /// Interactive REPL. Returns DebugAction.
    pub fn prompt(
        &mut self,
        func_name: &str,
        func_idx:  usize,
        ip:        usize,
        stack:     &[crate::vm::Value],
        locals:    &[crate::vm::Value],
        frames:    &[(usize, &str, usize)], // (func_idx, name, ip)
    ) -> DebugAction {
        let stdin  = io::stdin();
        let stdout = io::stdout();

        loop {
            print!("[dbg] {}:{:04} > ", func_name, ip);
            stdout.lock().flush().ok();

            let mut line = String::new();
            if stdin.lock().read_line(&mut line).is_err() { return DebugAction::Quit; }
            let line = line.trim();

            match line {
                "s" | "step" | "" => {
                    self.stepping = true;
                    return DebugAction::Step;
                }
                "c" | "continue" => {
                    self.stepping = false;
                    return DebugAction::Continue;
                }
                "q" | "quit" => return DebugAction::Quit,
                "stack" => {
                    println!("  stack ({} values):", stack.len());
                    for (i, v) in stack.iter().enumerate().rev().take(10) {
                        println!("    [{:3}] {:?}", i, v);
                    }
                }
                "locals" => {
                    println!("  locals ({}):", locals.len());
                    for (i, v) in locals.iter().enumerate() {
                        println!("    [{}] {:?}", i, v);
                    }
                }
                "bt" | "backtrace" => {
                    println!("  call stack:");
                    for (i, (fi, name, fip)) in frames.iter().enumerate().rev() {
                        println!("    #{} fn[{}] \"{}\" ip={}", i, fi, name, fip);
                    }
                }
                cmd if cmd.starts_with("b ") => {
                    // b <func_idx> <ip>
                    let parts: Vec<&str> = cmd[2..].split_whitespace().collect();
                    if parts.len() == 2 {
                        if let (Ok(fi), Ok(bip)) = (parts[0].parse::<usize>(), parts[1].parse::<usize>()) {
                            self.breakpoints.insert(Breakpoint { func_idx: fi, ip: bip });
                            println!("  breakpoint set at fn[{}]:{}", fi, bip);
                        }
                    } else {
                        println!("  usage: b <func_idx> <ip>");
                    }
                }
                cmd if cmd.starts_with("del ") => {
                    let parts: Vec<&str> = cmd[4..].split_whitespace().collect();
                    if parts.len() == 2 {
                        if let (Ok(fi), Ok(bip)) = (parts[0].parse::<usize>(), parts[1].parse::<usize>()) {
                            self.breakpoints.remove(&Breakpoint { func_idx: fi, ip: bip });
                            println!("  breakpoint removed");
                        }
                    }
                }
                "bp" | "breakpoints" => {
                    println!("  breakpoints ({}):", self.breakpoints.len());
                    for bp in &self.breakpoints {
                        println!("    fn[{}]:{}", bp.func_idx, bp.ip);
                    }
                }
                "help" | "?" => {
                    println!("  s/step     — step one instruction");
                    println!("  c/continue — run until next breakpoint");
                    println!("  b <fi> <ip>— set breakpoint");
                    println!("  del <fi> <ip> — remove breakpoint");
                    println!("  bp         — list breakpoints");
                    println!("  stack      — print value stack");
                    println!("  locals     — print local variables");
                    println!("  bt         — print call stack");
                    println!("  q/quit     — exit");
                }
                _ => println!("  unknown command '{}' — type 'help'", line),
            }
        }
    }
}

#[derive(Debug, PartialEq)]
pub enum DebugAction {
    Step,
    Continue,
    Quit,
}
