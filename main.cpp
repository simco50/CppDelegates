#include "Delegates.h"
#include <iostream>
#include <memory>

#define START_TEST() \
std::cout << "----- [" << __func__ << "] -----" << std::endl << std::endl

#define END_TEST() \
std::cout << std::endl << "------------------------------------" << std::endl << std::endl

struct Foo
{
	static void StaticBarVoid(float a)
	{
		std::cout << "Static delegate parameter: " << a << std::endl;
	}
	static int StaticBarInt(float a)
	{
		std::cout << "Static delegate parameter: " << a << std::endl;
		return 10;
	}
	void BarVoid(float a)
	{
		std::cout << "Delegate parameter: " << a << std::endl;
	}
	int BarInt(float a)
	{
		std::cout << "Delegate parameter: " << a << std::endl;
		return 10;
	}
};

void SinglecastDelegateTest()
{
	START_TEST();

	SinglecastDelegate<int, float> del;
	del.BindLambda([](float a) 
	{
		std::cout << "Lambda delegate parameter: " << a << std::endl;
		return 10;
	});
	std::cout << "Lambda delegate return value: " << del.Execute(20) << std::endl;

	del.BindStatic(&Foo::StaticBarInt);
	std::cout << "Static delegate return value: " << del.Execute(20) << std::endl;

	Foo foo;
	del.BindRaw(&foo, &Foo::BarInt);
	std::cout << "Raw delegate return value: " << del.Execute(20) << std::endl;

	std::shared_ptr<Foo> pFoo = std::make_shared<Foo>();
	del.BindSP(pFoo, &Foo::BarInt);
	std::cout << "SP delegate return value: " << del.Execute(20) << std::endl;

	END_TEST();
}

void MulticastDelegateTest()
{
	START_TEST();

	MulticastDelegate<float> del;
	del.AddLambda([](float a)
	{
		std::cout << "Lambda delegate parameter: " << a << std::endl;
	});

	del.AddStatic(&Foo::StaticBarVoid);

	Foo foo;
	del.AddRaw(&foo, &Foo::BarVoid);

	std::shared_ptr<Foo> pFoo = std::make_shared<Foo>();
	del.AddSP(pFoo, &Foo::BarVoid);

	del.Broadcast(20);

	END_TEST();
}

int main()
{
	SinglecastDelegateTest();
	MulticastDelegateTest();
}