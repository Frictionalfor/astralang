/// AstraLang bytecode loader
/// Reads .asc files produced by astrac.
///
/// File format (v1.2):
///   [4]  magic   "ASTR"
///   [4]  version u32  (major<<16 | minor)
///   [4]  flags   u32
///   [4]  str_count
///   ...  string pool
///   [4]  func_count
///   ...  functions
///   [4]  entry_func_index
///
/// IRInstr on disk: op(u32) + reserved(u32) + operand_a(i64) + operand_b(i64) = 24 bytes

use std::fs::File;
use std::io::{self, Read, BufReader};
use crate::vm::{IRModule, IRFunction, IRInstr, OpCode};

const BYTECODE_MAJOR: u32 = 1;

fn read_u32(r: &mut impl Read) -> io::Result<u32> {
    let mut buf = [0u8; 4];
    r.read_exact(&mut buf)?;
    Ok(u32::from_le_bytes(buf))
}

fn read_i64(r: &mut impl Read) -> io::Result<i64> {
    let mut buf = [0u8; 8];
    r.read_exact(&mut buf)?;
    Ok(i64::from_le_bytes(buf))
}

pub struct LoadedModule {
    pub module:    IRModule,
    pub flags:     u32,
    pub version:   u32,
}

pub fn load_module(path: &str) -> io::Result<LoadedModule> {
    let file = File::open(path)
        .map_err(|e| io::Error::new(e.kind(), format!("cannot open '{}': {}", path, e)))?;
    let mut r = BufReader::new(file);

    // magic
    let mut magic = [0u8; 4];
    r.read_exact(&mut magic)?;
    if &magic != b"ASTR" {
        return Err(io::Error::new(io::ErrorKind::InvalidData,
            "bad magic bytes — not an AstraLang bytecode file"));
    }

    // version
    let version = read_u32(&mut r)?;
    let file_major = version >> 16;
    if file_major != BYTECODE_MAJOR {
        return Err(io::Error::new(io::ErrorKind::InvalidData,
            format!("bytecode version mismatch: file major={}, runtime major={}",
                    file_major, BYTECODE_MAJOR)));
    }

    // flags
    let flags = read_u32(&mut r)?;

    // string pool
    let str_count = read_u32(&mut r)? as usize;
    let mut strings = Vec::with_capacity(str_count);
    for _ in 0..str_count {
        let len = read_u32(&mut r)? as usize;
        let mut buf = vec![0u8; len];
        r.read_exact(&mut buf)?;
        strings.push(String::from_utf8_lossy(&buf).into_owned());
    }

    // functions
    let func_count = read_u32(&mut r)? as usize;
    let mut functions = Vec::with_capacity(func_count);
    for _ in 0..func_count {
        let mut name_buf = [0u8; 128];
        r.read_exact(&mut name_buf)?;
        let name = String::from_utf8_lossy(&name_buf)
            .trim_end_matches('\0')
            .to_string();

        let param_count = read_u32(&mut r)? as usize;
        let local_count = read_u32(&mut r)? as usize;
        let instr_count = read_u32(&mut r)? as usize;

        let mut instrs = Vec::with_capacity(instr_count);
        for _ in 0..instr_count {
            // IRInstr layout: op(u32) + reserved(u32) + a(i64) + b(i64)
            let op_raw  = read_u32(&mut r)?;
            let _reserved = read_u32(&mut r)?;
            let a       = read_i64(&mut r)?;
            let b       = read_i64(&mut r)?;
            instrs.push(IRInstr {
                op: OpCode::from_u32(op_raw),
                operand_a: a,
                operand_b: b,
            });
        }

        functions.push(IRFunction { name, param_count, local_count, instrs });
    }

    let entry = read_u32(&mut r)? as usize;

    Ok(LoadedModule {
        module: IRModule { functions, strings, entry_func_index: entry },
        flags,
        version,
    })
}
