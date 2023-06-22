#pragma once
#include <assert.h>
#include <functional>

namespace CU
{
	template <typename T, typename CountType = unsigned short>
	class GrowingArray
	{
	public:
		struct Iterator {
			using iteratorCategory = std::forward_iterator_tag;
			using differenceType = std::ptrdiff_t;
			using valueType = T;
			using pointer = T*;
			using reference = T&;
			Iterator(pointer aPtr) : myPtr(aPtr) {}

			reference operator*() const { return *myPtr; }
			pointer operator->() { return myPtr; }

			Iterator& operator++() { myPtr++; return *this; }
			Iterator operator++(int) { Iterator temp = *this; ++(*this); return temp; }

			friend bool operator== (const Iterator& a, const Iterator& b) { return a.myPtr == b.myPtr; }
			friend bool operator!= (const Iterator& a, const Iterator& b) { return a.myPtr != b.myPtr; }


		private:
			pointer myPtr;
		};

		GrowingArray();

		GrowingArray(CountType aAmountOfItems, bool aUseSafeModeFlag = true);
		GrowingArray(const GrowingArray& aGrowingArray);
		GrowingArray(GrowingArray&& aGrowingArray);

		~GrowingArray();

		GrowingArray& operator=(const GrowingArray& aGrowingArray);
		GrowingArray& operator=(GrowingArray&& aGrowingArray);

		void Init(CountType aAmountOfItems, bool aUseSafeModeFlag = true);
		void ReInit(CountType aAmountOfItems, bool aUseSafeModeFlag = true);
		const CountType Size() const;
		inline const CountType Capacity() const { return myArrayCapacity; }

		inline T& operator[](const CountType& aIndex);
		inline const T& operator[](const CountType& aIndex) const;

		inline void Add(const T& aObject);
		inline void AddDefaultObject();
		inline void Insert(const CountType& aIndex, const T& aObject);
		inline void DeleteCyclic(const T& aObject);
		inline void DeleteCyclicAtIndex(CountType aItemIndex);
		inline void RemoveCyclic(const T& aObject);
		inline void RemoveCyclicAtIndex(CountType aItemIndex);
		inline void RemoveAtIndex(CountType aItemIndex);
		inline CountType Find(const T& aObject);
		inline T& GetLast();
		inline const T& GetLast() const;
		inline void RemoveAll();
		inline void DeleteAll();
		inline void Fill();
		inline void CallFunctionOnAllMembers(std::function<void(T&)> aFunction);
		inline void CallFunctionOnAllMembers(std::function<void(const T&)> aFunction) const;
		inline const bool IsInitialized() const;

		void CopyOverContent(GrowingArray<T, CountType>& aArrayToGetDataFrom);
		void InsertDataBuffer(T* aPointer, CountType aAmountOfData);
		Iterator begin() { assert(myIsInitialized == true && "Growing Array is not initialized"); return Iterator(&myArrayPointer[0]); }
		Iterator end() { assert(myIsInitialized == true && "Growing Array is not initialized"); return Iterator(&myArrayPointer[myElementCount]); }



	private:
		void DumpAll();
		void GrowArray();
		void GrowArray(const CountType aGrowthAmount);

		T* myArrayPointer;
		CountType myElementCount;
		CountType myArrayCapacity;
		bool myUseSafeMode;
		bool myIsInitialized;
	};

	template <typename T, typename CountType>
	inline GrowingArray<T, CountType>::GrowingArray()
	{
		myArrayPointer = nullptr;
		myElementCount = 0;
		myArrayCapacity = 0;
		myUseSafeMode = true;
		myIsInitialized = false;
	}

	template <typename T, typename CountType>
	inline GrowingArray<T, CountType>::GrowingArray(CountType aAmountOfItems, bool aUseSafeModeFlag)
	{
		myArrayPointer = nullptr;
		myArrayCapacity = 0;
		myElementCount = 0;
		myUseSafeMode = aUseSafeModeFlag;
		myIsInitialized = false;
		Init(aAmountOfItems, aUseSafeModeFlag);
	}

	template <typename T, typename CountType>
	inline GrowingArray<T, CountType>::GrowingArray(const GrowingArray& aGrowingArray)
	{
		*this = aGrowingArray;
	}

