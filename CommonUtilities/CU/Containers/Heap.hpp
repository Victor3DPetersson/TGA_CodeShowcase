#pragma once
#include "GrowingArray.hpp"
#include <assert.h>

namespace CU
{
	template <class T>
	class Heap
	{
	public:
		Heap();
		int Size() const;            
		void Enqueue(const T& aElement);
		const T& GetTop() const;      
		T Dequeue();         
	private:
		int GetLeftChildIndex(const unsigned int aIndex);
		int GetRightChildIndex(const unsigned int aIndex);
		int GetParentIndex(const unsigned int aIndex);
		void SiftDown();
		void SiftUp();
		GrowingArray<T> myContainer;
	};

	template <class T>
	inline Heap<T>::Heap()
	{
		myContainer.Init(100);
	}

	template <class T>
	inline int Heap<T>::Size() const
	{
		return myContainer.Size();
	}

	template <class T>
	inline void Heap<T>::Enqueue(const T& aElement)
	{
		myContainer.Add(aElement);
		SiftUp();
	}

	template <class T>
	inline const T& Heap<T>::GetTop() const
	{
		assert(myContainer.Size() >= 1 && "Heap is Empty");
		return myContainer[0];
	}

	template <class T>
	inline T Heap<T>::Dequeue()
	{
		assert(myContainer.Size() >= 1 && "Heap is Empty");
		T returnValue = GetTop();
		if (myContainer.Size() == 1)
		{
			myContainer.RemoveAtIndex(myContainer.Size() - 1);
			return returnValue;
		}
		myContainer[0] = myContainer.GetLast();
		myContainer.RemoveAtIndex(myContainer.Size() - 1);
		SiftDown();

		return returnValue;
	}
	template <class T>
	inline int Heap<T>::GetLeftChildIndex(const unsigned int aIndex)
	{
		return 2 * aIndex + 1;
	}
	template <class T>
	inline int Heap<T>::GetRightChildIndex(const unsigned int aIndex)
	{
		return 2 * aIndex + 1;
	}
	template <class T>
	inline int Heap<T>::GetParentIndex(const unsigned int aIndex)
	{
		return (aIndex - 1) / 2;
	}
	template <class T>
	inline void Heap<T>::SiftDown()
	{
		int rootIndex = 0;

		while (GetLeftChildIndex(rootIndex) < myContainer.Size())
		{
			int childIndex = GetLeftChildIndex(rootIndex);
			int swapIndex = rootIndex;

			if (myContainer[swapIndex] < myContainer[childIndex])
			{
				swapIndex = childIndex;
			}
			if (childIndex + 1 < myContainer.Size() &&
				myContainer[swapIndex] < myContainer[childIndex + 1])
			{
				swapIndex = childIndex + 1;
			}
			if (swapIndex == rootIndex)
			{
				return;
			}
			else
			{
				T temp = myContainer[rootIndex];
				myContainer[rootIndex] = myContainer[swapIndex];
				myContainer[swapIndex] = temp;
				rootIndex = swapIndex;
			}
		}
	}
	template <class T>
	inline void Heap<T>::SiftUp()
	{
		int rootIndex = 0;
		int childIndex = myContainer.Size() - 1;
		while (childIndex > rootIndex) {
			int parentIndex = GetParentIndex(childIndex);

			if (myContainer[parentIndex] < myContainer[childIndex]) {
				T temp = myContainer[parentIndex];
				myContainer[parentIndex] = myContainer[childIndex];
				myContainer[childIndex] = temp;
				childIndex = parentIndex;
			}
			else
			{
				return;
			}
		}
	}
} 
