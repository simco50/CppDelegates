#define DELEGATES_IMPLEMENTATION
#include "Delegates.h"
#include <memory>
#include <array>

#define CATCH_CONFIG_RUNNER
#define CATCH_CONFIG_NO_WCHAR
#include "catch.hpp"

struct Foo
{
	float Bar(float a)
	{
		return a;
	}
	float BarConst(float a) const
	{
		return a;
	}
	static float BarStatic(float a)
	{
		return a;
	}
};

TEST_CASE("Static Delegate")
{
	StaticDelegate<float(float)> del(&Foo::BarStatic);
	REQUIRE(del.GetOwner() == nullptr);
	REQUIRE(del.Execute(10) == 10);
}

struct DelegateGenerator
{
	template<typename Callback>
	static std::unique_ptr<IDelegate<float, float>> GetDelegate(Callback&& callback)
	{
		return std::make_unique<LambdaDelegate<Callback, float(float)>>(std::move(callback));
	}
};

TEST_CASE("Lambda Delegate")
{
	auto pDel = DelegateGenerator::GetDelegate([](float a) {return a; });
	REQUIRE(pDel->GetOwner() == nullptr);
	REQUIRE(pDel->Execute(10) == 10);
}

TEST_CASE("Raw Delegate")
{
	Foo foo;

	SECTION("Non-const")
	{
		RawDelegate<false, Foo, float(float)> del(&foo, &Foo::Bar);
		REQUIRE(del.Execute(10) == 10);
		REQUIRE(del.GetOwner() == &foo);
	}
	SECTION("Const")
	{
		RawDelegate<true, Foo, float(float)> del(&foo, &Foo::BarConst);
		REQUIRE(del.Execute(10) == 10);
		REQUIRE(del.GetOwner() == &foo);
	}
}

TEST_CASE("SP Delegate")
{
	std::shared_ptr<Foo> foo = std::make_shared<Foo>();
	SECTION("Non-const")
	{
		SPDelegate<false, Foo, float(float)> del(foo, &Foo::Bar);
		REQUIRE(del.Execute(10) == 10);
		REQUIRE(del.GetOwner() == foo.get());
	}
	SECTION("Const")
	{
		SPDelegate<true, Foo, float(float)> del(foo, &Foo::BarConst);
		REQUIRE(del.Execute(10) == 10);
		REQUIRE(del.GetOwner() == foo.get());
	}
}

TEST_CASE("Delegate Inits", "Delegate Constructor/Copying/Moving")
{
	DECLARE_DELEGATE(TestDelegate);
	TestDelegate testDelegate;

	SECTION("Default Constructor")
	{
		REQUIRE_FALSE(testDelegate.IsBound());
		REQUIRE(testDelegate.GetSize() == 0);
		REQUIRE(testDelegate.GetOwner() == nullptr);
	}

	SECTION("Constructor")
	{
		testDelegate.BindLambda([]() {});
		REQUIRE(testDelegate.IsBound());
		REQUIRE(testDelegate.GetSize() > 0);
	}

	SECTION("Copy Constructor")
	{
		testDelegate.BindLambda([]() {});
		TestDelegate testDelegate2 = testDelegate;
		REQUIRE(testDelegate2.IsBound());
		REQUIRE(testDelegate2.GetSize() > 0);
	}

	SECTION("Assignment Operator")
	{
		testDelegate.BindLambda([]() {});
		TestDelegate testDelegate2;
		testDelegate2 = testDelegate;
		REQUIRE(testDelegate2.IsBound());
		REQUIRE(testDelegate2.GetSize() > 0);
		REQUIRE(testDelegate.IsBound());
		REQUIRE(testDelegate.GetSize() > 0);
	}

	SECTION("Move Constructor")
	{
		testDelegate.BindLambda([]() {});
		TestDelegate testDelegate2 = std::move(testDelegate);
		REQUIRE(testDelegate2.IsBound());
		REQUIRE(testDelegate2.GetSize() > 0);
		REQUIRE_FALSE(testDelegate.IsBound());
		REQUIRE_FALSE(testDelegate.GetSize() > 0);
	}

	SECTION("Move Assignment Operator")
	{
		testDelegate.BindLambda([]() {});
		TestDelegate testDelegate2;
		testDelegate2 = std::move(testDelegate);
		REQUIRE(testDelegate2.IsBound());
		REQUIRE(testDelegate2.GetSize() > 0);
		REQUIRE_FALSE(testDelegate.IsBound());
		REQUIRE_FALSE(testDelegate.GetSize() > 0);
	}
}

