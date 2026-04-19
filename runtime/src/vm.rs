/// AstraLang stack-based virtual machine — v1.2
use std::collections::HashMap;
use crate::debugger::{Debugger, DebugAction};

// ---- IR types (mirror of astra_ir.h v1.2) ----
#[derive(Debug, Clone, Copy, PartialEq)]
#[repr(u32)]
pub enum OpCode {
    // Stack ops
    PushI64 = 0, PushF64 = 1, PushBool = 2, PushNull = 3,
    PushStr = 4, Pop = 5, Dup = 6, Swap = 7,
    // Locals/Globals
    LoadLocal = 10, StoreLocal = 11, LoadGlobal = 12, StoreGlobal = 13,
    // Integer arithmetic
    IAdd = 20, ISub = 21, IMul = 22, IDiv = 23, IMod = 24, INeg = 25,
    // Float arithmetic
    FAdd = 30, FSub = 31, FMul = 32, FDiv = 33, FNeg = 34,
    // Integer comparison
    IEq = 40, INeq = 41, ILt = 42, IGt = 43, ILte = 44, IGte = 45,
    // Float comparison
    FEq = 50, FNeq = 51, FLt = 52, FGt = 53, FLte = 54, FGte = 55,
    // Logic
    And = 60, Or = 61, Not = 62,
    // Bitwise
    BAnd = 70, BOr = 71, BXor = 72, BNot = 73, LShift = 74, RShift = 75,
    // Control flow
    Jmp = 80, JmpIf = 81, JmpIfNot = 82,
    // Functions
    Call = 90, CallExtern = 91, Return = 92, ReturnVoid = 93, TailCall = 94,
    // Memory
    Alloc = 100, Free = 101, LoadPtr = 102, StorePtr = 103,
    // Strings
    StrConcat = 110, StrLen = 111,
    // Casts
    I2F = 120, F2I = 121, I2B = 122, B2I = 123,
    // I/O
    Print = 130, Println = 131, Eprint = 132,
    // Debug/meta
    Breakpoint = 200, Nop = 201, LineInfo = 202,
    Halt = 255,
}

