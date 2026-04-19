// AstraLang — hello world example

fn add(a: i64, b: i64) -> i64 {
    return a + b;
}

fn main() -> void {
    let x: i64 = 10;
    let y: i64 = 32;
    let result: i64 = add(x, y);
    println(result);

    let mut counter: i64 = 0;
    while counter < 5 {
        println(counter);
        counter = counter + 1;
    }

    if result == 42 {
        println("the answer is 42");
    } else {
        println("not 42");
    }
}
