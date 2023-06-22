#pragma once
#include <stdint.h>
#include <initializer_list>
#include "../CommonUtilities/CU/Utility/Hash.h"

namespace CU
{
	constexpr size_t dictionaryDefaultCapacity = 32U;
	constexpr size_t capacityThreshold = 2U;

	/**
	* \brief Dictionary - Associative Growing Array implemented through a Hash Table.
	* 
	* \param Key - Type used for keys. Has to have a CU::CU_Hash() that takes it as an argument.
	* \param Value - Type used for values.
	* \param R - Amount of pairs to move from old table when inserting to new table.
	**/
	template <class Key, class Value = int, size_t R = 2>
	class Dictionary
	{
		static_assert(R > 0);

	public:
		/*** PUBLIC INTERFACE ***/

		struct KeyValuePair
		{
			Key key{};
			Value value{};
		};

		/* Constructors */

		Dictionary()
			: myHashTables{ { 0,0,0,0,0 } }
			, myWriteTable(0)
			, myMovingFromTable(size_t(-1))
			, myMovingFromTableMarker(0)
		{
			__AllocTable(0, dictionaryDefaultCapacity);
		}
		Dictionary(size_t aCapacity)
			: myHashTables{ { 0,0,0,0,0 } }
			, myWriteTable(0)
			, myMovingFromTable(size_t(-1))
			, myMovingFromTableMarker(0)
		{
			size_t size = aCapacity;
			--size;
			size |= size >> 1;
			size |= size >> 2;
			size |= size >> 4;
			size |= size >> 8;
			size |= size >> 16;
			size |= size >> 32;
			++size;

			__AllocTable(0, size);
		}
		Dictionary(std::initializer_list<KeyValuePair> anIList)
			: myHashTables{ { 0,0,0,0,0 } }
			, myWriteTable(0)
			, myMovingFromTable(size_t(-1))
			, myMovingFromTableMarker(0)
		{
			size_t count = anIList.size();
			size_t size = count;
			--size;
			size |= size >> 1;
			size |= size >> 2;
			size |= size >> 4;
			size |= size >> 8;
			size |= size >> 16;
			size |= size >> 32;
			++size;
			size <<= 1;

			__AllocTable(0, size);

			const auto it = anIList.begin();
			for (size_t index = 0U; index < count; ++index)
			{
				const auto i = it + index;
				Insert(i->key, i->value);
			}
		}

		/* Destructor */

		~Dictionary()
		{
			__FreeTable(0);
			__FreeTable(1);
		}

		/* Copying and Moving */

		Dictionary(const Dictionary<Key, Value, R>& aDict)
			: myHashTables{ { 0,0,0,0,0 } }
			, myWriteTable(0)
			, myMovingFromTable(size_t(-1))
			, myMovingFromTableMarker(0)
		{
			this->~Dictionary();

			if (aDict.myHashTables[0].states)
			{
				__Copy(aDict, 0);
			}
			if (aDict.myHashTables[1].states)
			{
				__Copy(aDict, 1);
			}

			myWriteTable = aDict.myWriteTable;
			myMovingFromTable = aDict.myMovingFromTable;
			myMovingFromTableMarker = aDict.myMovingFromTableMarker;
		}
		Dictionary(Dictionary<Key, Value, R>&& aDict) noexcept
			: myHashTables{ { 0,0,0,0,0 } }
			, myWriteTable(0)
			, myMovingFromTable(size_t(-1))
			, myMovingFromTableMarker(0)
		{
			this->~Dictionary();
			//Elia är grym
			__Move((Dictionary&&)aDict, 0);
			__Move((Dictionary&&)aDict, 1);

			myWriteTable = aDict.myWriteTable;
			myMovingFromTable = aDict.myMovingFromTable;
			myMovingFromTableMarker = aDict.myMovingFromTableMarker;

			aDict.myMovingFromTable = size_t(-1);
			aDict.myWriteTable = size_t(-1);
			aDict.myMovingFromTableMarker = size_t(-1);
		}
		Dictionary& operator=(const Dictionary<Key, Value, R>& aDict)
		{
			this->~Dictionary();

			if (aDict.myHashTables[0].states)
			{
				__Copy(aDict, 0);
			}
			if (aDict.myHashTables[1].states)
			{
				__Copy(aDict, 1);
			}

			myWriteTable = aDict.myWriteTable;
			myMovingFromTable = aDict.myMovingFromTable;
			myMovingFromTableMarker = aDict.myMovingFromTableMarker;

			return *this;
		}
		Dictionary& operator=(Dictionary<Key, Value, R>&& aDict) noexcept
		{
			this->~Dictionary();

			__Move((Dictionary&&)aDict, 0);
			__Move((Dictionary&&)aDict, 1);

			myWriteTable = aDict.myWriteTable;
			myMovingFromTable = aDict.myMovingFromTable;
			myMovingFromTableMarker = aDict.myMovingFromTableMarker;

			aDict.myMovingFromTable = size_t(-1);
			aDict.myWriteTable = size_t(-1);
			aDict.myMovingFromTableMarker = size_t(-1);

			return *this;
		}

