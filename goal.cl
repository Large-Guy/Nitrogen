//the goal.
//very C like, like a C+.
//one thing you may notice is that styling is forced in the language in a lot of aspects. 
//this should make codebases a lot more consistent with each other with very little room for variation.

//note: no header files. stuff is all declared inline.

module program; //defines a module which can then be imported.

//submodules like this: module program.submodule;

import stl; //imports the stl library, in reality this is unnecessary because it is automatically imported.

//dynamic array struct. would be built into stl.
struct DynArray<T>
{
    //private because of _ prefix.
    T* _data; //builtin unique type. in reality it would be better to use the stl::array<T> for builtin bounds checking.
    usize _capacity; //minor rust inspiration.
    usize _count; //compiler will error if functions appear before variables for style.
    
    //constructors are static, because they're not really a thing in a way.
    static DynArray<T> new()
    {
        DynArray<T> array;
        array._data = stl::alloc(sizeof(T)); //stl module memory allocation function.
        
        if(array._data == null) //null keyword instead of nullptr or NULL
        {
            error("Failed to allocate memory for DynArray"); //exits the program and prints error message.
        }
        
        array._capacity = 1;
        array._count = 0;
        
        return array;
    }
    
    //destructors are not static, minor inconsistency but really it makes sense if you think about it.
    ~()
    {
        stl::free(this._data);
    }
    
    void add(T value)
    {
        if(this._count == this._capacity)
        {
            this._capacity *= 2;
            this._data = stl::realloc(sizeof(T) * this._capacity);
        }
        this._data[this._count++] = value;
    }
    
    ref T operator [](usize index)
    {
        //stl::array<T> would typically handle this in reality.
        if(index < 0 || index >= this._count)
        {
            error("Attempt to access out of bounds memory");
        }
        return this._data[index);
    }
}



//this is pretty self explanatory imo.
interface IAnimal {
    void make_sound();
}

struct Dog : IAnimal { //i actually do like this explicit c++ syntax over the implicitness of rust.
    string name;
    
    static Dog new(string name)
    {
        Dog dog;
        dog.name = name;
        return dog;
    }

    void make_sound()
    {
        stl::print("Meow Meow!");
    }
}

struct Cat : IAnimal {
    string name;
    
    static Cat new(string name)
    {
        Cat cat;
        cat.name = name;
        return cat;
    }

    void make_sound()
    {
        stl::print("Woof Woof!");
    }
}

i32 main()
{
    DynArray<i32> numbers = DynArray<i32>.new(); //more rust inspiration
    numbers.add(24);
    
    stl::unique<IAnimal> animal = stl::unique<Dog>.new(); //builtin unique type.
    
    animal.make_sound(); // "Meow Meow!"
    
    stl::print("Hello, World!");
    
    return 0;
}