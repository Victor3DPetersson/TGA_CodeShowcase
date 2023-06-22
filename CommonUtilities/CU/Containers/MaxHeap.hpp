#pragma once
#include "GrowingArray.hpp"
#include <assert.h>

namespace CU
{
	template <class T>
	class MaxHeap
	{
	public:
		MaxHeap();
		int Size() const;             // returnerar antal element i heapen
		void Enqueue(const T& aElement); // lägger till elementet i heapen
		const T& GetTop() const;         // returnerar det största elementet i heapen
		T Dequeue();                     // tar bort det största elementet ur heapen och returnerar det
	private:
		unsigned short GetLeftChildIndex(const unsigned short aIndex);
		unsigned short GetRightChildIndex(const unsigned short aIndex);
		unsigned short GetParentIndex(const unsigned short aIndex);
		void SiftDown();
		void SiftUp();
		GrowingArray<T> myContainer;
	};

	template <class T>
	inline MaxHeap<T>::MaxHeap()
	{
		myContainer.Init(100);
	}

	template <class T>
	inline int MaxHeap<T>::Size() const
	{
		return myContainer.Size();
	}

	template <class T>
	inline void MaxHeap<T>::Enqueue(const T& aElement)
	{
		myContainer.Add(aElement);
		SiftUp();
	}

	template <class T>
	inline const T& MaxHeap<T>::GetTop() const
	{
		assert(myContainer.Size() >= 1 && "Heap is Empty");
		return myContainer[0];
	}

	template <class T>
	inline T MaxHeap<T>::Dequeue()
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
	inline unsigned short MaxHeap<T>::GetLeftChildIndex(const unsigned short aIndex)
	{
		return 2 * aIndex + 1;
	}
	template <class T>
	inline unsigned short MaxHeap<T>::GetRightChildIndex(const unsigned short aIndex)
	{
		return 2 * aIndex + 1;
	}
	template <class T>
	inline unsigned short MaxHeap<T>::GetParentIndex(const unsigned short aIndex)
	{
		return (aIndex - 1) / 2;
	}
	template <class T>
	inline void MaxHeap<T>::SiftDown()
	{
		unsigned short rootIndex = 0;

		while (GetLeftChildIndex(rootIndex) < myContainer.Size())
		{
			unsigned short childIndex = GetLeftChildIndex(rootIndex);
			unsigned short swapIndex = rootIndex;

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
	inline void MaxHeap<T>::SiftUp()
	{
		unsigned short rootIndex = 0;
		unsigned short childIndex = static_cast<unsigned short>(myContainer.Size() - 1);
		while (childIndex > rootIndex) {
			unsigned short parentIndex = GetParentIndex(childIndex);

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
} // namespace CommonUtilities