		/* Modifiers */

		inline Value* Insert(const Key& aKey, const Value& aValue)
		{
			/* Grow table */
			if (myHashTables[myWriteTable].size >= myHashTables[myWriteTable].capacity / capacityThreshold)
			{
				myMovingFromTable = myWriteTable;
				myWriteTable = !myWriteTable;
				myMovingFromTableMarker = 0U;

				/* The capacity has to be at least (R+1)/R times bigger to ensure we never run out of space in the new list before the old list is empty. */
				const size_t newCapacity = myHashTables[myMovingFromTable].capacity * 2U;

				size_t result;
				if (myHashTables[myWriteTable].states)
				{
					result = __ReallocTable(myWriteTable, newCapacity);
				}
				else
				{
					result = __AllocTable(myWriteTable, newCapacity);
				}

				if (result == size_t(-1)) return nullptr;
			}

			/* Insert to write table */
			Value* const val = __InsertToWriteTable(aKey, aValue);

			/* Move from table */
			if (myMovingFromTable != size_t(-1))
			{
				auto& ht = myHashTables[myMovingFromTable];
				const size_t capacity = ht.capacity;
				size_t moveCounter = 0U;
				for (; myMovingFromTableMarker < capacity; ++myMovingFromTableMarker)
				{
					if (__GetStateAtHashCode(myMovingFromTableMarker, myMovingFromTable) == SlotState_Used)
					{
						__InsertToWriteTable(ht.keys[myMovingFromTableMarker], ht.values[myMovingFromTableMarker]);
						--ht.size;
						__SetStateAtHashCode(myMovingFromTableMarker, myMovingFromTable, SlotState_Removed);
						if (++moveCounter >= R)
						{
							break;
						}
					}
				}
				if (ht.size == 0)
				{
					myMovingFromTable = size_t(-1);
				}
			}

			return val;
		}
		inline void Remove(const Key& aKey)
		{
			{
				const size_t hashCode = __FindInTable(myWriteTable, aKey);
				if (hashCode != size_t(-1))
				{
					__SetStateAtHashCode(hashCode, myWriteTable, SlotState_Removed);
					--myHashTables[myWriteTable].size;
					return;
				}
			}

			if (myMovingFromTable != size_t(-1))
			{
				const size_t hashCode = __FindInTable(myMovingFromTable, aKey);
				if (hashCode != size_t(-1))
				{
					__SetStateAtHashCode(hashCode, myMovingFromTable, SlotState_Removed);
					--myHashTables[myMovingFromTable].size;
					return;
				}
			}
		}
		inline void Clear()
		{
			__ClearTable(0);
			__ClearTable(1);
			myMovingFromTable = size_t(-1);
		}

		/* Access */

		inline Value* Get(const Key& aKey)
		{
			return __Get(aKey);
		}
		inline const Value* Get(const Key& aKey) const
		{
			return __Get(aKey);
		}
		inline void ForEach(void (*aCallback)(Key&, Value&, Dictionary&))
		{
			__ForEachTable(myWriteTable, aCallback);
			if (myMovingFromTable != size_t(-1))
			{
				__ForEachTable(myMovingFromTable, aCallback);
			}
		}
		inline void ForEach(void (*aCallback)(Key&, Value&, Dictionary&, void*), void* aData)
		{
			__ForEachTable(myWriteTable, aCallback, aData);
			if (myMovingFromTable != size_t(-1))
			{
				__ForEachTable(myMovingFromTable, aCallback, aData);
			}
		}
		inline void ForEach(void (*aCallback)(const Key&, const Value&, const Dictionary&)) const
		{
			__ForEachTable(myWriteTable, aCallback);
			if (myMovingFromTable != size_t(-1))
			{
				__ForEachTable(myMovingFromTable, aCallback);
			}
		}
		inline void ForEach(void (*aCallback)(const Key&, const Value&, const Dictionary&, const void*), const void* const aData) const
		{
			__ForEachTable(myWriteTable, aCallback, aData);
			if (myMovingFromTable != size_t(-1))
			{
				__ForEachTable(myMovingFromTable, aCallback, aData);
			}
		}
		/* Return false to break, return true to continue. */
		inline void ForEach(bool (*aCallback)(const Key&, Value&, Dictionary&))
		{
			if (!__ForEachTable(myWriteTable, aCallback))
			{
				return;
			}
			if (myMovingFromTable != size_t(-1))
			{
				if (!__ForEachTable(myMovingFromTable, aCallback))
				{
					return;
				}
			}
		}
		/* Return false to break, return true to continue. */
		inline void ForEach(bool (*aCallback)(const Key&, Value&, Dictionary&, void*), void* aData)
		{
			if (!__ForEachTable(myWriteTable, aCallback, aData))
			{
				return;
			}
			if (myMovingFromTable != size_t(-1))
			{
				if (!__ForEachTable(myMovingFromTable, aCallback, aData))
				{
					return;
				}
			}
		}