	template<typename T, typename CountType>
	inline GrowingArray<T, CountType>::GrowingArray(GrowingArray&& aGrowingArray)
	{
		myArrayPointer = aGrowingArray.myArrayPointer;
		myElementCount = aGrowingArray.myElementCount;
		myIsInitialized = aGrowingArray.myIsInitialized;
		myArrayCapacity = aGrowingArray.myArrayCapacity;
		myUseSafeMode = aGrowingArray.myUseSafeMode;
		aGrowingArray.myIsInitialized = false;
		aGrowingArray.myArrayPointer = nullptr;
	}

	template <typename T, typename CountType>
	inline GrowingArray<T, CountType>::~GrowingArray()
	{
		DumpAll();
	}

	template <typename T, typename CountType>
	inline GrowingArray<typename T, typename CountType>&
		GrowingArray<T, CountType>::operator=(const GrowingArray& aGrowingArray)
	{
		if (myIsInitialized)
		{
			DumpAll();
		}
		Init(aGrowingArray.myArrayCapacity, myUseSafeMode);

		myElementCount = aGrowingArray.myElementCount;
		myIsInitialized = aGrowingArray.myIsInitialized;
		if (myUseSafeMode)
		{
			for (CountType index = 0; index < myElementCount; ++index)
			{
				myArrayPointer[index] = aGrowingArray[index];
			}
		}
		else
		{
			memcpy(&myArrayPointer[0], &aGrowingArray.myArrayPointer[0], sizeof(T) * myElementCount);
		}
		return (*this);
	}

	template<typename T, typename CountType>
	inline GrowingArray<T, CountType>& GrowingArray<T, CountType>::operator=(GrowingArray&& aGrowingArray)
	{
		myArrayPointer = aGrowingArray.myArrayPointer;
		myElementCount = aGrowingArray.myElementCount;
		myIsInitialized = aGrowingArray.myIsInitialized;
		myArrayCapacity = aGrowingArray.myArrayCapacity;
		myUseSafeMode = aGrowingArray.myUseSafeMode;
		aGrowingArray.myIsInitialized = false;
		aGrowingArray.myArrayPointer = nullptr;

		return *this;
	}

	template <typename T, typename CountType>
	inline void GrowingArray<T, CountType>::Init(CountType aAmountOfItems, bool aUseSafeModeFlag)
	{
		assert(myIsInitialized == false && "Growing Array should not initiated twice");
		if (aAmountOfItems == 0)
		{
			myArrayCapacity = 1;
		}
		else
		{
			myArrayCapacity = aAmountOfItems;
		}
		myUseSafeMode = aUseSafeModeFlag;
		myIsInitialized = true;
		myArrayPointer = new T[myArrayCapacity];
	}

	template <typename T, typename CountType>
	inline void GrowingArray<T, CountType>::ReInit(CountType aAmountOfItems, bool aUseSafeModeFlag)
	{
		assert(myIsInitialized == true && "Growing Array is not initialized");
		DumpAll();
		Init(aAmountOfItems, aUseSafeModeFlag);
	}

	template <typename T, typename CountType>
	inline const CountType GrowingArray<T, CountType>::Size() const
	{
		return myElementCount;
	}

	template <typename T, typename CountType>
	inline T& GrowingArray<T, CountType>::operator[](const CountType& aIndex)
	{
		assert(myIsInitialized == true && "Growing Array is not initialized");
		assert(aIndex < myElementCount && "Index is out of Range");
		assert(aIndex >= 0 && "Index can not be negative");
		return myArrayPointer[aIndex];
	}

	template <typename T, typename CountType>
	inline const T& GrowingArray<T, CountType>::operator[](const CountType& aIndex) const
	{
		assert(myIsInitialized == true && "Growing Array is not initialized");
		assert(aIndex < myElementCount && "Index is out of Range");
		assert(aIndex >= 0 && "Index can not be negative");
		return myArrayPointer[aIndex];
	}

	template <typename T, typename CountType>
	inline void GrowingArray<T, CountType>::Add(const T& aObject)
	{
		assert(myIsInitialized == true && "Growing Array is not initialized");
		if (myElementCount + 1 > (myArrayCapacity))
		{
			GrowArray();
		}
		myArrayPointer[myElementCount] = aObject;
		myElementCount++;
	}
	template <typename T, typename CountType>
	inline void GrowingArray<T, CountType>::AddDefaultObject()
	{
		assert(myIsInitialized == true && "Growing Array is not initialized");
		if (myElementCount + 1 > (myArrayCapacity))
		{
			GrowArray();
		}
		myElementCount++;
	}

