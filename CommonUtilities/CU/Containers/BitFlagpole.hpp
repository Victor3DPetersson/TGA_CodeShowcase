#pragma once

#include <stdint.h>
#include <assert.h>
#include <intrin.h>

#define PO2(a) (1Ui32 << uint32_t(a))

/*
* -- USAGE EXAMPLE --
*
* enum ListOfEpicDanceMoves
* {
*	ListOfEpicDanceMoves_None,

*	ListOfEpicDanceMoves_Dab = PO2(0),
*	ListOfEpicDanceMoves_Floss = PO2(1),
*	ListOfEpicDanceMoves_GangnamStyle = PO2(2)
* };
*
* // alt.
*
* enum class ListOfEpicerDanceMoves
* {
*	None,
*
*	Nej = PO2(0),
*	Macarena = PO2(1),
*	Melkersson = PO2(2)
* };
*
* void Foo()
* {
*	CU::BitFlagpole16 fp(ListOfEpicDanceMoves_Dab | ListOfEpicDanceMoves_Floss);
*
*	fp.Test(ListOfEpicDanceMoves_Dab); // true
*	fp[ListOfEpicDanceMoves_Floss]; // true
*	fp[ListOfEpicerDanceMoves::Nej]; // false
*	fp.FlipAll();
*	fp.Test(ListOfEpicerDanceMoves::Melkersson); // true
* }
*/

namespace CU
{
	class BitFlagpole8
	{
		using StorageType = uint8_t;
		static constexpr size_t bitCount = sizeof(StorageType) * 8U;
		static constexpr size_t byteCount = sizeof(StorageType);

	public:
		/* Constructors & Destructor */
		inline BitFlagpole8()
			: myData(0U)
		{}
		inline BitFlagpole8(StorageType aRawValue)
			: myData(aRawValue)
		{}
		template <class EnumClass>
		inline BitFlagpole8(EnumClass anEnumValue)
			: myData(StorageType(anEnumValue))
		{}
		~BitFlagpole8() = default;

		inline BitFlagpole8(const BitFlagpole8& another)
			: myData(another.myData)
		{}
		inline BitFlagpole8(BitFlagpole8&& another) noexcept
			: myData(another.myData)
		{}

		/* Getters */
		inline bool Test(StorageType aBit) const
		{
			assert(("Index out of range.", aBit < 256));

			return myData & aBit;
		}
		template <class EnumClass>
		inline bool Test(EnumClass aBit) const
		{
			assert(("Index out of range.", StorageType(aBit) < 256));

			return myData & StorageType(aBit);
		}

		inline bool All() const
		{
			return myData == StorageType(-1);
		}

		inline bool Any() const
		{
			return bool(myData);
		}

		inline bool None() const
		{
			return !bool(myData);
		}

		inline StorageType First() const
		{
			unsigned long result = (unsigned long)-1;

			if (myData)
			{
				_BitScanForward64(&result, myData);
			}

			return 1Ui8 << StorageType(result);
		}

		inline StorageType Last() const
		{
			unsigned long result = (unsigned long)-1;

			if (myData)
			{
				_BitScanReverse64(&result, myData);
			}

			return 1Ui8 << StorageType(result);
		}

		StorageType& Data()
		{
			return myData;
		}

		/* Setters */
		inline void Set(StorageType aBit)
		{
			assert(("Index out of range.", aBit < 256));

			myData |= aBit;
		}
		template <class EnumClass>
		inline void Set(EnumClass aBit)
		{
			assert(("Index out of range.", StorageType(aBit) < 256));

			myData |= StorageType(aBit);
		}

		// Refrain from using this.
		inline void Set(StorageType aBit, bool aValue)
		{
			if (aValue) Set(aBit);
			else		Reset(aBit);
		}

		inline void Reset(StorageType aBit)
		{
			assert(("Index out of range.", aBit < 256));

			myData &= ~aBit;
		}
		template <class EnumClass>
		inline void Reset(EnumClass aBit)
		{
			assert(("Index out of range.", StorageType(aBit) < 256));

			myData &= ~StorageType(aBit);
		}

		inline void Flip(StorageType aBit)
		{
			assert(("Index out of range.", aBit < 256));

			myData ^= aBit;
		}
		template <class EnumClass>
		inline void Flip(EnumClass aBit)
		{
			assert(("Index out of range.", StorageType(aBit) < 256));

			myData ^= StorageType(aBit);
		}