		Value* operator[] (const Key& aKey)
		{
			Value* const val = __Get(aKey);
			if (val) return val;

			return Insert(aKey, Value());
		}
		const Value* operator[] (const Key& aKey) const
		{
			return __Get(aKey);
		}

		/* Capacity */

		inline bool Empty() const
		{
			return !(myHashTables[0].size + myHashTables[1].size);
		}
		inline size_t Size() const
		{
			return myHashTables[0].size + myHashTables[1].size;
		}
		inline size_t Capacity() const
		{
			return myHashTables[myWriteTable].capacity + ((myMovingFromTable != size_t(-1)) ? myHashTables[myMovingFromTable].capacity : 0Ui64);
		}

		/* Lookup */

		inline bool Contains(const Key& aKey) const
		{
			if (__ContainsTable(myWriteTable, aKey)) return true;
			if (myMovingFromTable != size_t(-1) && __ContainsTable(myMovingFromTable, aKey)) return true;
			return false;
		}

	private:
		/*** DATA ***/

		struct HashTable
		{
			uint64_t* states;
			Key* keys;
			Value* values;
			size_t capacity;
			size_t size = 0;
		};

		HashTable myHashTables[2];

		size_t myWriteTable;
		size_t myMovingFromTable;
		/* Basically a for-loop iterator for where we are in the moving of the old table. */
		size_t myMovingFromTableMarker;

		/*** INTERNAL METHODS ***/

		enum SlotState_ : uint64_t
		{
			SlotState_Empty = 0b00, SlotState_Used = 0b01, SlotState_Removed = 0b10
		};

		inline size_t __AllocTable(size_t anIndex, size_t aCapacity)
		{
			const size_t slotStateSize = sizeof(uint64_t) * (aCapacity / 32 + 1);
			const size_t keySize = sizeof(Key) * aCapacity;
			const size_t valueSize = sizeof(Value) * aCapacity;

			char* buffer = (char*)malloc(slotStateSize + keySize + valueSize);
			if (!buffer)
			{
				assert(false && "Malloc failed.");
				return size_t(-1);
			}

			myHashTables[anIndex].states = (uint64_t*)buffer;
			myHashTables[anIndex].keys = (Key*)(buffer + slotStateSize);
			myHashTables[anIndex].values = (Value*)(buffer + slotStateSize + keySize);
			myHashTables[anIndex].capacity = aCapacity;
			myHashTables[anIndex].size = 0U;

			memset(myHashTables[anIndex].states, 0, slotStateSize);

			myWriteTable = anIndex;

			return slotStateSize + keySize + valueSize;
		}

		inline size_t __ReallocTable(size_t anIndex, size_t aCapacity)
		{
			const size_t slotStateSize = sizeof(uint64_t) * (aCapacity / 32 + 1);
			const size_t keySize = sizeof(Key) * aCapacity;
			const size_t valueSize = sizeof(Value) * aCapacity;

			char* buffer = (char*)realloc(myHashTables[anIndex].states, slotStateSize + keySize + valueSize);
			if (!buffer)
			{
				assert(false && "Realloc failed.");
				return size_t(-1);
			}

			myHashTables[anIndex].states = (uint64_t*)buffer;
			myHashTables[anIndex].keys = (Key*)(buffer + slotStateSize);
			myHashTables[anIndex].values = (Value*)(buffer + slotStateSize + keySize);
			myHashTables[anIndex].capacity = aCapacity;
			myHashTables[anIndex].size = 0U;

			memset(myHashTables[anIndex].states, 0, slotStateSize);

			myWriteTable = anIndex;

			return slotStateSize + keySize + valueSize;
		}

