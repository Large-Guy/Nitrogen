module math;

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