impl OpCode {
    pub fn from_u32(v: u32) -> Self {
        match v {
            0  => OpCode::PushI64,   1  => OpCode::PushF64,
            2  => OpCode::PushBool,  3  => OpCode::PushNull,
            4  => OpCode::PushStr,   5  => OpCode::Pop,
            6  => OpCode::Dup,       7  => OpCode::Swap,
            10 => OpCode::LoadLocal, 11 => OpCode::StoreLocal,
            12 => OpCode::LoadGlobal,13 => OpCode::StoreGlobal,
            20 => OpCode::IAdd,  21 => OpCode::ISub,  22 => OpCode::IMul,
            23 => OpCode::IDiv,  24 => OpCode::IMod,  25 => OpCode::INeg,
            30 => OpCode::FAdd,  31 => OpCode::FSub,  32 => OpCode::FMul,
            33 => OpCode::FDiv,  34 => OpCode::FNeg,
            40 => OpCode::IEq,   41 => OpCode::INeq,  42 => OpCode::ILt,
            43 => OpCode::IGt,   44 => OpCode::ILte,  45 => OpCode::IGte,
            50 => OpCode::FEq,   51 => OpCode::FNeq,  52 => OpCode::FLt,
            53 => OpCode::FGt,   54 => OpCode::FLte,  55 => OpCode::FGte,
            60 => OpCode::And,   61 => OpCode::Or,    62 => OpCode::Not,
            70 => OpCode::BAnd,  71 => OpCode::BOr,   72 => OpCode::BXor,
            73 => OpCode::BNot,  74 => OpCode::LShift,75 => OpCode::RShift,
            80 => OpCode::Jmp,   81 => OpCode::JmpIf, 82 => OpCode::JmpIfNot,
            90 => OpCode::Call,  91 => OpCode::CallExtern,
            92 => OpCode::Return,93 => OpCode::ReturnVoid, 94 => OpCode::TailCall,
            100=> OpCode::Alloc, 101=> OpCode::Free,
            102=> OpCode::LoadPtr,103=> OpCode::StorePtr,
            110=> OpCode::StrConcat, 111=> OpCode::StrLen,
            120=> OpCode::I2F,   121=> OpCode::F2I,
            122=> OpCode::I2B,   123=> OpCode::B2I,
            130=> OpCode::Print, 131=> OpCode::Println, 132=> OpCode::Eprint,
            200=> OpCode::Breakpoint, 201=> OpCode::Nop, 202=> OpCode::LineInfo,
            255=> OpCode::Halt,
            _  => OpCode::Halt,
        }
    }
    pub fn name(&self) -> &'static str {
        match self {
            OpCode::PushI64    => "PUSH_I64",   OpCode::PushF64    => "PUSH_F64",
            OpCode::PushBool   => "PUSH_BOOL",  OpCode::PushNull   => "PUSH_NULL",
            OpCode::PushStr    => "PUSH_STR",   OpCode::Pop        => "POP",
            OpCode::Dup        => "DUP",        OpCode::Swap       => "SWAP",
            OpCode::LoadLocal  => "LOAD_LOCAL", OpCode::StoreLocal => "STORE_LOCAL",
            OpCode::LoadGlobal => "LOAD_GLOBAL",OpCode::StoreGlobal=> "STORE_GLOBAL",
            OpCode::IAdd       => "IADD",       OpCode::ISub       => "ISUB",
            OpCode::IMul       => "IMUL",       OpCode::IDiv       => "IDIV",
            OpCode::IMod       => "IMOD",       OpCode::INeg       => "INEG",
            OpCode::FAdd       => "FADD",       OpCode::FSub       => "FSUB",
            OpCode::FMul       => "FMUL",       OpCode::FDiv       => "FDIV",
            OpCode::FNeg       => "FNEG",       OpCode::IEq        => "IEQ",
            OpCode::INeq       => "INEQ",       OpCode::ILt        => "ILT",
            OpCode::IGt        => "IGT",        OpCode::ILte       => "ILTE",
            OpCode::IGte       => "IGTE",       OpCode::FEq        => "FEQ",
            OpCode::FNeq       => "FNEQ",       OpCode::FLt        => "FLT",
            OpCode::FGt        => "FGT",        OpCode::FLte       => "FLTE",
            OpCode::FGte       => "FGTE",       OpCode::And        => "AND",
            OpCode::Or         => "OR",         OpCode::Not        => "NOT",
            OpCode::BAnd       => "BAND",       OpCode::BOr        => "BOR",
            OpCode::BXor       => "BXOR",       OpCode::BNot       => "BNOT",
            OpCode::LShift     => "LSHIFT",     OpCode::RShift     => "RSHIFT",
            OpCode::Jmp        => "JMP",        OpCode::JmpIf      => "JMP_IF",
            OpCode::JmpIfNot   => "JMP_IFNOT",  OpCode::Call       => "CALL",
            OpCode::CallExtern => "CALL_EXTERN",OpCode::Return     => "RETURN",
            OpCode::ReturnVoid => "RETURN_VOID",OpCode::TailCall   => "TAIL_CALL",
            OpCode::Alloc      => "ALLOC",      OpCode::Free       => "FREE",
            OpCode::LoadPtr    => "LOAD_PTR",   OpCode::StorePtr   => "STORE_PTR",
            OpCode::StrConcat  => "STR_CONCAT", OpCode::StrLen     => "STR_LEN",
            OpCode::I2F        => "I2F",        OpCode::F2I        => "F2I",
            OpCode::I2B        => "I2B",        OpCode::B2I        => "B2I",
            OpCode::Print      => "PRINT",      OpCode::Println    => "PRINTLN",
            OpCode::Eprint     => "EPRINT",     OpCode::Breakpoint => "BREAKPOINT",
            OpCode::Nop        => "NOP",        OpCode::LineInfo   => "LINE_INFO",
            OpCode::Halt       => "HALT",
            #[allow(unreachable_patterns)]
            _                  => "???",
        }
    }
}

#[derive(Debug, Clone)]
pub struct IRInstr {
    pub op:        OpCode,
    pub operand_a: i64,
    pub operand_b: i64,
}

#[derive(Debug, Clone)]
pub struct IRFunction {
    pub name:        String,
    pub param_count: usize,
    pub local_count: usize,
    pub instrs:      Vec<IRInstr>,
}

#[derive(Debug)]
pub struct IRModule {
    pub functions:        Vec<IRFunction>,
    pub strings:          Vec<String>,
    pub entry_func_index: usize,
}

// ---- Runtime value ----
#[derive(Debug, Clone)]
pub enum Value {
    Int(i64),
    Float(f64),
    Bool(bool),
    Str(String),
    Ptr(usize),
    Null,
}