	template <typename T, typename CountType>
	inline void GrowingArray<T, CountType>::Insert(const CountType& aIndex, const T& aObject)
	{
		assert(myIsInitialized == true && "Growing Array is not initialized");
		assert(aIndex >= 0 && "Index can not be negative");
		assert(aIndex <= myElementCount && "Index is out of range");
		T tempObject = myArrayPointer[myElementCount - 1];
		if (aIndex != myElementCount)
		{
			for (CountType index = myElementCount - 1; index > aIndex; --index)
			{
				myArrayPointer[index] = myArrayPointer[index - 1];
			}
		}
		myArrayPointer[aIndex] = aObject;
		Add(tempObject);
	}

	template <typename T, typename CountType>
	inline void GrowingArray<T, CountType>::DeleteCyclic(const T& aObject)
	{
		assert(myIsInitialized == true && "Growing Array is not initialized");
		const CountType objectIndex = Find(aObject);
		if (objectIndex != -1)
		{
			DeleteCyclicAtIndex(objectIndex);
		}
	}

	template <typename T, typename CountType>
	inline void GrowingArray<T, CountType>::DeleteCyclicAtIndex(CountType aIndex)
	{
		assert(myIsInitialized == true && "Growing Array is not initialized");
		assert(aIndex <= myElementCount && "Index is out of range");
		assert(aIndex >= 0 && "Index can not be negative");
		if (myElementCount != 1)
		{
			if (aIndex == myElementCount - 1)
			{
				delete myArrayPointer[aIndex];
				myArrayPointer[aIndex] = nullptr;
			}
			else
			{
				delete myArrayPointer[aIndex];
				myArrayPointer[aIndex] = myArrayPointer[myElementCount - 1];
			}
			--myElementCount;
		}
		else
		{
			delete myArrayPointer[0];
			myArrayPointer[0] = nullptr;
			RemoveAll();
		}
	}

	template <typename T, typename CountType>
	inline void GrowingArray<T, CountType>::RemoveCyclic(const T& aObject)
	{
		assert(myIsInitialized == true && "Growing Array is not initialized");

		const CountType objectIndex = Find(aObject);
		if (objectIndex != -1)
		{
			T temp = myArrayPointer[objectIndex];
			myArrayPointer[objectIndex] = myArrayPointer[myElementCount - 1];
			myArrayPointer[myElementCount - 1] = temp;
			--myElementCount;
		}
	}

	template <typename T, typename CountType>
	inline void GrowingArray<T, CountType>::RemoveCyclicAtIndex(CountType aItemIndex)
	{
		assert(myIsInitialized == true && "Growing Array is not initialized");
		assert(aItemIndex < myElementCount && "Index is out of range");
		assert(aItemIndex >= 0 && "Index can not be negative");
		if (myElementCount != 1)
		{
			myArrayPointer[aItemIndex] = myArrayPointer[myElementCount - 1];
			--myElementCount;
		}
		else
		{
			myElementCount = 0;
		}
	}

	template <typename T, typename CountType>
	inline void GrowingArray<T, CountType>::RemoveAtIndex(CountType aItemIndex)
	{
		assert(myIsInitialized == true && "Growing Array is not initialized");
		assert(aItemIndex < myElementCount && "Index is out of range");
		assert(aItemIndex >= 0 && "Index can not be negative");
		--myElementCount;
		for (int i = aItemIndex; i < myElementCount; ++i)
		{
			myArrayPointer[i] = myArrayPointer[i + 1];
		}
	}

	template <typename T, typename CountType>
	inline CountType GrowingArray<T, CountType>::Find(const T& aObject)
	{
		assert(myIsInitialized == true && "Growing Array is not initialized");
		for (CountType index = 0; index < myElementCount; ++index)
		{
			if (myArrayPointer[index] == aObject)
			{
				return index;
			}
		}
		return (CountType)-1;
	}

	template <typename T, typename CountType>
	inline T& GrowingArray<T, CountType>::GetLast()
	{
		assert(myIsInitialized == true && "Growing Array is not initialized");
		assert(myElementCount > 0 && "Vector is empty");
		return myArrayPointer[myElementCount - 1];
	}

