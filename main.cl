module program;

i32 fibonacci(i32 n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

i32 sum_even(i32 n) {
    i32 sum = 0;
    
    i32 i = 0;
    while (i <= n) {
        if (i % 2 == 0) {
            sum = sum + i;
        }
        i = i + 1;
    }
    return sum;
}

i32 combined(i32 n) {
    i32 fib = fibonacci(n);
    i32 evenSum = sum_even(n);
    if (fib % 2 == 0) {
        return fib + evenSum;
    } else {
        return fib - evenSum;
    }
}

i32 main() {
    i32 n = 10;
    i32 result = combined(n);
    return 0;
}