		inline void SetAll()
		{
			myData = StorageType(-1);
		}

		inline void ResetAll()
		{
			myData = 0;
		}

		inline void FlipAll()
		{
			myData = ~myData;
		}

		/* Operators */

		inline BitFlagpole8& operator=(const BitFlagpole8& another)
		{
			myData = another.myData;
			return *this;
		}
		inline BitFlagpole8& operator=(BitFlagpole8&& another) noexcept
		{
			myData = another.myData;
			return *this;
		}

		inline const bool operator[] (StorageType aBit) const
		{
			assert(("Index out of range.", aBit < 256));

			return myData & aBit;
		}
		inline bool operator[] (StorageType aBit)
		{
			assert(("Index out of range.", aBit < 256));

			return myData & aBit;
		}

		template <class EnumClass>
		inline const bool operator[] (EnumClass aBit) const
		{
			assert(("Index out of range.", StorageType(aBit) < 256));

			return myData & StorageType(aBit);
		}
		template <class EnumClass>
		inline bool operator[] (EnumClass aBit)
		{
			assert(("Index out of range.", StorageType(aBit) < 256));

			return myData & StorageType(aBit);
		}

		inline bool operator==(BitFlagpole8 another) const
		{
			return myData == another.myData;
		}
		inline bool operator!=(BitFlagpole8 another) const
		{
			return myData != another.myData;
		}

		inline BitFlagpole8 operator & (BitFlagpole8 another) const
		{
			return BitFlagpole8(myData & another.myData);
		}
		inline BitFlagpole8 operator | (BitFlagpole8 another) const
		{
			return BitFlagpole8(myData | another.myData);
		}
		inline BitFlagpole8 operator ^ (BitFlagpole8 another) const
		{
			return BitFlagpole8(myData ^ another.myData);
		}
		inline BitFlagpole8 operator ~ () const
		{
			return BitFlagpole8(~myData);
		}
		inline BitFlagpole8 operator << (StorageType aShiftAmount) const
		{
			return BitFlagpole8(myData << aShiftAmount);
		}
		inline BitFlagpole8 operator >> (StorageType aShiftAmount) const
		{
			return BitFlagpole8(myData >> aShiftAmount);
		}

		inline BitFlagpole8& operator &= (BitFlagpole8 another)
		{
			myData &= another.myData;

			return *this;
		}
		inline BitFlagpole8& operator |= (BitFlagpole8 another)
		{
			myData |= another.myData;

			return *this;
		}
		inline BitFlagpole8& operator ^= (BitFlagpole8 another)
		{
			myData ^= another.myData;

			return *this;
		}
		inline BitFlagpole8& operator <<= (BitFlagpole8 another)
		{
			myData <<= another.myData;

			return *this;
		}
		inline BitFlagpole8& operator >>= (BitFlagpole8 another)
		{
			myData >>= another.myData;

			return *this;
		}

	private:

		StorageType myData;
	};
	
	class BitFlagpole16
	{
	public:
		using StorageType = uint16_t;
		static constexpr size_t bitCount = sizeof(StorageType) * 8U;
		static constexpr size_t byteCount = sizeof(StorageType);

		/* Constructors & Destructor */
		inline BitFlagpole16()
			: myData(0U)
		{}
		inline BitFlagpole16(StorageType aRawValue)
			: myData(aRawValue)
		{}
		template <class EnumClass>
		inline BitFlagpole16(EnumClass anEnumValue)
			: myData(StorageType(anEnumValue))
		{}
		~BitFlagpole16() = default;

		inline BitFlagpole16(const BitFlagpole16& another)
			: myData(another.myData)
		{}
		inline BitFlagpole16(BitFlagpole16&& another) noexcept
			: myData(another.myData)
		{}

		/* Getters */
		inline bool Test(StorageType aBit) const
		{
			assert(("Index out of range.", aBit < bitCount));

			return myData & aBit;
		}
		template <class EnumClass>
		inline bool Test(EnumClass aBit) const
		{
			assert(("Index out of range.", StorageType(aBit) < bitCount));

			return myData & StorageType(aBit);
		}

		inline bool All() const
		{
			return myData == StorageType(-1);
		}

		inline bool Any() const
		{
			return bool(myData);
		}

		inline bool None() const
		{
			return !bool(myData);
		}

		inline StorageType First() const
		{
			unsigned long result = (unsigned long)-1;

			if (myData)
			{
				_BitScanForward64(&result, myData);
			}

			return 1Ui16 << StorageType(result);
		}

