#pragma once
#include <initializer_list>
#include <assert.h>

#pragma warning(push)
#pragma warning(disable : 4244)

namespace CU
{
	template <typename Type, unsigned int size, typename CountType = unsigned short, bool UseSafeModeFlag = true>
	class VectorOnStack
	{
	public:
		VectorOnStack() : myCapacity(size){	} 
		VectorOnStack(const std::initializer_list<Type>& aInitList) : myCount(static_cast<CountType>(aInitList.size())), myCapacity(size)
		{
			int counter{ 0 };
			assert(aInitList.size() <= static_cast<unsigned int>(size) && "List initialization larger than size.");
			for (Type object : aInitList)
			{
				myContainer[counter] = object;
				counter++;
			}
		}

		VectorOnStack(const VectorOnStack& aVectorOnStack)
		{
			if (UseSafeModeFlag)
			{
				this->myCount = aVectorOnStack.myCount;
				this->myCapacity = aVectorOnStack.myCapacity;
				for (int i = 0; i < Size(); i++)
				{
					this->myContainer[i] = aVectorOnStack[i];
				}
			}
			else
			{
				this->myCount = aVectorOnStack.myCount;
				this->myCapacity = aVectorOnStack.myCapacity;
				memcpy(&this->myContainer, &aVectorOnStack.myContainer, sizeof(aVectorOnStack.myContainer));
			}
		}

		~VectorOnStack()
		{
		}

		VectorOnStack& operator=(const VectorOnStack& aVectorOnStack)
		{
			if constexpr (UseSafeModeFlag == true)
			{
				this->myCount = aVectorOnStack.myCount;
				this->myCapacity = aVectorOnStack.myCapacity;
				for (int i = 0; i < Size(); i++)
				{
					this->myContainer[i] = aVectorOnStack[i];
				}
			}
			else
			{
				this->myCount = aVectorOnStack.myCount;
				this->myCapacity = aVectorOnStack.myCapacity;
				memcpy(&this->myContainer, &aVectorOnStack.myContainer, sizeof(aVectorOnStack.myContainer));
			}

			return *this;
		}

		inline VectorOnStack<Type, size, CountType>& Init(const Type* const arr, size_t count)
		{
			memcpy(myContainer, arr, sizeof(Type) * count);
			myCount = count;

			return *this;
		}

		inline const Type& operator[](const CountType aIndex) const
		{
			assert(aIndex < Size() && "Index out of Range.");
			assert(aIndex >= 0 && "Index out of Range.");
			return myContainer[aIndex];
		}

		inline Type& operator[](const CountType aIndex)
		{
			assert(aIndex < Size() && "Index out of Range.");
			assert(aIndex >= 0 && "Index out of Range.");
			return myContainer[aIndex];
		}


		inline void Add(const Type& aObject)
		{
			assert(myCount < myCapacity && "Array is Full.");
			myContainer[myCount] = aObject;
			myCount++;
		}

		inline Type& PushDefault()
		{
			assert(myCount < myCapacity && "Array is Full.");
			myContainer[myCount] = Type();

			return myContainer[myCount++];
		}

		inline void Insert(const CountType aIndex, const Type& aObject)
		{
			assert(myCount < myCapacity && "Array is Full.");
			assert(aIndex < Size() && "Index out of Range.");
			assert(aIndex >= 0 && "Index out of Range.");
			if (UseSafeModeFlag) {
				for (CountType i = Size(); i > aIndex; i--)
				{
					myContainer[i] = myContainer[i - 1];
				}
			}
			else if (aIndex != Size()) {
				CountType numbersOfElementsToCopy = Size() - aIndex;
				memcpy(&myContainer[aIndex + 1], &myContainer[aIndex], sizeof(Type) * numbersOfElementsToCopy);
			}
			myContainer[aIndex] = aObject;
			myCount++;
		}

		inline void RemoveCyclic(const Type& aObject)
		{
			assert(Size() == 0 && "Array is Empty.");
			for (CountType i = Size(); i >= 0; i--)
			{
				if (myContainer[i] == aObject)
				{
					myContainer[i] = FindLastElementInArray();
					myCount--;
					break;
				}
			}
		}

		inline void RemoveCyclicAtIndex(const CountType aIndex)
		{
			assert(aIndex < Size() && "Index out of Range.");
			assert(aIndex >= 0 && "Index out of Range.");
			myContainer[aIndex] = FindLastElementInArray();
			myCount--;
		}

		inline void Clear()
		{			
			myCount = 0;
		}

		inline CountType Size() const
		{
			return myCount;
		}

		inline void SetSize(CountType size)
		{
			myCount = size;
		}

		inline Type* Data()
		{
			return myContainer;
		}

	private:

		Type FindLastElementInArray()
		{
			if (myCount == 0)
			{
				return myContainer[size];
			}
			return myContainer[Size() - 1];
		}

		Type myContainer[size + 1];
		CountType myCount{};
		CountType myCapacity{};
	};
}

template <typename Type, unsigned int size, typename CountType = unsigned short, bool UseSafeModeFlag = true>
using VictorOnCrack = CU::VectorOnStack<Type, size, CountType, UseSafeModeFlag>;

#pragma warning(pop)