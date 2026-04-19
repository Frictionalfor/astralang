// AstraLang — struct and module example

struct Point {
    x: i64,
    y: i64,
}

fn distance_sq(p: Point) -> i64 {
    return p.x * p.x + p.y * p.y;
}

mod math {
    fn factorial(n: i64) -> i64 {
        if n <= 1 {
            return 1;
        }
        return n * factorial(n - 1);
    }
}

fn main() -> void {
    let n: i64 = 10;
    let fact: i64 = math::factorial(n);
    println(fact);
}