impl std::fmt::Display for Value {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Value::Int(v)   => write!(f, "{}", v),
            Value::Float(v) => write!(f, "{}", v),
            Value::Bool(v)  => write!(f, "{}", v),
            Value::Str(v)   => write!(f, "{}", v),
            Value::Ptr(v)   => write!(f, "0x{:x}", v),
            Value::Null     => write!(f, "null"),
        }
    }
}

// ---- Call frame ----
struct Frame {
    func_idx: usize,
    ip:       usize,
    locals:   Vec<Value>,
    base:     usize,
}

// ---- VM ----
pub struct VM {
    module:      IRModule,
    stack:       Vec<Value>,
    frames:      Vec<Frame>,
    heap:        Vec<u8>,
    globals:     HashMap<usize, Value>,
    trace:       bool,
    stack_limit: usize,
    debugger:    Option<Debugger>,
}

impl VM {
    pub fn new(module: IRModule, trace: bool, stack_limit: usize, debug_mode: bool) -> Self {
        VM {
            module,
            stack:    Vec::with_capacity(256),
            frames:   Vec::new(),
            heap:     Vec::new(),
            globals:  HashMap::new(),
            trace,
            stack_limit,
            debugger: if debug_mode { Some(Debugger::new()) } else { None },
        }
    }

    pub fn run(&mut self) -> Result<(), String> {
        let entry = self.module.entry_func_index;
        self.call_fn(entry, &[])?;
        self.execute()
    }

    fn call_fn(&mut self, func_idx: usize, args: &[Value]) -> Result<(), String> {
        if func_idx >= self.module.functions.len() {
            return Err(format!("call to invalid function index {}", func_idx));
        }
        let func = &self.module.functions[func_idx];
        let size = func.local_count.max(func.param_count);
        let mut locals = vec![Value::Null; size];
        for (i, a) in args.iter().enumerate() {
            if i < locals.len() { locals[i] = a.clone(); }
        }
        if self.trace {
            eprintln!("[trace] CALL fn[{}] \"{}\" args={}", func_idx, func.name, args.len());
        }
        self.frames.push(Frame { func_idx, ip: 0, locals, base: self.stack.len() });
        Ok(())
    }

    fn push(&mut self, v: Value) -> Result<(), String> {
        if self.stack.len() >= self.stack_limit {
            return Err(format!("stack overflow (limit={})", self.stack_limit));
        }
        self.stack.push(v);
        Ok(())
    }

    fn pop(&mut self) -> Result<Value, String> {
        self.stack.pop().ok_or_else(|| "stack underflow".to_string())
    }

    fn peek_top(&self) -> Result<&Value, String> {
        self.stack.last().ok_or_else(|| "stack empty".to_string())
    }

    // Print a stack trace to stderr
    pub fn print_stack_trace(&self) {
        eprintln!("--- stack trace (depth={}) ---", self.frames.len());
        for (i, frame) in self.frames.iter().enumerate().rev() {
            let name = &self.module.functions[frame.func_idx].name;
            eprintln!("  #{} fn \"{}\" ip={}", i, name, frame.ip);
        }
    }

