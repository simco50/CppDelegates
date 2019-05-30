#include "Delegates.h"
#include <iostream>
#include <memory>

#define START_TEST() \
std::cout << "----- [" << __func__ << "] -----" << std::endl; { std::cout << std::endl \

#define END_TEST() \
std::cout << std::endl; } std::cout << std::endl << "------------------------------------" << std::endl << std::endl

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
	DECLARE_DELEGATE_RET(TestDelegate, int, float);

	TestDelegate testDelegate;
	testDelegate.BindLambda([](float a)
	{
		std::cout << "Lambda delegate parameter: " << a << std::endl;
		return 10;
	});
	std::cout << "Lambda delegate return value: " << testDelegate.Execute(20) << std::endl;

	testDelegate.BindStatic(&Foo::StaticBarInt);
	std::cout << "Static delegate return value: " << testDelegate.Execute(20) << std::endl;

	Foo foo;
	testDelegate.BindRaw(&foo, &Foo::BarInt);
	std::cout << "Raw delegate return value: " << testDelegate.Execute(20) << std::endl;

	std::shared_ptr<Foo> pFoo = std::make_shared<Foo>();
	testDelegate.BindSP(pFoo, &Foo::BarInt);
	std::cout << "SP delegate return value: " << testDelegate.Execute(20) << std::endl;

	char buffer[] = "Hello World";
	testDelegate.BindLambda([buffer](float)
	{
		std::cout << buffer << std::endl;
		return 0;
	});
	testDelegate.ExecuteIfBound(20);

	TestDelegate copyConstructed = testDelegate;
	
	copyConstructed.ExecuteIfBound(20);
	testDelegate.ExecuteIfBound(20);

	TestDelegate moveConstructed = std::move(copyConstructed);
	moveConstructed.Execute(20);

	TestDelegate copyAssigned;
	copyAssigned = moveConstructed;

	copyAssigned.Execute(20);
	moveConstructed.Execute(20);

	TestDelegate moveAssigned;
	moveAssigned = std::move(copyAssigned);
	moveAssigned.Execute(20);

	END_TEST();
}

void MulticastDelegateTest()
{
	START_TEST();
	DECLARE_MULTICAST_DELEGATE(Test, float);

	Test testDelegate;
	testDelegate.AddLambda([](float a)
	{
		std::cout << "Lambda delegate parameter: " << a << std::endl;
	});

	testDelegate.AddStatic(&Foo::StaticBarVoid);

	Foo foo;
	testDelegate.AddRaw(&foo, &Foo::BarVoid);

	std::shared_ptr<Foo> pFoo = std::make_shared<Foo>();
	testDelegate.AddSP(pFoo, &Foo::BarVoid);
	
	testDelegate.Broadcast(20);

	Test copyConstructed = testDelegate;

	copyConstructed.Broadcast(20);
	testDelegate.Broadcast(20);

	Test moveConstructed = std::move(copyConstructed);
	moveConstructed.Broadcast(20);

	Test copyAssigned;
	copyAssigned = moveConstructed;

	copyAssigned.Broadcast(20);
	moveConstructed.Broadcast(20);

	Test moveAssigned;
	moveAssigned = std::move(copyAssigned);
	moveAssigned.Broadcast(20);

	DelegateHandle handle = moveAssigned += TestDelegate::CreateLambda([](float) {});
	moveAssigned -= handle;

	END_TEST();
}


int main()
{
	SinglecastDelegateTest();
	MulticastDelegateTest();
}