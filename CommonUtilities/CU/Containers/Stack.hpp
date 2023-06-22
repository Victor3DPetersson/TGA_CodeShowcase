#pragma once
#include <assert.h>
namespace CU
{
	template <class T, const unsigned int T_Size = 1000>
	class Stack
	{
	public:
		Stack();
		int Size() const;
		const T& GetTop() const;
		T& GetTop();
		void Push(const T& aValue);
		T Pop();
	private:
		T myContainer[T_Size];
		unsigned int myIterator;
	};

	template<class T, const unsigned int T_Size>
	inline Stack<T, T_Size>::Stack()
	{
		myIterator = 0;
	}

	template<class T, const unsigned int T_Size>
	inline int Stack<T, T_Size>::Size() const
	{
		return myIterator;
	}

	template<class T, const unsigned int T_Size>
	inline const T& Stack<T, T_Size>::GetTop() const
	{
		assert(myIterator != 0 && "Stack is Empty");
		return myContainer[myIterator - 1];
	}

	template<class T, const unsigned int T_Size>
	inline T& Stack<T, T_Size>::GetTop()
	{
		assert(myIterator != 0 && "Stack is Empty");
		return myContainer[myIterator - 1];
	}

	template<class T, const unsigned int T_Size>
	inline void Stack<T, T_Size>::Push(const T& aValue)
	{
		assert(myIterator <= T_Size && "Stack Filled, increase size of Container");
		myContainer[myIterator] = aValue;

		myIterator++;
	}

	template<class T, const unsigned int T_Size>
	inline T Stack<T, T_Size>::Pop()
	{
		assert(myIterator != 0 && "Stack Empty, cannot remove from empty container");
		T returnValue = myContainer[myIterator - 1];
		myIterator--;
		return returnValue;
	}
}