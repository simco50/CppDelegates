#pragma once

#include <assert.h>
#include <memory>
#include <vector>
#include <tuple>

//#define TEST_LOGGING

#define STR(x) #x
#define STRINGIFY(x) STR(x)

#ifdef TEST_LOGGING
#define LOG(msg, ...) printf("[" __FUNCTION__ ": " STRINGIFY(__LINE__) "] > " msg##"\n", __VA_ARGS__)
#else
#define LOG(msg, ...)
#endif

#define DECLARE_DELEGATE(name, ...) \
using name = SinglecastDelegate<void, __VA_ARGS__>

#define DECLARE_DELEGATE_RET(name, retValue, ...) \
using name = SinglecastDelegate<retValue, __VA_ARGS__>

#define DECLARE_MULTICAST_DELEGATE(name, ...) \
using name = MulticastDelegate<__VA_ARGS__>

#define DECLARE_EVENT(name, ownerType, ...) \
class name : public MulticastDelegate<__VA_ARGS__> \
{ \
private: \
	friend class ownerType; \
	using MulticastDelegate::Broadcast; \
	using MulticastDelegate::RemoveAll; \
	using MulticastDelegate::Remove; \
};

//Base type for delegates
template<typename RetVal, typename ...Args>
class IDelegate
{
public:
	virtual ~IDelegate() {}
	virtual RetVal Execute(Args&&... args) = 0;
	virtual void* GetOwner() = 0;
};

template<typename RetVal, typename... Args2>
class StaticDelegate;

template<typename RetVal, typename... Args, typename... Args2>
class StaticDelegate<RetVal (Args...), Args2...> : public IDelegate<RetVal, Args...>
{
public:
	using DelegateFunction = RetVal(*)(Args..., Args2...);

	StaticDelegate(DelegateFunction function, Args2&&... args) 
		: m_Function(function), m_Payload(std::forward<Args2>(args)...)
	{}
	virtual ~StaticDelegate() {}
	virtual RetVal Execute(Args&&... args) override
	{
		return Execute_Internal(std::forward<Args>(args)..., std::index_sequence_for<Args2...>());
	}
	virtual void* GetOwner() override
	{
		return nullptr;
	}

private:
	template<std::size_t... Is>
	RetVal Execute_Internal(Args&&... args, std::index_sequence<Is...>)
	{
		return m_Function(std::forward<Args>(args)..., std::get<Is>(m_Payload)...);
	}

	DelegateFunction m_Function;
	std::tuple<Args2...> m_Payload;
};

template<typename T, typename RetVal, typename... Args2>
class RawDelegate;

template<typename T, typename RetVal, typename... Args, typename... Args2>
class RawDelegate<T, RetVal(Args...), Args2...> : public IDelegate<RetVal, Args...>
{
public:
	using DelegateFunction = RetVal(T::*)(Args..., Args2...);

	RawDelegate(T* pObject, DelegateFunction function, Args2&&... args) 
		: m_pObject(pObject), m_Function(function), m_Payload(std::forward<Args2>(args)...)
	{}
	virtual ~RawDelegate() {}
	virtual RetVal Execute(Args&&... args) override
	{
		return Execute_Internal(std::forward<Args>(args)..., std::index_sequence_for<Args2...>());
	}
	virtual void* GetOwner() override
	{
		return nullptr;
	}

private:
	template<std::size_t... Is>
	RetVal Execute_Internal(Args&&... args, std::index_sequence<Is...>)
	{
		return (m_pObject->*m_Function)(std::forward<Args>(args)..., std::get<Is>(m_Payload)...);
	}

	T* m_pObject;
	DelegateFunction m_Function;
	std::tuple<Args2...> m_Payload;
};

template<typename TLambda, typename RetVal, typename... Args>
class LambdaDelegate;

template<typename TLambda, typename RetVal, typename... Args, typename... Args2>
class LambdaDelegate<TLambda, RetVal(Args...), Args2...> : public IDelegate<RetVal, Args...>
{
public:
	explicit LambdaDelegate(TLambda&& lambda, Args2&&... args) :
		m_Lambda(std::forward<TLambda>(lambda)),
		m_Payload(std::forward<Args2>(args)...)
	{}

	RetVal Execute(Args&&... args) override
	{
		return Execute_Internal(std::forward<Args>(args)..., std::index_sequence_for<Args2...>());
	}

