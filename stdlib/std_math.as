// AstraLang Standard Library — std::math v1.0
// Status: stable
// Requires: bytecode v1.0+

pub mod math {

    pub fn abs(n: i64) -> i64 {
        if n < 0 { return -n; }
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

    pub fn factorial(n: i64) -> i64 {
        if n <= 1 { return 1; }
        return n * factorial(n - 1);
    }

    pub fn gcd(a: i64, b: i64) -> i64 {
        let mut x: i64 = a;
        let mut y: i64 = b;
        while y != 0 {
            let mut t: i64 = y;
            y = x - (x / y) * y;
            x = t;
        }
        return x;
    }

    pub fn is_even(n: i64) -> bool {
        return n % 2 == 0;
    }

    pub fn is_odd(n: i64) -> bool {
        return n % 2 != 0;
    }
}
