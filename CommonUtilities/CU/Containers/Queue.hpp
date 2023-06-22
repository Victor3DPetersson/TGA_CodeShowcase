#pragma once
#include <assert.h>

namespace CU
{
	template <class T, const unsigned int T_Size = 256>
	class Queue
	{
	public:
		Queue();
		unsigned int Size() const;
		const T& GetFront() const;
		T& GetFront();
		void Enqueue(const T& aValue);
		T Dequeue();
		void ClearQueue();
	private:
		T myContainer[T_Size];
		unsigned int myIterator;
		unsigned int myFront;
	};

	template<class T, const unsigned int T_Size>
	inline Queue<T, T_Size>::Queue()
	{
		myIterator = 0;
		myFront = 0;
	}

	template<class T, const unsigned int T_Size>
	inline unsigned int Queue<T, T_Size>::Size() const
	{
		return myIterator - myFront;
	}

	template<class T, const unsigned int T_Size>
	inline const T& Queue<T, T_Size>::GetFront() const
	{
		assert(myIterator != 0 && "Que is empty");
		return myContainer[myFront];
	}

	template<class T, const unsigned int T_Size>
	inline T& Queue<T, T_Size>::GetFront()
	{
		assert(myIterator != 0 && "Que is empty");
		return myContainer[myFront];
	}
	
	template<class T, const unsigned int T_Size>
	inline void Queue<T, T_Size>::Enqueue(const T& aValue)
	{
		assert(myIterator - myFront != T_Size && "Que Filled, increase size of Container");
		if (myIterator == T_Size)
		{
			T tempContainer[T_Size];
			for (unsigned int i = 0; i < T_Size - myFront; i++)
			{
				tempContainer[i] = myContainer[myFront + i];
			}
			myIterator -= myFront;
			myFront = 0;
			for (unsigned int i = 0; i < myIterator; i++)
			{
				myContainer[i] = tempContainer[i];
			}
		}
		myContainer[myIterator] = aValue;
		myIterator++;
	}

	template<class T, const unsigned int T_Size>
	inline T Queue<T, T_Size>::Dequeue()
	{
		assert(myIterator != 0 && "Que is empty");
		assert(myIterator != myFront && "Que is empty");
		T returnValue = myContainer[myFront];
		myContainer[myFront];
		myFront++;
		return returnValue;
	}
	template<class T, const unsigned int T_Size>
	inline void Queue<T, T_Size>::ClearQueue()
	{
		myIterator = 0;
		myFront = 0;
	}

	template <class T>
	class GrowingQueue
	{
	public:
		GrowingQueue()
			: myData(nullptr)
			, myCapacity(8Ui64)
			, myFront(0Ui64)
			, myEnd(0Ui64)
			, mySize(0Ui64)
		{
			myData = new T[myCapacity];
		}
		~GrowingQueue()
		{
			delete[] myData;
			myData = nullptr;
			myCapacity = 0Ui64;
			myFront = 0Ui64;
			myEnd = 0Ui64;
			mySize = 0Ui64;
		}
		const int& GetSize() const
		{
			return mySize;
		}
		const T& GetFront() const
		{
			assert(mySize);
			return myData[myFront];
		}
		T& GetFront()
		{
			assert(mySize);
			return myData[myFront];
		}
		void Enqueue(const T& aValue)
		{
			if (++mySize >= myCapacity)
			{
				myCapacity *= 2;
				T* arr = new T[myCapacity];
				for (int index = 0; index < mySize; ++index)
				{
					int indexInOldArray = myFront + index;
					if (indexInOldArray >= mySize)
					{
						indexInOldArray = indexInOldArray - (mySize - 1);
					}
					arr[index] = myData[indexInOldArray];
				}
				delete[] myData;
				myData = arr;
				myFront = 0;
				myEnd = mySize - 1;
			}
			else if (myEnd >= myCapacity)
			{
				myEnd = 0Ui64;
			}
			myData[myEnd++] = aValue;
		}
		T Dequeue()
		{
			assert(mySize);
			T data = myData[myFront];
			if (++myFront >= myCapacity)
			{
				myFront = 0Ui64;
			}
			--mySize;
			return data;
		}
		void Clear()
		{
			myFront = 0;
			myEnd = 0;
			mySize = 0;
		}
	private:
		T* myData;
		size_t myCapacity;
		uint64_t myFront;
		uint64_t myEnd;
		int mySize;
	};
}