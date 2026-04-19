// AstraLang Standard Library — math module

pub mod math {
    pub fn abs(n: i64) -> i64 {
        if n < 0 {
            return -n;
        }
        return n;
    }

    pub fn min(a: i64, b: i64) -> i64 {
        if a < b { return a; }
        return b;
    }

    pub fn max(a: i64, b: i64) -> i64 {
        if a > b { return a; }
        return b;
    }

    pub fn pow(base: i64, exp: i64) -> i64 {
        let mut result: i64 = 1;
        let mut e: i64 = exp;
        while e > 0 {
            result = result * base;
            e = e - 1;
        }
        return result;
    }

    pub fn clamp(val: i64, lo: i64, hi: i64) -> i64 {
        if val < lo { return lo; }
        if val > hi { return hi; }
        return val;
    }
}