	virtual void* GetOwner() override
	{
		return nullptr;
	}

private:
	template<std::size_t... Is>
	RetVal Execute_Internal(Args&&... args, std::index_sequence<Is...>)
	{
		return (RetVal)((m_Lambda)(std::forward<Args>(args)..., std::get<Is>(m_Payload)...));
	}

	TLambda m_Lambda;
	std::tuple<Args2...> m_Payload;
};

template<typename T, typename RetVal, typename... Args>
class SPDelegate;

template<typename RetVal, typename T, typename ...Args, typename... Args2>
class SPDelegate<T, RetVal(Args...), Args2...> : public IDelegate<RetVal, Args...>
{
public:
	using DelegateFunction = RetVal(T::*)(Args..., Args2...);

	SPDelegate(const std::shared_ptr<T>& pObject, DelegateFunction pFunction, Args2&&... args) :
		m_pObject(pObject),
		m_pFunction(pFunction),
		m_Payload(std::forward<Args2>(args)...)
	{
	}

	virtual RetVal Execute(Args&&... args) override
	{
		return Execute_Internal(std::forward<Args>(args)..., std::index_sequence_for<Args2...>());
	}

	virtual void* GetOwner() override
	{
		return m_pObject.get();
	}

private:
	template<std::size_t... Is>
	RetVal Execute_Internal(Args&&... args, std::index_sequence<Is...>)
	{
		return (m_pObject.get()->*m_pFunction)(std::forward<Args>(args)..., std::get<Is>(m_Payload)...);
	}

	std::shared_ptr<T> m_pObject;
	DelegateFunction m_pFunction;
	std::tuple<Args2...> m_Payload;
};

//A handle to a delegate used for a multicast delegate
//Static ID so that every handle is unique
class DelegateHandle
{
public:
	constexpr DelegateHandle() noexcept
		: m_Id(-1)
	{
	}

	explicit DelegateHandle(bool /*generateId*/) noexcept
		: m_Id(GetNewID())
	{
	}

	~DelegateHandle() noexcept = default;
	DelegateHandle(const DelegateHandle& other) = default;
	DelegateHandle& operator=(const DelegateHandle& other) = default;

	DelegateHandle(DelegateHandle&& other) noexcept
		: m_Id(other.m_Id)
	{
		other.Reset();
	}

	DelegateHandle& operator=(DelegateHandle&& other) noexcept
	{
		m_Id = other.m_Id;
		other.Reset();
		return *this;
	}

	inline operator bool() const noexcept
	{
		return IsValid();
	}

	inline bool operator==(const DelegateHandle& other) const noexcept
	{
		return m_Id == other.m_Id;
	}

	inline bool operator<(const DelegateHandle& other) const noexcept
	{
		return m_Id < other.m_Id;
	}

	inline bool IsValid() const noexcept
	{
		return m_Id != -1;
	}

	inline void Reset() noexcept
	{
		m_Id = -1;
	}
private:
	int m_Id;
	static int CURRENT_ID;
	static int GetNewID();
};

template<typename size_t MaxStackSize>
class InlineAllocator
{
public:
	//Constructor
	constexpr InlineAllocator() noexcept
		: m_Size(0)
	{
	}

	//Destructor
	~InlineAllocator() noexcept
	{
		Free();
	}

	//Copy constructor
	InlineAllocator(const InlineAllocator& other)
		: m_Size(0)
	{
		LOG("InlineAllocator Copy operator");

		if (other.HasAllocation())
		{
			memcpy(Allocate(other.m_Size), other.GetAllocation(), other.m_Size);
		}
		m_Size = other.m_Size;
	}

	//Copy assignment operator
	InlineAllocator& operator=(const InlineAllocator& other)
	{
		LOG("InlineAllocator Copy assignment operator");

		if (other.HasAllocation())
		{
			memcpy(Allocate(other.m_Size), other.GetAllocation(), other.m_Size);
		}
		m_Size = other.m_Size;
		return *this;
	}

	//Move constructor
	InlineAllocator(InlineAllocator&& other) noexcept
		: m_Size(other.m_Size)
	{
		LOG("InlineAllocator Move constructor");

		other.m_Size = 0;
		if (m_Size > MaxStackSize)
		{
			std::swap(pPtr, other.pPtr);
		}
		else
		{
			memcpy(Buffer, other.Buffer, m_Size);
		}
	}

