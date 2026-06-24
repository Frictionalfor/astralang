#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

ASTRAC="$ROOT/build/astrac"
ASTRA="$ROOT/runtime/target/release/astra"

pass_count=0

write_case() {
    local name="$1"
    local body="$2"
    printf '%s\n' "$body" > "$TMP/$name.as"
}

expect_check_fail() {
    local name="$1"
    local body="$2"
    local needle="$3"
    write_case "$name" "$body"
    if "$ASTRAC" check "$TMP/$name.as" >"$TMP/$name.out" 2>&1; then
        echo "FAIL: $name unexpectedly passed" >&2
        cat "$TMP/$name.out" >&2
        exit 1
    fi
    if ! grep -q "$needle" "$TMP/$name.out"; then
        echo "FAIL: $name did not mention '$needle'" >&2
        cat "$TMP/$name.out" >&2
        exit 1
    fi
    pass_count=$((pass_count + 1))
}

expect_run_output() {
    local name="$1"
    local body="$2"
    local expected="$3"
    local opt_flag="${4:-}"
    write_case "$name" "$body"
    if [[ -n "$opt_flag" ]]; then
        "$ASTRAC" build "$TMP/$name.as" "$opt_flag" -o "$TMP/$name.asc" >"$TMP/$name.build" 2>&1
    else
        "$ASTRAC" build "$TMP/$name.as" -o "$TMP/$name.asc" >"$TMP/$name.build" 2>&1
    fi
    "$ASTRA" "$TMP/$name.asc" >"$TMP/$name.out" 2>&1
    if [[ "$(cat "$TMP/$name.out")" != "$expected" ]]; then
        echo "FAIL: $name output mismatch" >&2
        echo "expected: $expected" >&2
        echo "actual: $(cat "$TMP/$name.out")" >&2
        exit 1
    fi
    pass_count=$((pass_count + 1))
}

expect_run_output "extern_indexing" '
extern fn host() -> void;

fn main() -> void {
    println(7);
}
' '7'

expect_check_fail "missing_main" '
fn helper() -> void {
    println(1);
}
' "missing required entrypoint"

expect_check_fail "var_type_mismatch" '
fn main() -> void {
    let x: i64 = "wrong";
}
' "type mismatch"

expect_check_fail "invalid_assignment_target" '
fn main() -> void {
    let mut x: i64 = 1;
    (x + 1) = 2;
}
' "invalid assignment target"

expect_check_fail "return_type_mismatch" '
fn bad() -> i64 {
    return "wrong";
}

fn main() -> void {
    println(1);
}
' "return type mismatch"

expect_check_fail "undefined_identifier" '
fn main() -> void {
    println(missing);
}
' "undefined symbol"

expect_check_fail "invalid_function_call" '
fn add(a: i64, b: i64) -> i64 {
    return a + b;
}

fn main() -> void {
    println(add(1));
}
' "expects 2 arguments"

expect_check_fail "mutability_violation" '
fn main() -> void {
    let x: i64 = 1;
    x = 2;
}
' "immutable binding"

expect_check_fail "assignment_type_mismatch" '
fn main() -> void {
    let mut x: i64 = 1;
    x = "wrong";
}
' "assignment type mismatch"

expect_run_output "short_circuit_and" '
fn main() -> void {
    if false && (1 / 0 == 0) {
        println(0);
    } else {
        println(1);
    }
}
' '1' '--no-opt'

expect_run_output "short_circuit_or" '
fn main() -> void {
    if true || (1 / 0 == 0) {
        println(1);
    } else {
        println(0);
    }
}
' '1' '--no-opt'

write_case "disasm_source" '
fn main() -> void {
    println(3);
}
'
"$ASTRAC" build "$TMP/disasm_source.as" -o "$TMP/disasm_source.asc" >/dev/null 2>&1
INJECT_MARK="$ROOT/injected_marker.asc"
rm -f "$INJECT_MARK"
EVIL="$TMP/evil;touch injected_marker.asc"
cp "$TMP/disasm_source.asc" "$EVIL"
"$ASTRAC" disasm "$EVIL" >"$TMP/disasm.out" 2>&1
if [[ -e "$INJECT_MARK" ]]; then
    echo "FAIL: disasm filename executed through a shell" >&2
    rm -f "$INJECT_MARK"
    exit 1
fi
rm -f "$INJECT_MARK"
pass_count=$((pass_count + 1))

echo "regression tests passed ($pass_count)"