		inline void __FreeTable(size_t anIndex)
		{
			if (myHashTables[anIndex].states)
			{
				free(myHashTables[anIndex].states);
				myHashTables[anIndex] = { 0,0,0,0,0 };
			}
		}

		inline size_t __GetHashCode(const Key& aKey, size_t aCapacity) const
		{
			return CU_Hash(aKey) % aCapacity;
		}

		inline void __SetStateAtHashCode(size_t aHashCode, size_t aTableIndex, uint64_t aSlotState)
		{
			const size_t stateIndex = aHashCode / 32;
			const size_t shiftAmount = (aHashCode - (stateIndex << 5)) << 1;
			myHashTables[aTableIndex].states[stateIndex] &= ~(0b11Ui64 << shiftAmount);
			myHashTables[aTableIndex].states[stateIndex] |= (aSlotState << shiftAmount);
		}
		inline uint64_t __GetStateAtHashCode(size_t aHashCode, size_t aTableIndex) const
		{
			const size_t stateIndex = aHashCode / 32;
			const size_t shiftAmount = (aHashCode - (stateIndex << 5)) << 1;
			const size_t state = myHashTables[aTableIndex].states[stateIndex];
			const size_t shiftedState = state >> shiftAmount;
			const size_t returnValue = shiftedState & 0b11;
			return returnValue;
		}

		inline void __Copy(const Dictionary& aDict, size_t anIndex)
		{
			const size_t size = __AllocTable(anIndex, aDict.myHashTables[anIndex].capacity);
			memcpy(myHashTables[anIndex].states, aDict.myHashTables[anIndex].states, size);
			myHashTables[anIndex].size = aDict.myHashTables[anIndex].size;
		}
		inline void __Move(Dictionary&& aDict, size_t anIndex)
		{
			myHashTables[anIndex].states = aDict.myHashTables[anIndex].states;
			myHashTables[anIndex].keys = aDict.myHashTables[anIndex].keys;
			myHashTables[anIndex].values = aDict.myHashTables[anIndex].values;
			myHashTables[anIndex].capacity = aDict.myHashTables[anIndex].capacity;
			myHashTables[anIndex].size = aDict.myHashTables[anIndex].size;

			aDict.myHashTables[anIndex].states = 0;
			aDict.myHashTables[anIndex].keys = 0;
			aDict.myHashTables[anIndex].values = 0;
			aDict.myHashTables[anIndex].capacity = 0;
			aDict.myHashTables[anIndex].size = 0;
		}

		inline size_t __FindInTable(size_t anIndex, const Key& aKey) const
		{
			const size_t capacity = myHashTables[anIndex].capacity;
			if (!capacity) return size_t(-1);

			const size_t originHashCode = __GetHashCode(aKey, capacity);
			size_t hashCode = originHashCode;

			while (true)
			{
				switch (__GetStateAtHashCode(hashCode, anIndex))
				{
				case SlotState_Used:
				{
					if (myHashTables[anIndex].keys[hashCode] == aKey)
					{
						return hashCode;
					}

					__fallthrough;
				}
				case SlotState_Removed:
				{
					hashCode = (hashCode + 1) % capacity;
					if (hashCode == originHashCode)
					{
						return size_t(-1);
					}

					break;
				}
				case SlotState_Empty:
				{
					return size_t(-1);
				}
				}
			}

			return 0;
		}

		inline Value* __Get(const Key& aKey) const
		{
			const size_t table1Index = __FindInTable(myWriteTable, aKey);
			if (table1Index != size_t(-1))
			{
				return myHashTables[myWriteTable].values + table1Index;
			}
			if (myMovingFromTable != size_t(-1))
			{
				const size_t table2Index = __FindInTable(myMovingFromTable, aKey);
				if (table2Index != size_t(-1))
				{
					return myHashTables[myMovingFromTable].values + table2Index;
				}
			}

			return nullptr;
		}

		inline void __ClearTable(size_t anIndex)
		{
			if (myHashTables[anIndex].states)
			{
				const size_t slotStateSize = sizeof(uint64_t) * (myHashTables[anIndex].capacity / 32 + 1);
				memset(myHashTables[anIndex].states, 0, slotStateSize);
				myHashTables[anIndex].size = 0U;
			}
		}