TEST_CASE("Delegate Creates", "Delegate creation")
{
	DECLARE_DELEGATE_RET(TestDelegate, float, float);

	SECTION("Lambda")
	{
		TestDelegate del = TestDelegate::CreateLambda([](float a) { return a; });
		REQUIRE(del.Execute(10) == 10);
	}
	SECTION("Lambda Large")
	{
		std::array<float, 1024> arr{};
		TestDelegate del = TestDelegate::CreateLambda([arr](float a) mutable { arr[0] = a; return a; });
		REQUIRE(del.Execute(10) == 10);
	}
	SECTION("Static")
	{
		TestDelegate del = TestDelegate::CreateStatic(&Foo::BarStatic);
		REQUIRE(del.Execute(10) == 10);
	}
	SECTION("Raw")
	{
		Foo foo;
		TestDelegate del = TestDelegate::CreateRaw(&foo, &Foo::Bar);
		REQUIRE(del.Execute(10) == 10);
	}
	SECTION("Raw Const")
	{
		Foo foo;
		TestDelegate del = TestDelegate::CreateRaw(&foo, &Foo::BarConst);
		REQUIRE(del.Execute(10) == 10);
	}
	SECTION("SP")
	{
		std::shared_ptr<Foo> foo = std::make_shared<Foo>();
		TestDelegate del = TestDelegate::CreateSP(foo, &Foo::Bar);
		REQUIRE(del.Execute(10) == 10);
	}
	SECTION("SP Const")
	{
		std::shared_ptr<Foo> foo = std::make_shared<Foo>();
		TestDelegate del = TestDelegate::CreateSP(foo, &Foo::BarConst);
		REQUIRE(del.Execute(10) == 10);
	}
}

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
		REQUIRE(testDelegate.GetOwner() == nullptr);
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
		REQUIRE(testDelegate.GetOwner() == nullptr);
	}

	SECTION("Raw")
	{
		Foo foo;
		testDelegate.BindRaw(&foo, &Foo::Bar);
		REQUIRE(testDelegate.IsBound());
		REQUIRE(testDelegate.Execute(10) == 10);
		REQUIRE(testDelegate.GetOwner() == &foo);
	}

	SECTION("Raw Const")
	{
		Foo foo;
		testDelegate.BindRaw(&foo, &Foo::Bar);
		REQUIRE(testDelegate.IsBound());
		REQUIRE(testDelegate.Execute(10) == 10);
		REQUIRE(testDelegate.GetOwner() == &foo);
	}

	SECTION("Static")
	{
		testDelegate.BindStatic(&Foo::BarStatic);
		REQUIRE(testDelegate.IsBound());
		REQUIRE(testDelegate.Execute(10) == 10);
		REQUIRE(testDelegate.GetOwner() == nullptr);
	}

	SECTION("SP")
	{
		std::shared_ptr<Foo> foo = std::make_shared<Foo>();
		testDelegate.BindSP(foo, &Foo::Bar);
		REQUIRE(testDelegate.IsBound());
		REQUIRE(testDelegate.Execute(10) == 10);
		REQUIRE(testDelegate.GetOwner() == foo.get());
	}

	SECTION("SP Const")
	{
		std::shared_ptr<Foo> foo = std::make_shared<Foo>();
		testDelegate.BindSP(foo, &Foo::Bar);
		REQUIRE(testDelegate.IsBound());
		REQUIRE(testDelegate.Execute(10) == 10);
		REQUIRE(testDelegate.GetOwner() == foo.get());
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

TEST_CASE("Multicast Delegate Removes", "Simple")
{
	DECLARE_MULTICAST_DELEGATE(Test, int);
	Test testDelegate;

	using ValueArray = std::array<int, 64>;
	ValueArray values{};

	SECTION("Handle")
	{
		DelegateHandle handle = testDelegate.AddLambda([&values](int a)
			{
				values[a] = a;
			});
		REQUIRE(values[10] == 0);
		testDelegate.Broadcast(10);
		REQUIRE(values[10] == 10);
		testDelegate.Remove(handle);
		testDelegate.Broadcast(20);
		REQUIRE(values[10] == 10);
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
		REQUIRE(values[10] == 0);
		testDelegate.Broadcast(10);
		REQUIRE(values[10] == 10);
		testDelegate.RemoveObject(&foo);
		testDelegate.Broadcast(20);
		REQUIRE(values[10] == 10);
	}
}

TEST_CASE("Multicase Delegate Inits", "Delegate Constructor/Copying/Moving")
{
	DECLARE_MULTICAST_DELEGATE(TestDelegate);
	TestDelegate testDelegate;

	SECTION("Default Constructor")
	{
		REQUIRE(testDelegate.GetSize() == 0);
	}

	SECTION("Constructor")
	{
		testDelegate.AddLambda([]() {});
		testDelegate.AddLambda([]() {});
		testDelegate.AddLambda([]() {});
		REQUIRE(testDelegate.GetSize() == 3);
	}

	SECTION("Copy Constructor")
	{
		testDelegate.AddLambda([]() {});
		testDelegate.AddLambda([]() {});
		testDelegate.AddLambda([]() {});
		TestDelegate testDelegate2 = testDelegate;
		REQUIRE(testDelegate2.GetSize() == 3);
	}

	SECTION("Assignment Operator")
	{
		testDelegate.AddLambda([]() {});
		testDelegate.AddLambda([]() {});
		testDelegate.AddLambda([]() {});
		TestDelegate testDelegate2;
		testDelegate2 = testDelegate;
		REQUIRE(testDelegate2.GetSize() == 3);
		REQUIRE(testDelegate.GetSize() == 3);
	}

	SECTION("Move Constructor")
	{
		testDelegate.AddLambda([]() {});
		testDelegate.AddLambda([]() {});
		testDelegate.AddLambda([]() {});
		TestDelegate testDelegate2 = std::move(testDelegate);
		REQUIRE(testDelegate2.GetSize() == 3);
		REQUIRE(testDelegate.GetSize() == 0);
	}

	SECTION("Move Assignment Operator")
	{
		testDelegate.AddLambda([]() {});
		testDelegate.AddLambda([]() {});
		testDelegate.AddLambda([]() {});
		TestDelegate testDelegate2;
		testDelegate2 = std::move(testDelegate);
		REQUIRE(testDelegate2.GetSize() == 3);
		REQUIRE(testDelegate.GetSize() == 0);
	}
}

int main(int argc, char* argv[])
{
	// Enable run-time memory leak check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(21499);
#endif

	{
		Catch::Session session; // There must be exactly one instance

								// writing to session.configData() here sets defaults
								// this is the preferred way to set them

		int returnCode = session.applyCommandLine(argc, argv);
		if (returnCode != 0) // Indicates a command line error
			return returnCode;

		// writing to session.configData() or session.Config() here 
		// overrides command line args
		// only do this if you know you need to

		int numFailed = session.run();

		// numFailed is clamped to 255 as some unices only use the lower 8 bits.
		// This clamping has already been applied, so just return it here
		// You can also do any post run clean-up here
		return numFailed;
	}
}