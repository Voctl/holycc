I64 factorial(I64 n)
{
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

I64 fibonacci(I64 n)
{
    I64 a = 0;
    I64 b = 1;
    I64 i = 0;
    while (i < n) {
        I64 tmp = a + b;
        a = b;
        b = tmp;
        i = i + 1;
    }
    return a;
}

Bool is_even(I64 x)
{
    return x % 2 == 0;
}

F64 average(F64 a, F64 b)
{
    return (a + b) / 2.0;
}

enum Colors {
    RED,
    GREEN = 5,
    BLUE
};

class Vec2 {
    F64 x;
    F64 y;
};