		inline bool __ContainsTable(size_t aTable, const Key& aKey) const
		{
			return __FindInTable(aTable, aKey) != size_t(-1);
		}

		inline void __ForEachTable(size_t aTable, void (*aCallback)(Key&, Value&, Dictionary&))
		{
			HashTable& table = myHashTables[aTable];
			for (size_t hashCode = 0U; hashCode < table.capacity; ++hashCode)
			{
				if (__GetStateAtHashCode(hashCode, aTable) == SlotState_Used)
				{
					aCallback(table.keys[hashCode], table.values[hashCode], *this);
				}
			}
		}
		inline void __ForEachTable(size_t aTable, void (*aCallback)(Key&, Value&, Dictionary&, void*), void* aData)
		{
			HashTable& table = myHashTables[aTable];
			for (size_t hashCode = 0U; hashCode < table.capacity; ++hashCode)
			{
				if (__GetStateAtHashCode(hashCode, aTable) == SlotState_Used)
				{
					aCallback(table.keys[hashCode], table.values[hashCode], *this, aData);
				}
			}
		}
		inline void __ForEachTable(size_t aTable, void (*aCallback)(const Key&, const Value&, const Dictionary&)) const
		{
			const HashTable& table = myHashTables[aTable];
			for (size_t hashCode = 0U; hashCode < table.capacity; ++hashCode)
			{
				if (__GetStateAtHashCode(hashCode, aTable) == SlotState_Used)
				{
					aCallback(table.keys[hashCode], table.values[hashCode], *this);
				}
			}
		}
		inline void __ForEachTable(size_t aTable, void (*aCallback)(const Key&, const Value&, const Dictionary&, const void*), const void* const aData) const
		{
			const HashTable& table = myHashTables[aTable];
			for (size_t hashCode = 0U; hashCode < table.capacity; ++hashCode)
			{
				if (__GetStateAtHashCode(hashCode, aTable) == SlotState_Used)
				{
					aCallback(table.keys[hashCode], table.values[hashCode], *this, aData);
				}
			}
		}
		inline bool __ForEachTable(size_t aTable, bool (*aCallback)(const Key&, Value&, Dictionary&))
		{
			const HashTable& table = myHashTables[aTable];
			for (size_t hashCode = 0U; hashCode < table.capacity; ++hashCode)
			{
				if (__GetStateAtHashCode(hashCode, aTable) == SlotState_Used)
				{
					if (!aCallback(table.keys[hashCode], table.values[hashCode], *this))
					{
						return false;
					}
				}
			}

			return true;
		}
		inline bool __ForEachTable(size_t aTable, bool (*aCallback)(const Key&, Value&, Dictionary&, void*), void* aData)
		{
			const HashTable& table = myHashTables[aTable];
			for (size_t hashCode = 0U; hashCode < table.capacity; ++hashCode)
			{
				if (__GetStateAtHashCode(hashCode, aTable) == SlotState_Used)
				{
					if (!aCallback(table.keys[hashCode], table.values[hashCode], *this, aData))
					{
						return false;
					}
				}
			}

			return true;
		}

		inline Value* __InsertToWriteTable(const Key& aKey, const Value& aValue)
		{
			const size_t capacity = myHashTables[myWriteTable].capacity;
			const size_t originalHashCode = __GetHashCode(aKey, capacity);
			size_t hashCode = originalHashCode;
			while (true)
			{
				switch (__GetStateAtHashCode(hashCode, myWriteTable))
				{
				case SlotState_Used:
				{
					hashCode = (hashCode + 1) % capacity;
					if (hashCode == originalHashCode)
					{
						assert(!"This should never ever ever ever ever ever happen.");
						return nullptr;
					}

					break;
				}
				case SlotState_Empty:
				{
					new (myHashTables[myWriteTable].keys + hashCode) Key;
					new (myHashTables[myWriteTable].values + hashCode) Value;

					__fallthrough;
				}
				case SlotState_Removed:
				{
					myHashTables[myWriteTable].keys[hashCode] = aKey;
					myHashTables[myWriteTable].values[hashCode] = aValue;
					__SetStateAtHashCode(hashCode, myWriteTable, SlotState_Used);
					++myHashTables[myWriteTable].size;

					return myHashTables[myWriteTable].values + hashCode;
				}
				}
			}
		}
	};

	template <class Q, class W, size_t R = 2>
	using GrowingDict = Dictionary<Q, W, R>;
}


;