	//Move assignment operator
	InlineAllocator& operator=(InlineAllocator&& other) noexcept
	{
		LOG("InlineAllocator Move assignment operator");

		Free();
		m_Size = other.m_Size;
		other.m_Size = 0;
		if (m_Size > MaxStackSize)
		{
			std::swap(pPtr, other.pPtr);
		}
		else
		{
			memcpy(Buffer, other.Buffer, m_Size);
		}
		return *this;
	}

	//Allocate memory of given size
	//If the size is over the predefined threshold, it will be allocated on the heap
	void* Allocate(const size_t size)
	{
		Free();
		m_Size = size;
		if (size > MaxStackSize)
		{
			LOG("Heap allocation: %d", (int)size);
			pPtr = new char[size];
			return pPtr;
		}
		LOG("Stack allocation: %d", (int)size);
		return (void*)Buffer;
	}
	
	//Free the allocated memory
	void Free()
	{
		if (m_Size > MaxStackSize)
		{
			LOG("Heap deallocation: %d", (int)m_Size);
			delete[] pPtr;
		}
		m_Size = 0;
	}

	//Return the allocated memory either on the stack or on the heap
	void* GetAllocation() const
	{
		if (HasAllocation())
		{
			return m_Size > MaxStackSize ? pPtr : (void*)Buffer;
		}
		else
		{
			return nullptr;
		}
	}

	inline bool HasAllocation() const
	{
		return m_Size > 0;
	}

private:
	//If the allocation is smaller than the threshold, Buffer is used
	//Otherwise pPtr is used together with a separate dynamic allocation
	union
	{
		char Buffer[MaxStackSize];
		void* pPtr;
	};
	size_t m_Size;
};

template<typename RetVal, typename ...Args>
class DelegateHandler
{
public:
	using IDelegateT = IDelegate<RetVal, Args...>;

	//Copy constructor
	DelegateHandler(const DelegateHandler& other)
		: m_Allocator(other.m_Allocator), m_Handle(true)
	{
		LOG("DelegateHandler copy constructor");
	}

	//Copy assignment operator
	DelegateHandler& operator=(const DelegateHandler& other)
	{
		LOG("DelegateHandler copy assignment operator");
		m_Allocator = other.m_Allocator;
		m_Handle = DelegateHandle(true);
		return *this;
	}

	~DelegateHandler() noexcept
	{
		Release();
	}

	//Move constructor
	DelegateHandler(DelegateHandler&& other) noexcept
		: m_Allocator(std::move(other.m_Allocator)), m_Handle(std::move(other.m_Handle))
	{
		LOG("DelegateHandler move constructor");
	}

	//Move assignment operator
	DelegateHandler& operator=(DelegateHandler&& other) noexcept
	{
		LOG("DelegateHandler move assignment operator");

		m_Allocator.Free();
		m_Allocator = std::move(other.m_Allocator);
		m_Handle = std::move(m_Handle);
		return *this;
	}

	//Create delegate using member function
	template<typename T, typename... Args2>
	static DelegateHandler CreateRaw(T* pObj, RetVal(T::*pFunction)(Args..., Args2...), Args2... args)
	{
		DelegateHandler handler;
		handler.Bind<RawDelegate<T, RetVal(Args...), Args2...>>(pObj, pFunction, args...);
		return handler;
	}

	//Create delegate using global/static function
	template<typename... Args2>
	static DelegateHandler CreateStatic(RetVal(*pFunction)(Args..., Args2...), Args2... args)
	{
		DelegateHandler handler;
		handler.Bind<StaticDelegate<RetVal(Args...), Args2...>>(pFunction, args...);
		return handler;
	}

	//Create delegate using std::shared_ptr
	template<typename T, typename... Args2>
	static DelegateHandler CreateSP(const std::shared_ptr<T>& pObject, RetVal(T::*pFunction)(Args..., Args2...), Args2... args)
	{
		DelegateHandler handler;
		handler.Bind<SPDelegate<T, RetVal(Args...), Args2...>>(pObject, pFunction, args...);
		return handler;
	}

	//Create delegate using a lambda
	template<typename TLambda, typename... Args2>
	static DelegateHandler CreateLambda(TLambda&& lambda, Args2... args)
	{
		DelegateHandler handler;
		handler.Bind<LambdaDelegate<TLambda, RetVal(Args...), Args2...>>(std::forward<TLambda>(lambda), args...);
		return handler;
	}

	//Execute the delegate with the given parameters
	RetVal Execute(Args&&... args) const
	{
		assert(m_Allocator.HasAllocation());
		return GetDelegate()->Execute(std::forward<Args>(args)...);
	}

