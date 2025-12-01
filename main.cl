module program;

struct Vector2
{
    i32 x;
    i32 y;
    
    i32 square_length()
    {
        return x * x + y * y;
    }
}

i32 main() {
    i32 n = 10;
    i32 result = combined(n);
    return 0;
}