		inline StorageType Last() const
		{
			unsigned long result = (unsigned long)-1;

			if (myData)
			{
				_BitScanReverse64(&result, myData);
			}

			return 1Ui16 << StorageType(result);
		}

		StorageType& Data()
		{
			return myData;
		}

		/* Setters */
		inline void Set(StorageType aBit)
		{
			assert(("Index out of range.", aBit < bitCount));

			myData |= aBit;
		}
		template <class EnumClass>
		inline void Set(EnumClass aBit)
		{
			assert(("Index out of range.", StorageType(aBit) < bitCount));

			myData |= StorageType(aBit);
		}

		// Refrain from using this.
		inline void Set(StorageType aBit, bool aValue)
		{
			if (aValue) Set(aBit);
			else		Reset(aBit);
		}

		inline void Reset(StorageType aBit)
		{
			assert(("Index out of range.", aBit < bitCount));

			myData &= ~aBit;
		}
		template <class EnumClass>
		inline void Reset(EnumClass aBit)
		{
			assert(("Index out of range.", StorageType(aBit) < bitCount));

			myData &= ~StorageType(aBit);
		}

		inline void Flip(StorageType aBit)
		{
			assert(("Index out of range.", aBit < bitCount));

			myData ^= aBit;
		}
		template <class EnumClass>
		inline void Flip(EnumClass aBit)
		{
			assert(("Index out of range.", StorageType(aBit) < bitCount));

			myData ^= StorageType(aBit);
		}

		inline void SetAll()
		{
			myData = StorageType(-1);
		}

		inline void ResetAll()
		{
			myData = 0;
		}

		inline void FlipAll()
		{
			myData = ~myData;
		}

		/* Operators */

		inline BitFlagpole16& operator=(const BitFlagpole16& another)
		{
			myData = another.myData;
		}
		inline BitFlagpole16& operator=(BitFlagpole16&& another) noexcept
		{
			myData = another.myData;
		}

		inline const bool operator[] (StorageType aBit) const
		{
			assert(("Index out of range.", aBit < bitCount));

			return myData & aBit;
		}
		inline bool operator[] (StorageType aBit)
		{
			assert(("Index out of range.", aBit < bitCount));

			return myData & aBit;
		}

		template <class EnumClass>
		inline const bool operator[] (EnumClass aBit) const
		{
			assert(("Index out of range.", StorageType(aBit) < bitCount));

			return myData & StorageType(aBit);
		}
		template <class EnumClass>
		inline bool operator[] (EnumClass aBit)
		{
			assert(("Index out of range.", StorageType(aBit) < bitCount));

			return myData & StorageType(aBit);
		}

		inline bool operator==(BitFlagpole16 another) const
		{
			return myData == another.myData;
		}
		inline bool operator!=(BitFlagpole16 another) const
		{
			return myData != another.myData;
		}

		inline BitFlagpole16 operator & (BitFlagpole16 another) const
		{
			return BitFlagpole16(myData & another.myData);
		}
		inline BitFlagpole16 operator | (BitFlagpole16 another) const
		{
			return BitFlagpole16(myData | another.myData);
		}
		inline BitFlagpole16 operator ^ (BitFlagpole16 another) const
		{
			return BitFlagpole16(myData ^ another.myData);
		}
		inline BitFlagpole16 operator ~ () const
		{
			return BitFlagpole16(~myData);
		}
		inline BitFlagpole16 operator << (StorageType aShiftAmount) const
		{
			return BitFlagpole16(myData << aShiftAmount);
		}
		inline BitFlagpole16 operator >> (StorageType aShiftAmount) const
		{
			return BitFlagpole16(myData >> aShiftAmount);
		}

		inline BitFlagpole16& operator &= (BitFlagpole16 another)
		{
			myData &= another.myData;

			return *this;
		}
		inline BitFlagpole16& operator |= (BitFlagpole16 another)
		{
			myData |= another.myData;

			return *this;
		}
		inline BitFlagpole16& operator ^= (BitFlagpole16 another)
		{
			myData ^= another.myData;

			return *this;
		}
		inline BitFlagpole16& operator <<= (BitFlagpole16 another)
		{
			myData <<= another.myData;

			return *this;
		}
		inline BitFlagpole16& operator >>= (BitFlagpole16 another)
		{
			myData >>= another.myData;

			return *this;
		}

	private:

		StorageType myData;
	};
}