    fn execute(&mut self) -> Result<(), String> {
        loop {
            let (func_idx, ip) = {
                let frame = self.frames.last().ok_or("no frame")?;
                (frame.func_idx, frame.ip)
            };

            let instr = {
                let func = &self.module.functions[func_idx];
                if ip >= func.instrs.len() {
                    self.frames.pop();
                    if self.frames.is_empty() { return Ok(()); }
                    continue;
                }
                func.instrs[ip].clone()
            };

            if self.trace {
                let fname = &self.module.functions[func_idx].name;
                eprintln!("[trace] {}:{:04}  {:12}  a={} b={}  stack_depth={}",
                    fname, ip, instr.op.name(),
                    instr.operand_a, instr.operand_b,
                    self.stack.len());
            }

            // debugger hook
            if let Some(ref mut dbg) = self.debugger {
                if dbg.should_pause(func_idx, ip) {
                    let fname = &self.module.functions[func_idx].name;
                    let frame_info: Vec<(usize, &str, usize)> = self.frames.iter()
                        .map(|f| (f.func_idx, self.module.functions[f.func_idx].name.as_str(), f.ip))
                        .collect();
                    let locals = self.frames.last()
                        .map(|f| f.locals.as_slice())
                        .unwrap_or(&[]);
                    let action = dbg.prompt(fname, func_idx, ip,
                                            &self.stack, locals, &frame_info);
                    match action {
                        DebugAction::Quit     => return Ok(()),
                        DebugAction::Step     => {}
                        DebugAction::Continue => {}
                    }
                }
            }

            self.frames.last_mut().unwrap().ip += 1;

            match instr.op {
                OpCode::PushI64  => self.push(Value::Int(instr.operand_a))?,
                OpCode::PushF64  => {
                    let f = f64::from_bits(instr.operand_a as u64);
                    self.push(Value::Float(f))?;
                }
                OpCode::PushBool => self.push(Value::Bool(instr.operand_a != 0))?,
                OpCode::PushNull => self.push(Value::Null)?,
                OpCode::PushStr  => {
                    let s = self.module.strings.get(instr.operand_a as usize)
                        .cloned().unwrap_or_default();
                    self.push(Value::Str(s))?;
                }

                OpCode::Pop => { self.pop()?; }
                OpCode::Dup => {
                    let v = self.peek_top()?.clone();
                    self.push(v)?;
                }

                OpCode::LoadLocal => {
                    let slot = instr.operand_a as usize;
                    let v = self.frames.last().unwrap().locals
                        .get(slot).cloned().unwrap_or(Value::Null);
                    self.push(v)?;
                }
                OpCode::StoreLocal => {
                    let slot = instr.operand_a as usize;
                    let v = self.pop()?;
                    let frame = self.frames.last_mut().unwrap();
                    if slot >= frame.locals.len() { frame.locals.resize(slot + 1, Value::Null); }
                    frame.locals[slot] = v;
                }

                OpCode::LoadGlobal => {
                    let v = self.globals.get(&(instr.operand_a as usize))
                        .cloned().unwrap_or(Value::Null);
                    self.push(v)?;
                }
                OpCode::StoreGlobal => {
                    let v = self.pop()?;
                    self.globals.insert(instr.operand_a as usize, v);
                }

                // Integer arithmetic
                OpCode::IAdd => { let (b,a)=(self.pop()?,self.pop()?); self.push(int_binop(a,b,|x,y|x+y)?)? }
                OpCode::ISub => { let (b,a)=(self.pop()?,self.pop()?); self.push(int_binop(a,b,|x,y|x-y)?)? }
                OpCode::IMul => { let (b,a)=(self.pop()?,self.pop()?); self.push(int_binop(a,b,|x,y|x*y)?)? }
                OpCode::IDiv => {
                    let (b,a) = (self.pop()?, self.pop()?);
                    if let Value::Int(bv) = &b { if *bv == 0 { return Err("division by zero".into()); } }
                    self.push(int_binop(a, b, |x,y| x/y)?)?;
                }
                OpCode::IMod => { let (b,a)=(self.pop()?,self.pop()?); self.push(int_binop(a,b,|x,y|x%y)?)? }
                OpCode::INeg => {
                    match self.pop()? {
                        Value::Int(i) => self.push(Value::Int(-i))?,
                        _ => return Err("INeg on non-int".into()),
                    }
                }

                // Float arithmetic
                OpCode::FAdd => { let (b,a)=(self.pop()?,self.pop()?); self.push(flt_binop(a,b,|x,y|x+y)?)? }
                OpCode::FSub => { let (b,a)=(self.pop()?,self.pop()?); self.push(flt_binop(a,b,|x,y|x-y)?)? }
                OpCode::FMul => { let (b,a)=(self.pop()?,self.pop()?); self.push(flt_binop(a,b,|x,y|x*y)?)? }
                OpCode::FDiv => { let (b,a)=(self.pop()?,self.pop()?); self.push(flt_binop(a,b,|x,y|x/y)?)? }
                OpCode::FNeg => {
                    match self.pop()? {
                        Value::Float(f) => self.push(Value::Float(-f))?,
                        _ => return Err("FNeg on non-float".into()),
                    }
                }

                // Comparisons
                OpCode::IEq  => { let (b,a)=(self.pop()?,self.pop()?); self.push(int_cmp(a,b,|x,y|x==y)?)? }
                OpCode::INeq => { let (b,a)=(self.pop()?,self.pop()?); self.push(int_cmp(a,b,|x,y|x!=y)?)? }
                OpCode::ILt  => { let (b,a)=(self.pop()?,self.pop()?); self.push(int_cmp(a,b,|x,y|x<y)?)? }
                OpCode::IGt  => { let (b,a)=(self.pop()?,self.pop()?); self.push(int_cmp(a,b,|x,y|x>y)?)? }
                OpCode::ILte => { let (b,a)=(self.pop()?,self.pop()?); self.push(int_cmp(a,b,|x,y|x<=y)?)? }
                OpCode::IGte => { let (b,a)=(self.pop()?,self.pop()?); self.push(int_cmp(a,b,|x,y|x>=y)?)? }

                // Logic
                OpCode::And => {
                    let (b,a) = (self.pop()?, self.pop()?);
                    self.push(Value::Bool(is_truthy(&a) && is_truthy(&b)))?;
                }
                OpCode::Or => {
                    let (b,a) = (self.pop()?, self.pop()?);
                    self.push(Value::Bool(is_truthy(&a) || is_truthy(&b)))?;
                }
                OpCode::Not => {
                    let v = self.pop()?;
                    self.push(Value::Bool(!is_truthy(&v)))?;
                }

                // Bitwise
                OpCode::BAnd   => { let (b,a)=(self.pop()?,self.pop()?); self.push(int_binop(a,b,|x,y|x&y)?)? }
                OpCode::BOr    => { let (b,a)=(self.pop()?,self.pop()?); self.push(int_binop(a,b,|x,y|x|y)?)? }
                OpCode::BXor   => { let (b,a)=(self.pop()?,self.pop()?); self.push(int_binop(a,b,|x,y|x^y)?)? }
                OpCode::BNot   => {
                    match self.pop()? {
                        Value::Int(i) => self.push(Value::Int(!i))?,
                        _ => return Err("BNot on non-int".into()),
                    }
                }
                OpCode::LShift => { let (b,a)=(self.pop()?,self.pop()?); self.push(int_binop(a,b,|x,y|x<<y)?)? }
                OpCode::RShift => { let (b,a)=(self.pop()?,self.pop()?); self.push(int_binop(a,b,|x,y|x>>y)?)? }

                // Control flow
                OpCode::Jmp => {
                    self.frames.last_mut().unwrap().ip = instr.operand_a as usize;
                }
                OpCode::JmpIf => {
                    let v = self.pop()?;
                    if is_truthy(&v) {
                        self.frames.last_mut().unwrap().ip = instr.operand_a as usize;
                    }
                }
                OpCode::JmpIfNot => {
                    let v = self.pop()?;
                    if !is_truthy(&v) {
                        self.frames.last_mut().unwrap().ip = instr.operand_a as usize;
                    }
                }

                // Function calls
                OpCode::Call => {
                    let fidx     = instr.operand_a as usize;
                    let argc     = instr.operand_b as usize;
                    let mut args = vec![Value::Null; argc];
                    for i in (0..argc).rev() { args[i] = self.pop()?; }
                    self.call_fn(fidx, &args)?;
                }
                OpCode::Return => {
                    let ret_val = self.pop()?;
                    let base    = self.frames.last().unwrap().base;
                    self.frames.pop();
                    self.stack.truncate(base);
                    self.push(ret_val)?;
                    if self.frames.is_empty() { return Ok(()); }
                }
                OpCode::ReturnVoid => {
                    let base = self.frames.last().unwrap().base;
                    self.frames.pop();
                    self.stack.truncate(base);
                    if self.frames.is_empty() { return Ok(()); }
                }

                // Memory
                OpCode::Alloc => {
                    let size = instr.operand_a as usize;
                    let ptr  = self.heap.len();
                    self.heap.resize(ptr + size, 0);
                    self.push(Value::Ptr(ptr))?;
                }
                OpCode::Free => { self.pop()?; }
                OpCode::LoadPtr => {
                    let idx  = self.pop()?;
                    let base = self.pop()?;
                    match (base, idx) {
                        (Value::Ptr(p), Value::Int(i)) => {
                            let addr = p + i as usize;
                            let v = if addr < self.heap.len() { self.heap[addr] as i64 } else { 0 };
                            self.push(Value::Int(v))?;
                        }
                        _ => return Err("LoadPtr: invalid operands".into()),
                    }
                }
                OpCode::StorePtr => {
                    let val  = self.pop()?;
                    let idx  = self.pop()?;
                    let base = self.pop()?;
                    if let (Value::Ptr(p), Value::Int(i), Value::Int(v)) = (base, idx, val) {
                        let addr = p + i as usize;
                        if addr < self.heap.len() { self.heap[addr] = v as u8; }
                    }
                }

                // Strings
                OpCode::StrConcat => {
                    let (b, a) = (self.pop()?, self.pop()?);
                    match (a, b) {
                        (Value::Str(sa), Value::Str(sb)) => self.push(Value::Str(sa + &sb))?,
                        _ => return Err("StrConcat: non-string operands".into()),
                    }
                }

                // Casts
                OpCode::I2F => {
                    match self.pop()? {
                        Value::Int(i) => self.push(Value::Float(i as f64))?,
                        _ => return Err("I2F: not an int".into()),
                    }
                }
                OpCode::F2I => {
                    match self.pop()? {
                        Value::Float(f) => self.push(Value::Int(f as i64))?,
                        _ => return Err("F2I: not a float".into()),
                    }
                }

                // I/O
                OpCode::Print => {
                    let v = self.pop()?;
                    print!("{}", v);
                }
                OpCode::Println => {
                    let v = self.pop()?;
                    println!("{}", v);
                }
                OpCode::Eprint => {
                    let v = self.pop()?;
                    eprintln!("{}", v);
                }

                // New casts
                OpCode::I2B => {
                    match self.pop()? {
                        Value::Int(i) => self.push(Value::Bool(i != 0))?,
                        _ => return Err("I2B: not an int".into()),
                    }
                }
                OpCode::B2I => {
                    match self.pop()? {
                        Value::Bool(b) => self.push(Value::Int(if b { 1 } else { 0 }))?,
                        _ => return Err("B2I: not a bool".into()),
                    }
                }

                // Swap
                OpCode::Swap => {
                    let a = self.pop()?;
                    let b = self.pop()?;
                    self.push(a)?;
                    self.push(b)?;
                }

                // String length
                OpCode::StrLen => {
                    match self.pop()? {
                        Value::Str(s) => self.push(Value::Int(s.len() as i64))?,
                        _ => return Err("STR_LEN: not a string".into()),
                    }
                }

                // Tail call (same as call for now — full TCO is a future pass)
                OpCode::TailCall => {
                    let fidx = instr.operand_a as usize;
                    let argc = instr.operand_b as usize;
                    let mut args = vec![Value::Null; argc];
                    for i in (0..argc).rev() { args[i] = self.pop()?; }
                    // pop current frame and replace with new call
                    let base = self.frames.last().unwrap().base;
                    self.frames.pop();
                    self.stack.truncate(base);
                    self.call_fn(fidx, &args)?;
                }

                // Debug/meta
                OpCode::Nop      => {}  // intentional no-op
                OpCode::LineInfo => {}  // consumed by debugger/trace only
                OpCode::Breakpoint => {
                    // force debugger pause on next iteration
                    if let Some(ref mut dbg) = self.debugger {
                        dbg.stepping = true;
                    } else {
                        eprintln!("[breakpoint] fn[{}]:{}", func_idx, ip);
                    }
                }

                OpCode::Halt => return Ok(()),

                #[allow(unreachable_patterns)]
                _ => return Err(format!("unimplemented opcode {:?}", instr.op)),
            }
        }
    }
}