	template <typename T, typename CountType>
	inline const T& GrowingArray<T, CountType>::GetLast() const
	{
		assert(myIsInitialized == true && "Growing Array is not initialized");
		assert(myElementCount > 0 && "Vector is empty");
		return myArrayPointer[myElementCount - 1];
	}

	template <typename T, typename CountType>
	inline void GrowingArray<T, CountType>::RemoveAll()
	{
		myElementCount = 0;
	}
	template <typename T, typename CountType>
	inline void GrowingArray<T, CountType>::Fill()
	{
		assert(myIsInitialized == true && "Growing Array is not initialized");
		myElementCount = myArrayCapacity;
	}
	template <typename T, typename CountType>
	inline void GrowingArray<T, CountType>::DeleteAll()
	{
		assert(myIsInitialized == true && "Growing Array is not initialized");
		//for (CountType i = 0; i < myElementCount; ++i)
		//{
		//	delete myArrayPointer[i];
		//	myArrayPointer = nullptr;
		//}
		delete[] myArrayPointer;		
		myArrayPointer = nullptr;
		RemoveAll();
	}

	template <typename T, typename CountType>
	inline void GrowingArray<T, CountType>::CallFunctionOnAllMembers(std::function<void(T&)> aFunction)
	{
		assert(myIsInitialized == true && "Growing Array is not initialized");
		for (CountType i = 0; i < myElementCount; i++)
		{
			aFunction((*this)[i]);
		}
	}

	template <typename T, typename CountType>
	inline void
		GrowingArray<T, CountType>::CallFunctionOnAllMembers(std::function<void(const T&)> aFunction) const
	{
		assert(myIsInitialized == true && "Growing Array is not initialized");
		for (CountType i = 0; i < myElementCount; i++)
		{
			aFunction((*this)[i]);
		}
	}

	template <typename T, typename CountType>
	inline const bool GrowingArray<T, CountType>::IsInitialized() const
	{
		return myIsInitialized;
	}

	template<typename T, typename CountType>
	void GrowingArray<T, CountType>::CopyOverContent(GrowingArray<T, CountType>& aArrayToGetDataFrom)
	{
		assert(myIsInitialized == true && "Growing Array is not initialized");
		assert(aArrayToGetDataFrom.myIsInitialized == true && "Growing Array to get Data from is not initialized");

		memcpy(&myArrayPointer[0], &aArrayToGetDataFrom.myArrayPointer[0], sizeof(T) * aArrayToGetDataFrom.myArrayCapacity);
		myElementCount = aArrayToGetDataFrom.myArrayCapacity;
	}

	template<typename T, typename CountType>
	inline void GrowingArray<T, CountType>::InsertDataBuffer(T* aPointer, CountType aAmountOfData)
	{
		if (myIsInitialized)
		{
			DumpAll();
			Init(aAmountOfData);
		}
		else
		{
			Init(aAmountOfData);
		}
		memcpy(&myArrayPointer[0], &aPointer[0], sizeof(T) * aAmountOfData);
		myElementCount = aAmountOfData;
	}

	template <typename T, typename CountType>
	inline void GrowingArray<T, CountType>::DumpAll()
	{
		if (myIsInitialized)
		{
			delete[] myArrayPointer;
			myArrayPointer = nullptr;
			myArrayCapacity = 0;
		}
		myIsInitialized = false;
		RemoveAll();
	}

	template <typename T, typename CountType>
	inline void GrowingArray<T, CountType>::GrowArray()
	{
		assert(myIsInitialized == true && "Growing Array is not initialized");
		GrowArray(myArrayCapacity * 2);
	}

	template <typename T, typename CountType>
	inline void GrowingArray<T, CountType>::GrowArray(const CountType aGrowthAmount)
	{
		assert(myIsInitialized == true && "Growing Array is not initialized");
		T* tempArrayPointer = myArrayPointer;
		const CountType tempCount = myElementCount;
		myIsInitialized = false;
		Init(aGrowthAmount);
		if (myUseSafeMode)
		{
			for (CountType i = 0; i < tempCount; ++i)
			{
				myArrayPointer[i] = std::move(tempArrayPointer[i]);
			}
		}
		else
		{
			memcpy(myArrayPointer, tempArrayPointer, (sizeof(T) * tempCount));
		}
		delete[] tempArrayPointer;
		tempArrayPointer = nullptr;
	}

}