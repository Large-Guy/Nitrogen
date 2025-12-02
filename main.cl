module program;

void print()
{
    return; //no print functionality exists yet
}

interface IAnimal
{
    void make_sound();
}

interface IDamage
{
    void deal_damage();
}

struct Dog : IAnimal, IDamage
{
    void make_sound()
    {
        return;
    }
}

void main()
{
    Dog dog;
    dog.make_sound();
}