// ---- helpers ----
fn int_binop(a: Value, b: Value, f: impl Fn(i64,i64)->i64) -> Result<Value, String> {
    match (a, b) {
        (Value::Int(x), Value::Int(y)) => Ok(Value::Int(f(x, y))),
        _ => Err("int_binop: non-integer operands".into()),
    }
}

fn flt_binop(a: Value, b: Value, f: impl Fn(f64,f64)->f64) -> Result<Value, String> {
    match (a, b) {
        (Value::Float(x), Value::Float(y)) => Ok(Value::Float(f(x, y))),
        (Value::Int(x),   Value::Int(y))   => Ok(Value::Float(f(x as f64, y as f64))),
        _ => Err("flt_binop: non-numeric operands".into()),
    }
}

fn int_cmp(a: Value, b: Value, f: impl Fn(i64,i64)->bool) -> Result<Value, String> {
    match (a, b) {
        (Value::Int(x), Value::Int(y)) => Ok(Value::Bool(f(x, y))),
        _ => Err("int_cmp: non-integer operands".into()),
    }
}

fn is_truthy(v: &Value) -> bool {
    match v {
        Value::Bool(b)  => *b,
        Value::Int(i)   => *i != 0,
        Value::Float(f) => *f != 0.0,
        Value::Null     => false,
        _               => true,
    }
}
