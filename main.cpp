#include "Delegates.h"
#include <memory>
#include <array>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

TEST_CASE("Delegate", "Simple Delegate")
{
	DECLARE_DELEGATE_RET(TestDelegate, float, float);
	TestDelegate testDelegate;

	REQUIRE_FALSE(testDelegate.IsBound());

	SECTION("Lambda")
	{
		testDelegate.BindLambda([](float a)
			{
				return 10 * a;
			});
		REQUIRE(testDelegate.IsBound());
		REQUIRE(testDelegate.Execute(10) == 100);
	}

	SECTION("Large Lambda")
	{
		std::array<char, 1024> largeBuffer{};
		largeBuffer[0] = 10;
		testDelegate.BindLambda([largeBuffer](float a)
			{
				return largeBuffer[0] * a;
			});
		REQUIRE(testDelegate.IsBound());
		REQUIRE(testDelegate.Execute(10) == 100);
		REQUIRE(testDelegate.GetSize() >= 1024);
	}

	SECTION("Raw")
	{
		struct Foo
		{
			float Bar(float a)
			{
				return 10 * a;
			}
		};
		Foo foo;
		testDelegate.BindRaw(&foo, &Foo::Bar);
		REQUIRE(testDelegate.IsBound());
		REQUIRE(testDelegate.Execute(10) == 100);
	}

	SECTION("Raw Const")
	{
		struct Foo
		{
			float Bar(float a) const
			{
				return 10 * a;
			}
		};
		Foo foo;
		testDelegate.BindRaw(&foo, &Foo::Bar);
		REQUIRE(testDelegate.IsBound());
		REQUIRE(testDelegate.Execute(10) == 100);
	}

	SECTION("Static")
	{
		struct Foo
		{
			static float Bar(float a)
			{
				return 10 * a;
			}
		};
		testDelegate.BindStatic(&Foo::Bar);
		REQUIRE(testDelegate.IsBound());
		REQUIRE(testDelegate.Execute(10) == 100);
	}

	SECTION("SP")
	{
		struct Foo
		{
			float Bar(float a)
			{
				return 10 * a;
			}
		};
		std::shared_ptr<Foo> foo = std::make_shared<Foo>();
		testDelegate.BindSP(foo, &Foo::Bar);
		REQUIRE(testDelegate.IsBound());
		REQUIRE(testDelegate.Execute(10) == 100);
	}

	SECTION("SP Const")
	{
		struct Foo
		{
			float Bar(float a) const
			{
				return 10 * a;
			}
		};
		std::shared_ptr<Foo> foo = std::make_shared<Foo>();
		testDelegate.BindSP(foo, &Foo::Bar);
		REQUIRE(testDelegate.IsBound());
		REQUIRE(testDelegate.Execute(10) == 100);
	}
}

TEST_CASE("Multicast Delegate", "Simple")
{
	DECLARE_MULTICAST_DELEGATE(Test, int);
	Test testDelegate;

	using ValueArray = std::array<int, 64>;
	ValueArray values{};

	SECTION("Lambda - Reference")
	{
		testDelegate.AddLambda([&values](int a)
			{
				values[a] = a;
			});
		REQUIRE(values[10] == 0);
		testDelegate.Broadcast(10);
		REQUIRE(values[10] == 10);
	}

	SECTION("Lambda Many - Reference")
	{
		testDelegate.AddLambda([&values](int a)
			{
				values[a] = a;
			});
		testDelegate.AddLambda([&values](int a)
			{
				values[a + 1] = a;
			});
		testDelegate.AddLambda([&values](int a)
			{
				values[a + 2] = a;
			});
		REQUIRE(values[10] == 0);
		testDelegate.Broadcast(10);
		REQUIRE(values[10] == 10);
		REQUIRE(values[11] == 10);
		REQUIRE(values[12] == 10);
	}

	SECTION("Lambda - Value")
	{
		testDelegate.AddLambda([values](int a) mutable
			{
				values[a] = a;
			});
		REQUIRE(values[10] == 0);
		testDelegate.Broadcast(10);
		REQUIRE(values[10] == 0);
	}

	SECTION("Raw")
	{
		struct Foo
		{
			Foo(ValueArray& v) : Values(v) {}
			void Bar(int a)
			{
				Values[a] = a;
			}
			ValueArray& Values;
		};
		Foo foo(values);
		testDelegate.AddRaw(&foo, &Foo::Bar);
		testDelegate.Broadcast(10);
		REQUIRE(values[10] == 10);
	}

	SECTION("Raw Const")
	{
		struct Foo
		{
			Foo(ValueArray& v) : Values(v) {}
			void Bar(int a) const
			{
				Values[a] = a;
			}
			ValueArray& Values;
		};
		Foo foo(values);
		testDelegate.AddRaw(&foo, &Foo::Bar);
		testDelegate.Broadcast(10);
		REQUIRE(values[10] == 10);
	}

	SECTION("SP")
	{
		struct Foo
		{
			Foo(ValueArray& v) : Values(v) {}
			void Bar(int a)
			{
				Values[a] = a;
			}
			ValueArray& Values;
		};
		std::shared_ptr<Foo> foo = std::make_shared<Foo>(values);
		testDelegate.AddSP(foo, &Foo::Bar);
		testDelegate.Broadcast(10);
		REQUIRE(values[10] == 10);
	}

	SECTION("SP Const")
	{
		struct Foo
		{
			Foo(ValueArray& v) : Values(v) {}
			void Bar(int a)
			{
				Values[a] = a;
			}
			ValueArray& Values;
		};
		std::shared_ptr<Foo> foo = std::make_shared<Foo>(values);
		testDelegate.AddSP(foo, &Foo::Bar);
		testDelegate.Broadcast(10);
		REQUIRE(values[10] == 10);
	}
}