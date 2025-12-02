module vector2;

f32 abs(f32 x)
{
    if(x < 0)
    {
        return -x;
    }
}

f32 sqrt(f32 x) 
{
    if(x < 0f) 
        return -1f;    
    if(x == 0f) 
        return 0f;
    
    f32 y = 1f;
    if(x >= 1f)
    {
        x = x / 2f;
    }
    f32 eps = 0.00000001f;
    i32 max_iter = 100;
    
    for(i32 i = 0; i < max_iter; i = i + 1)
    {
        f32 prev = y;
        y = 0.5 * (y + x / y);
        if(abs(y - prev) <= eps * y)
            break;
        i = i + 1;
    }
    
    return y;
}

struct Vector2
{
    f32 x;
    f32 y;
    
    f32 square_length()
    {
        return x * x + y * y;
    }
    
    f32 length()
    {
        return sqrt(square_length());
    }
}

i32 main() {
    return 0;
}