	//Gets the owner of the deletage
	//Only valid for SPDelegate and RawDelegate.
	//Otherwise returns nullptr by default
	void* GetOwner()
	{
		if (m_Allocator.HasAllocation())
		{
			return m_Allocator->GetAllocation()->GetOwner();
		}
		return nullptr;
	}

	inline DelegateHandle& GetHandle() 
	{
		return m_Handle;
	}

	inline const DelegateHandle& GetHandle() const
	{
		return m_Handle; 
	}

protected:
	constexpr DelegateHandler() noexcept
	{
	}

	template<typename T, typename... Args>
	void Bind(Args&&... args)
	{
		Release();
		void* pAlloc = m_Allocator.Allocate(sizeof(T));
		new (pAlloc) T(std::forward<Args>(args)...);
		m_Handle = DelegateHandle(true);
	}

	void Release()
	{
		m_Handle.Reset();
		if (m_Allocator.HasAllocation())
		{
			GetDelegate()->~IDelegate();
			m_Allocator.Free();
		}
	}

	inline IDelegateT* GetDelegate() const
	{
		return (IDelegateT*)(m_Allocator.GetAllocation());
	}

	//Allocator for the delegate itself.
	//Delegate gets allocated inline when its is smaller or equal than 32 bytes in size.
	//Can be changed by preference
	InlineAllocator<32> m_Allocator;
	DelegateHandle m_Handle;
};

//Delegate that can be bound to by just ONE object
template<typename RetVal, typename ...Args>
class SinglecastDelegate : public DelegateHandler<RetVal, Args...>
{
public:
	using DelegateHandlerT = DelegateHandler<RetVal, Args...>;

	//Default constructor
	SinglecastDelegate() = default;

	//Default destructor
	~SinglecastDelegate() = default;

	//Copy contructor
	SinglecastDelegate(const SinglecastDelegate& other)
		: m_Allocator(other.m_Allocator), m_Handle(true)
	{
		LOG("SinglecastDelegate copy constructor");
	}

	//Copy assignment operator
	SinglecastDelegate& operator=(const SinglecastDelegate& other)
	{
		LOG("SinglecastDelegate copy assignment operator");

		m_Allocator = other.m_Allocator;
		m_Handle = DelegateHandle(true);
		return *this;
	}

	//Move constructor
	SinglecastDelegate(SinglecastDelegate&& other) noexcept
		: DelegateHandler(std::move(other))
	{
		LOG("SinglecastDelegate move constructor");
	}

	//Move assignment operator
	SinglecastDelegate& operator=(SinglecastDelegate&& other) noexcept
	{
		LOG("SinglecastDelegate move assignment operator");

		m_Allocator = std::move(other.m_Allocator);
		m_Handle = std::move(other.m_Handle);
		return *this;
	}

	//Bind a member function
	template<typename T, typename... Args2>
	inline void BindRaw(T* pObject, RetVal(T::*pFunction)(Args..., Args2...), Args2&&... args)
	{
		Bind<RawDelegate<T, RetVal(Args...), Args2...>>(pObject, pFunction, std::forward<Args2>(args)...);
	}

	//Bind a static/global function
	template<typename... Args2>
	inline void BindStatic(RetVal(*pFunction)(Args..., Args2...), Args2&&... args)
	{
		Bind<StaticDelegate<RetVal(Args...), Args2...>>(pFunction, std::forward<Args2>(args)...);
	}

	//Bind a lambda
	template<typename LambdaType, typename... Args2>
	inline void BindLambda(LambdaType&& lambda, Args2&&... args)
	{
		Bind<LambdaDelegate<LambdaType, RetVal(Args...), Args2...>>(std::forward<LambdaType>(lambda), std::forward<Args2>(args)...);
	}

	//Bind a member function with a shared_ptr object
	template<typename T, typename... Args2>
	inline void BindSP(std::shared_ptr<T> pObject, RetVal(T::*pFunction)(Args..., Args2...), Args2&&... args)
	{
		Bind<SPDelegate<T, RetVal(Args...), Args2...>>(pObject, pFunction, std::forward<Args2>(args)...);
	}

	//If the allocator has a size, it means it's bound to something
	inline bool IsBound() const
	{
		return GetDelegate()->HasAllocation();
	}

	RetVal ExecuteIfBound(Args&&... args) const
	{
		if (IsBound())
		{
			return m_pDelegate->Execute(std::forward<Args>(args)...);
		}
		return RetVal();
	}

