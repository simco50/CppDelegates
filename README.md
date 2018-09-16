# C++ Delegates #
The point of this repository is just to experiment and create delegates in C++.
These are not standard in C++ but are a great way for communication and it's widely known in other programming languages like C#.
Unreal Engine is worth nothing as it has its own implementation of Delegates in C++ too.

## Classes ##
- ```Delegate<RetVal, Args>```
- ```MulticastDelegate<Args>```

## Features ##
- Support for:
	- Static/Global methods
	- Member functions
	- Lambda's
	- std::shared_ptr
- Delegate object is allocated inline if it is under 32 bytes
- Add payload to delegate during bind-time
- Move operations enable optimization

## Example Usage ##

### Delegate ###

``` 
Delegate<int, float> del;
del.BindLambda([](float a, int payload)
{
	std::cout << "Lambda delegate parameter: " << a << std::endl;
	std::cout << "Lambda delegate payload: " << payload << std::endl;
	return 10;
}, 50);
std::cout << "Lambda delegate return value: " << del.Execute(20) << std::endl;
```
```
Output:
Lambda delegate parameter: 20
Lambda delegate payload: 50
Lambda delegate return value: 10
```

### MulticastDelegate ###

```
struct Foo
{
	void Bar(float a, int payload)
	{
		std::cout << "Raw delegate parameter: " << a << std::endl;
		std::cout << "Raw delegate payload: " << payload << std::endl;
	}
};
MulticastDelegate<float> del;
del.AddLambda([](float a, int payload)
{
	std::cout << "Lambda delegate parameter: " << a << std::endl;
	std::cout << "Lambda delegate payload: " << payload << std::endl;
}, 90);

Foo foo;
del.AddRaw(&foo, &Foo::Bar, 10);
del.Broadcast(20);
```

```
Output:
Lambda delegate parameter: 20
Lambda delegate payload: 90
Raw delegate parameter: 20
Raw delegate payload: 10
```
