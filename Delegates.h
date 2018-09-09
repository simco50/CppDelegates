#pragma once

#include <assert.h>
#include <memory>
#include <vector>
#include <tuple>

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
	virtual RetVal Execute(Args ...args) = 0;
	virtual void* GetOwner() = 0;
};

template<typename RetVal, typename... Args2>
class StaticDelegate;

template<typename RetVal, typename... Args, typename... Args2>
class StaticDelegate<RetVal (Args...), Args2...> : public IDelegate<RetVal, Args...>
{
public:
	using DelegateFunction = RetVal(*)(Args..., Args2...);

	StaticDelegate(DelegateFunction function, Args2... args) : m_Function(function), m_Payload(args...) {}
	virtual ~StaticDelegate() {}
	virtual RetVal Execute(Args ...args) override
	{
		return Execute_Internal(std::forward<Args>(args)..., std::index_sequence_for<Args2...>());
	}
	virtual void* GetOwner() override
	{
		return nullptr;
	}

private:
	template<std::size_t... Is>
	RetVal Execute_Internal(Args... args, std::index_sequence<Is...>)
	{
		return m_Function(args..., std::get<Is>(m_Payload)...);
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

	RawDelegate(T* pObject, DelegateFunction function, Args2... args) : m_pObject(pObject), m_Function(function), m_Payload(args...) {}
	virtual ~RawDelegate() {}
	virtual RetVal Execute(Args ...args) override
	{
		return Execute_Internal(std::forward<Args>(args)..., std::index_sequence_for<Args2...>());
	}
	virtual void* GetOwner() override
	{
		return nullptr;
	}

private:
	template<std::size_t... Is>
	RetVal Execute_Internal(Args... args, std::index_sequence<Is...>)
	{
		return (m_pObject->*m_Function)(args..., std::get<Is>(m_Payload)...);
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
	explicit LambdaDelegate(TLambda&& lambda, Args2... args) :
		m_pLambda(std::make_shared<TLambda>(lambda)),
		m_Payload(std::forward<Args2>(args)...)
	{}

	RetVal Execute(Args... args) override
	{
		return Execute_Internal(args..., std::index_sequence_for<Args2...>());
	}

	virtual void* GetOwner() override
	{
		return nullptr;
	}

private:
	template<std::size_t... Is>
	RetVal Execute_Internal(Args... args, std::index_sequence<Is...>)
	{
		return (RetVal)((*m_pLambda)(args..., std::get<Is>(m_Payload)...));
	}

	std::shared_ptr<TLambda> m_pLambda;
	std::tuple<Args2...> m_Payload;
};

template<typename T, typename RetVal, typename... Args>
class SPDelegate;

template<typename RetVal, typename T, typename ...Args, typename... Args2>
class SPDelegate<T, RetVal(Args...), Args2...> : public IDelegate<RetVal, Args...>
{
public:
	using DelegateFunction = RetVal(T::*)(Args..., Args2...);

	SPDelegate(const std::shared_ptr<T>& pObject, DelegateFunction pFunction, Args2... args) :
		m_pObject(pObject),
		m_pFunction(pFunction),
		m_Payload(args...)
	{}

	virtual RetVal Execute(Args ...args) override
	{
		return Execute_Internal(args..., std::index_sequence_for<Args2...>());
	}

	virtual void* GetOwner() override
	{
		return m_pObject.get();
	}

private:
	template<std::size_t... Is>
	RetVal Execute_Internal(Args... args, std::index_sequence<Is...>)
	{
		return (m_pObject.get()->*m_pFunction)(args..., std::get<Is>(m_Payload)...);
	}

	std::shared_ptr<T> m_pObject;
	DelegateFunction m_pFunction;
	std::tuple<Args2...> m_Payload;
};

template<typename RetVal, typename ...Args>
class DelegateHandler
{
public:
	template<typename T, typename... Args2>
	static DelegateHandler CreateRaw(T* pObj, RetVal(T::*pFunction)(Args..., Args2...), Args2... args)
	{
		DelegateHandler handler;
		handler.m_pDelegate = std::make_shared<RawDelegate<T, RetVal(Args...), Args2...>>(pObj, pFunction, args...);
		return std::move(handler);
	}

	template<typename... Args2>
	static DelegateHandler CreateStatic(RetVal(*pFunction)(Args..., Args2...), Args2... args)
	{
		DelegateHandler handler;
		handler.m_pDelegate = std::make_shared<StaticDelegate<RetVal(Args...), Args2...>>(pFunction, args...);
		return std::move(handler);
	}

	template<typename T, typename... Args2>
	static DelegateHandler CreateSP(const std::shared_ptr<T>& pObject, RetVal(T::*pFunction)(Args..., Args2...), Args2... args)
	{
		DelegateHandler handler;
		handler.m_pDelegate = std::make_shared<SPDelegate<T, RetVal(Args...), Args2...>>(pObject, pFunction, args...);
		return std::move(handler);
	}

	template<typename TLambda, typename... Args2>
	static DelegateHandler CreateLambda(TLambda&& lambda, Args2... args)
	{
		DelegateHandler handler;
		handler.m_pDelegate = std::make_shared<LambdaDelegate<TLambda, RetVal(Args...), Args2...>>(std::forward<TLambda>(lambda), args...);
		return std::move(handler);
	}

	~DelegateHandler()
	{
		Release();
	}

	RetVal Execute(Args... args) const
	{
		if (m_pDelegate)
		{
			return m_pDelegate->Execute(args...);
		}
		return RetVal();
	}

	void* GetOwner()
	{
		if (m_pDelegate)
		{
			return m_pDelegate->GetOwner();
		}
		return nullptr;
	}

private:
	DelegateHandler()
	{}

	void Release()
	{
		m_pDelegate.reset();
	}

	std::shared_ptr<IDelegate<RetVal, Args...>> m_pDelegate;
};

//Delegate that can be bound to by just ONE object
template<typename RetVal, typename ...Args>
class SinglecastDelegate
{
public:
	SinglecastDelegate() {}
	~SinglecastDelegate() {}

	//Create a SinglecastDelegate instance bound with member function
	template<typename T, typename... Args2>
	static SinglecastDelegate CreateRaw(T* pObject, RetVal(T::*pFunction)(Args...), Args2&&... args)
	{
		SinglecastDelegate NewDelegate;
		NewDelegate.BindRaw(pObject, pFunction, std::forward<Args2>(args)...);
		return NewDelegate;
	}

	//Create a SinglecastDelegate instance bound with a static/global function
	template<typename... Args2>
	static SinglecastDelegate CreateStatic(RetVal(*pFunction)(Args...), Args2&&... args)
	{
		SinglecastDelegate NewDelegate;
		NewDelegate.BindStatic(pFunction, std::forward<Args2>(args)...);
		return NewDelegate;
	}

	//Create a SinglecastDelegate instance bound with a lambda
	template<typename LambdaType, typename... Args2>
	static SinglecastDelegate CreateLambda(LambdaType&& lambda, Args2&&... args)
	{
		SinglecastDelegate NewDelegate;
		NewDelegate.BindLambda(std::forward<LambdaType>(lambda), std::forward<Args2>(args)...);
		return NewDelegate;
	}

	//Create a SinglecastDelegate instance bound with member function using a shared_ptr
	template<typename T, typename... Args2>
	static SinglecastDelegate CreateSP(std::shared_ptr<T> pObject, RetVal(T::*pFunction)(Args...), Args2&&... args)
	{
		SinglecastDelegate NewDelegate;
		NewDelegate.BindStatic(pObject, pFunction, std::forward<Args2>(args)...);
		return NewDelegate;
	}

	//Bind a member function
	template<typename T, typename... Args2>
	void BindRaw(T* pObject, RetVal(T::*pFunction)(Args..., Args2...), Args2&&... args)
	{
		m_pEvent = std::make_shared<RawDelegate<T, RetVal(Args...), Args2...>>(pObject, pFunction, std::forward<Args2>(args)...);
	}

	//Bind a static/global function
	template<typename... Args2>
	void BindStatic(RetVal(*pFunction)(Args..., Args2...), Args2&&... args)
	{
		m_pEvent = std::make_shared<StaticDelegate<RetVal (Args...), Args2...>>(pFunction, std::forward<Args2>(args)...);
	}

	//Bind a lambda
	template<typename LambdaType, typename... Args2>
	void BindLambda(LambdaType&& lambda, Args2&&... args)
	{
		m_pEvent = std::make_shared<LambdaDelegate<LambdaType, RetVal(Args...), Args2...>>(std::forward<LambdaType>(lambda), std::forward<Args2>(args)...);
	}

	//Bind a member function with a shared_ptr object
	template<typename T, typename... Args2>
	void BindSP(std::shared_ptr<T> pObject, RetVal(T::*pFunction)(Args..., Args2...), Args2&&... args)
	{
		m_pEvent = std::make_shared<SPDelegate<T, RetVal(Args...), Args2...>>(pObject, pFunction, std::forward<Args2>(args)...);
	}

	//Execute the function if the delegate is bound
	RetVal ExecuteIfBound(Args ...args) const
	{
		if (IsBound())
		{
			return m_pEvent->Execute(std::forward<Args>(args)...);
		}
		return RetVal();
	}

	//Execute the function
	RetVal Execute(Args ...args) const
	{
		assert(m_pEvent != nullptr);
		return m_pEvent->Execute(args...);
	}

	//Check if there is a function bound
	inline bool IsBound() const
	{
		return m_pEvent != nullptr;
	}

	inline bool IsBoundTo(void* pObject) const
	{
		return IsBound() && m_pEvent->GetOwner() == pObject;
	}

	inline void Clear()
	{
		Release();
	}

	inline void ClearIfBoundTo(void* pObject)
	{
		if (IsBoundTo(pObject))
		{
			Release();
		}
	}

private:
	inline void Release()
	{
		m_pEvent.reset();
	}

	std::shared_ptr<IDelegate<RetVal, Args...>> m_pEvent;
};

//A handle to a delegate used for a multicast delegate
//Static ID so that every handle is unique
class DelegateHandle
{
public:
	DelegateHandle() : m_Id(-1) {}
	explicit DelegateHandle(bool /*generateId*/) : m_Id(GetNewID()) {}
	~DelegateHandle() {}

	bool operator==(const DelegateHandle& other) const
	{
		return m_Id == other.m_Id;
	}

	bool operator<(const DelegateHandle& other) const
	{
		return m_Id < other.m_Id;
	}

	bool IsValid() const 
	{
		return m_Id != -1; 
	}

	void Reset() 
	{ 
		m_Id = -1; 
	}

private:
	int m_Id;
	static int CURRENT_ID;
	static int GetNewID();
};

//Delegate that can be bound to by MULTIPLE objects
template<typename ...Args>
class MulticastDelegate
{
private:
	using DelegateHandlerT = DelegateHandler<void, Args...>;

public:
	MulticastDelegate() {}
	~MulticastDelegate() {}

	MulticastDelegate(const MulticastDelegate& other) = delete;
	MulticastDelegate& operator=(const MulticastDelegate& other) = delete;

	inline DelegateHandle operator+=(DelegateHandlerT&& handler)
	{
		return AddDelegate(std::move(handler));
	}

	inline bool operator-=(DelegateHandle& handle)
	{
		return Remove(handle);
	}

	//Bind a member function
	template<typename T, typename... Args2>
	inline DelegateHandle AddRaw(T* pObject, void(T::*pFunction)(Args..., Args2...), Args2... args)
	{
		return AddDelegate(DelegateHandlerT::CreateRaw(pObject, pFunction, args...));
	}

	//Bind a static/global function
	template<typename... Args2>
	inline DelegateHandle AddStatic(void(*pFunction)(Args..., Args2...), Args2... args)
	{
		return AddDelegate(DelegateHandlerT::CreateStatic(pFunction, std::forward<Args2>(args)...));
	}

	//Bind a lambda
	template<typename LambdaType, typename... Args2>
	inline DelegateHandle AddLambda(LambdaType&& lambda, Args2... args)
	{
		return AddDelegate(DelegateHandlerT::CreateLambda(std::forward<LambdaType>(lambda), std::forward<Args2>(args)...));
	}

	//Bind a member function with a shared_ptr object
	template<typename T, typename... Args2>
	inline DelegateHandle AddSP(std::shared_ptr<T> pObject, void(T::*pFunction)(Args..., Args2...), Args2... args)
	{
		return AddDelegate(DelegateHandlerT::CreateSP(pObject, pFunction, std::forward<Args2>(args)...));
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
				if (e.second->GetOwner() == pObject)
				{
					std::swap(e, m_Events[m_Events.size() - 1]);
					m_Events.pop_back();
				}
			}
		}
	}

	//Remove a function from the event list by the handle
	bool Remove(DelegateHandle& handle)
	{
		for (auto& e : m_Events)
		{
			if (e.first == handle)
			{
				std::swap(e, m_Events[m_Events.size() - 1]);
				m_Events.pop_back();
				handle.Reset();
				return true;
			}
		}
		return false;
	}

	//Remove all the functions bound to the delegate
	void RemoveAll()
	{
		m_Events.clear();
	}

	//Execute all functions that are bound
	void Broadcast(Args ...args) const
	{
		for (size_t i = 0; i < m_Events.size(); ++i)
		{
			m_Events[i].second.Execute(args...);
		}
	}

private:
	DelegateHandle AddDelegate(DelegateHandlerT&& d)
	{
		DelegateHandle handle(true);
		m_Events.push_back(std::pair<DelegateHandle, DelegateHandlerT>(handle, std::move(d)));
		return handle;
	}

	std::vector<std::pair<DelegateHandle, DelegateHandlerT>> m_Events;
};