	//Clear the bound delegate if it exists
	inline void Clear()
	{
		Release();
	}

	//Clear the bound delegate if it is bound to the given object.
	//Ignored when pObject is a nullptr
	inline void ClearIfBoundTo(void* pObject)
	{
		if (pObject != nullptr && IsBoundTo(pObject))
		{
			Release();
		}
	}
};

//Delegate that can be bound to by MULTIPLE objects
template<typename ...Args>
class MulticastDelegate
{
private:
	using DelegateHandlerT = DelegateHandler<void, Args...>;

public:
	//Default constructor
	constexpr MulticastDelegate() = default;

	//Default destructor
	~MulticastDelegate() noexcept = default;

	//Default copy constructor
	MulticastDelegate(const MulticastDelegate& other) = default;

	//Defaul copy assignment operator
	MulticastDelegate& operator=(const MulticastDelegate& other) = default;

	//Move constructor
	MulticastDelegate(MulticastDelegate&& other) noexcept
		: m_Events(std::move(other.m_Events))
	{}

	//Move assignment operator
	MulticastDelegate& operator=(MulticastDelegate&& other) noexcept
	{
		m_Events = std::move(other.m_Events);
		return *this;
	}

	//Add delegate with the += operator
	inline DelegateHandle operator+=(DelegateHandlerT&& handler)
	{
		return Add(std::forward<DelegateHandlerT>(handler));
	}

	//Remove a delegate using its DelegateHandle
	inline bool operator-=(DelegateHandle& handle)
	{
		return Remove(handle);
	}

	inline DelegateHandle Add(DelegateHandlerT&& handler)
	{
		m_Events.push_back(std::forward<DelegateHandlerT>(handler));
		return m_Events.back().GetHandle();
	}

	//Bind a member function
	template<typename T, typename... Args2>
	inline DelegateHandle AddRaw(T* pObject, void(T::*pFunction)(Args..., Args2...), Args2&&... args)
	{
		m_Events.push_back(DelegateHandlerT::CreateRaw(pObject, pFunction, std::forward<Args2>(args)...));
		return m_Events.back().GetHandle();
	}

	//Bind a static/global function
	template<typename... Args2>
	inline DelegateHandle AddStatic(void(*pFunction)(Args..., Args2...), Args2&&... args)
	{
		m_Events.push_back(DelegateHandlerT::CreateStatic(pFunction, std::forward<Args2>(args)...));
		return m_Events.back().GetHandle();
	}

	//Bind a lambda
	template<typename LambdaType, typename... Args2>
	inline DelegateHandle AddLambda(LambdaType&& lambda, Args2&&... args)
	{
		m_Events.push_back(DelegateHandlerT::CreateLambda(std::forward<LambdaType>(lambda), std::forward<Args2>(args)...));
		return m_Events.back().GetHandle();
	}

	//Bind a member function with a shared_ptr object
	template<typename T, typename... Args2>
	inline DelegateHandle AddSP(std::shared_ptr<T> pObject, void(T::*pFunction)(Args..., Args2...), Args2&&... args)
	{
		m_Events.push_back(DelegateHandlerT::CreateSP(pObject, pFunction, std::forward<Args2>(args)...));
		return m_Events.back().GetHandle();
	}

	//Removes all handles that are bound from a specific object
	//Ignored when pObject is null
	//Note: Only works on Raw and SP bindings
	void RemoveObject(void* pObject)
	{
		if (pObject != nullptr)
		{
			for (size_t i = 0; i < m_Events.size(); ++i)
			{
				if (m_Events[i]->GetOwner() == pObject)
				{
					std::swap(m_Events[i], m_Events[m_Events.size() - 1]);
					m_Events.pop_back();
				}
			}
		}
	}

	//Remove a function from the event list by the handle
	bool Remove(DelegateHandle& handle)
	{
		for (auto& del : m_Events)
		{
			if (del.GetHandle() == handle)
			{
				std::swap(e, m_Events[m_Events.size() - 1]);
				m_Events.pop_back();
				return true;
			}
		}
		return false;
	}

	//Remove all the functions bound to the delegate
	inline void RemoveAll()
	{
		m_Events.clear();
	}

	//Execute all functions that are bound
	inline void Broadcast(Args&& ...args) const
	{
		for (size_t i = 0; i < m_Events.size(); ++i)
		{
			m_Events[i].Execute(std::forward<Args>(args)...);
		}
	}

private:
	std::vector<DelegateHandlerT> m_Events;
};