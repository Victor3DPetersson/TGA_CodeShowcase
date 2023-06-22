#pragma once
#include <vector>

namespace CU
{
	namespace Details
	{
		template <class T>
		void Swap(T& aFirstElement, T& aSecondElement)
		{
			T temp = aFirstElement;
			aFirstElement = aSecondElement;
			aSecondElement = temp;
		};

		template <class T>
		int Partition(std::vector<T>& aVector, int aLow, int aHigh)
		{
			T pivot = aVector[aHigh];
			int i = (aLow - 1);

			for (int j = aLow; j <= aHigh - 1; j++)
			{
				if (aVector[j] <= pivot)
				{
					i++;
					Details::Swap(aVector[i], aVector[j]);
				}
			}
			Details::Swap(aVector[i + 1], aVector[aHigh]);
			return (i + 1);
		};

		template <class T>
		void QuickSorting(std::vector<T>& aVector, int aLow, int aHigh)
		{
			if (aLow < aHigh)
			{
				int partitioningIndex = Details::Partition(aVector, aLow, aHigh);

				Details::QuickSorting(aVector, aLow, partitioningIndex - 1);
				Details::QuickSorting(aVector, partitioningIndex + 1, aHigh);
			}
		};

		template <class T>
		void Merge(std::vector<T>& aVector, int aLeft, int aMiddle, int aRight)
		{
			int vectorLeftSize = aMiddle - aLeft + 1;
			int vectorRightSize = aRight - aMiddle;

			std::vector<T> tempLeftVector, tempRightVector;

			for (int i = 0; i < vectorLeftSize; i++)
			{
				tempLeftVector.push_back(aVector[aLeft + i]);
			}
			for (int j = 0; j < vectorRightSize; j++)
			{
				tempRightVector.push_back(aVector[aMiddle + j + 1]);
			}

			int leftIndex = 0;
			int rightIndex = 0;
			int middleIndex = aLeft;

			while (leftIndex < vectorLeftSize && rightIndex < vectorRightSize)
			{
				if (tempLeftVector[leftIndex] <= tempRightVector[rightIndex])
				{
					aVector[middleIndex] = tempLeftVector[leftIndex];
					leftIndex++;
				}
				else
				{
					aVector[middleIndex] = tempRightVector[rightIndex];
					rightIndex++;
				}
				middleIndex++;
			}

			while (leftIndex < vectorLeftSize)
			{
				aVector[middleIndex] = tempLeftVector[leftIndex];
				leftIndex++;
				middleIndex++;
			}

			while (rightIndex < vectorRightSize)
			{
				aVector[middleIndex] = tempRightVector[rightIndex];
				rightIndex++;
				middleIndex++;
			}
		}

		template <class T>
		void MergeSorting(std::vector<T>& aVector, int aLeft, int aRight)
		{
			if (aLeft >= aRight)
				return;

			int aMiddle = aLeft + (aRight - aLeft) / 2;
			Details::MergeSorting(aVector, aLeft, aMiddle);
			Details::MergeSorting(aVector, aMiddle + 1, aRight);
			Details::Merge(aVector, aLeft, aMiddle, aRight);
		}
	} // namespace Details

	template <class T>
	void SelectionSort(std::vector<T>& aVector)
	{
		int minIndex;

		for (int i = 0; i < aVector.size() - 1; i++)
		{
			minIndex = i;

			for (int j = i + 1; j < aVector.size(); j++)
			{
				if (aVector[j] < aVector[minIndex])
				{
					minIndex = j;
				}
			}
			Details::Swap(aVector[minIndex], aVector[i]);
		}
	};
	template <class T>
	void BubbleSort(std::vector<T>& aVector)
	{
		for (int i = 0; i < aVector.size() - 1; i++)
		{
			for (int j = 0; j < aVector.size() - i - 1; j++)
			{
				if (aVector[j] > aVector[j + 1])
				{
					Details::Swap(aVector[j], aVector[j + 1]);
				}
			}
		}
	};
	template <class T>
	void QuickSort(std::vector<T>& aVector)
	{
		Details::QuickSorting(aVector, 0, aVector.size() - 1);
	};
	template <class T>
	void MergeSort(std::vector<T>& aVector)
	{
		Details::MergeSorting(aVector, 0, aVector.size() - 1);
	};

} // namespace CommonUtilities#pragma once
