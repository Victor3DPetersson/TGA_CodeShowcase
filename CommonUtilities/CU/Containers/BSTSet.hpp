#pragma once
#include "BSTSetNode.hpp"
#include <assert.h>
#include <math.h>

namespace CU
{
	template <class T>
	class BSTSet
	{
	public:
		BSTSet() { myHeadNode = nullptr; };
		~BSTSet() {};
		// Returnerar true om elementet finns i mängden, och false annars.
		bool HasElement(const T& aValue);
		// Stoppar in elementet i mängden, om det inte redan finns där. Gör
		// ingenting annars.
		void Insert(const T& aValue);
		// Plockar bort elementet ur mängden, om det finns. Gör ingenting annars.
		void Remove(const T& aValue);
		void DSWBalance();
	private:
		//Functions
		void RemoveParent(const T& aValue, SetNode<T>* aCurrentNode, SetNode<T>* aPreviousNode = nullptr);

		SetNode<T>* FindSmallestRight(SetNode<T>* aCurrentNode);
		SetNode<T>* FindLargestLeft(SetNode<T>* aCurrentNode);
		SetNode<T>* TraverseToNode(const T& aValue, SetNode<T>* aCurrentNode);
		SetNode<T>* TraverseToNode(const T& aValue, SetNode<T>* aCurrentNode, bool& aHasFoundValue);
		SetNode<T>* TraverseLeftDown(SetNode<T>* someNodeAbove);
		SetNode<T>* TraverseRightDown(SetNode<T>* someNodeAbove);
		void CreateBackbone(int& aNumberOfNodes);
		void CreatePerfectTree(const int aAmountOfNodes);
		static bool GRotateLeft(SetNode<T>* aNode);
		
		SetNode<T>* myHeadNode;
	};
	template<class T>
	inline bool BSTSet<T>::HasElement(const T& aValue)
	{
		if (myHeadNode == nullptr)
		{
			return false;
		}

		SetNode<T>* iteratorNode = myHeadNode;
		bool hasFoundValue = true;
		iteratorNode = TraverseToNode(aValue, iteratorNode, hasFoundValue);

		if (hasFoundValue)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	template<class T>
	inline void BSTSet<T>::Insert(const T& aValue)
	{
		if (myHeadNode == nullptr)
		{
			myHeadNode = new SetNode<T>(aValue);
			return;
		}
		SetNode<T>* currentNode = myHeadNode;
		currentNode = TraverseToNode(aValue, currentNode);
		if (currentNode == nullptr)
		{
			printf("Value is already in BST");
		}
		else
		{
			if (aValue < currentNode->GetValue())
			{
				currentNode->SetLeft(new SetNode<T>(aValue));
			}
			else
			{
				currentNode->SetRight(new SetNode<T>(aValue));
			}
		}
	}
	template<class T>
	inline void BSTSet<T>::Remove(const T& aValue)
	{
		SetNode<T>* iteratorNode = myHeadNode;
		bool hasFoundValue = false;
		iteratorNode = TraverseToNode(aValue, iteratorNode, hasFoundValue);
		if (hasFoundValue)
		{
			if (iteratorNode->GetValue() == myHeadNode->GetValue())
			{
				if (myHeadNode->GetLeft() == nullptr && myHeadNode->GetRight() == nullptr)
				{
					delete myHeadNode;
					myHeadNode = nullptr;
				}
				else if (myHeadNode->GetLeft() == nullptr)
				{
					SetNode<T>* iteratorNodeToRemove = myHeadNode->GetRight();
					iteratorNodeToRemove = FindSmallestRight(iteratorNodeToRemove);
					RemoveParent(iteratorNodeToRemove->GetValue(), myHeadNode);
					myHeadNode->GetValue() = iteratorNodeToRemove->GetValue();
					delete iteratorNodeToRemove;
					iteratorNodeToRemove = nullptr;
				}
				else
				{
					SetNode<T>* iteratorNodeToRemove = myHeadNode->GetLeft();
					iteratorNodeToRemove = FindLargestLeft(iteratorNodeToRemove);
					RemoveParent(iteratorNodeToRemove->GetValue(), myHeadNode);
					myHeadNode->GetValue() = iteratorNodeToRemove->GetValue();
					delete iteratorNodeToRemove;
					iteratorNodeToRemove = nullptr;
				}
			}
			else
			{
				if (iteratorNode->GetLeft() == nullptr && iteratorNode->GetRight() == nullptr)
				{
					RemoveParent(iteratorNode->GetValue(), myHeadNode);
					delete iteratorNode;
					iteratorNode = nullptr;
				}
				else if (iteratorNode->GetLeft() == nullptr)
				{
					SetNode<T>* iteratorNodeToRemove = iteratorNode->GetRight();
					iteratorNodeToRemove = FindSmallestRight(iteratorNodeToRemove);
					RemoveParent(iteratorNodeToRemove->GetValue(), myHeadNode);
					iteratorNode->GetValue() = iteratorNodeToRemove->GetValue();
					delete iteratorNodeToRemove;
					iteratorNodeToRemove = nullptr;
				}
				else
				{
					SetNode<T>* iteratorNodeToRemove = iteratorNode->GetLeft();
					iteratorNodeToRemove = FindLargestLeft(iteratorNodeToRemove);
					RemoveParent(iteratorNodeToRemove->GetValue(), myHeadNode);
					iteratorNode->GetValue() = iteratorNodeToRemove->GetValue();
					delete iteratorNodeToRemove;
					iteratorNodeToRemove = nullptr;
				}
			}
		}
		else
		{
			printf("Value does not exist in BST");
		}
	}
	template<class T>
	inline void BSTSet<T>::DSWBalance()
	{
		int numberOfNodes = 0;
		CreateBackbone(numberOfNodes);
		CreatePerfectTree(numberOfNodes);
	}
	template<class T>
	inline void BSTSet<T>::RemoveParent(const T& aValue, SetNode<T>* aCurrentNode, SetNode<T>* aPreviousNode)
	{
		//If we find aValue on our current Node
		if (!(aValue < aCurrentNode->GetValue()) && !(aCurrentNode->GetValue() < aValue))
		{
			//We check if our value is smaller or larger than our previous node, if smaller we are Left else Right
			if (aPreviousNode->GetValue() < aValue)
			{
				if (aCurrentNode->GetLeft() != nullptr)
				{
					aPreviousNode->SetRight(aCurrentNode->GetLeft());
				}
				else
				{
					aPreviousNode->SetRight(aCurrentNode->GetRight());
				}
			}
			else
			{
				if (aCurrentNode->GetRight() != nullptr)
				{
					aPreviousNode->SetLeft(aCurrentNode->GetRight());
				}
				else
				{
					aPreviousNode->SetLeft(aCurrentNode->GetLeft());
				}
			}
			return;
		}
		SetNode<T>* nodeToInsert = nullptr;

		if (aValue < aCurrentNode->GetValue())
		{
			nodeToInsert = TraverseLeftDown(aCurrentNode);
			aPreviousNode = aCurrentNode;
			aCurrentNode = nodeToInsert;
			RemoveParent(aValue, aCurrentNode, aPreviousNode);

		}
		else if (aValue > aCurrentNode->GetValue())
		{
			nodeToInsert = TraverseRightDown(aCurrentNode);
			aPreviousNode = aCurrentNode;
			aCurrentNode = nodeToInsert;
			RemoveParent(aValue, aCurrentNode, aPreviousNode);
		}
	}
	template<class T>
	inline SetNode<T>* BSTSet<T>::FindSmallestRight(SetNode<T>* aCurrentNode)
	{
		if (aCurrentNode->GetLeft() == nullptr)
		{
			return aCurrentNode;
		}
		else
		{
			aCurrentNode = aCurrentNode->GetLeft();
			FindSmallestRight(aCurrentNode);
		}
	}
	template<class T>
	inline SetNode<T>* BSTSet<T>::FindLargestLeft(SetNode<T>* aCurrentNode)
	{
		if (aCurrentNode->GetRight() == nullptr)
		{
			return aCurrentNode;
		}
		else
		{
			aCurrentNode = aCurrentNode->GetRight();
			FindLargestLeft(aCurrentNode);
		}
	}
	template<class T>
	inline SetNode<T>* BSTSet<T>::TraverseToNode(const T& aValue, SetNode<T>* aIteratorNode)
	{
		if (!(aValue < aIteratorNode->GetValue()) && !(aIteratorNode->GetValue() < aValue))
		{
			return nullptr;
		}
		SetNode<T>* nodeToInsert = nullptr;
		if (aValue < aIteratorNode->GetValue())
		{
			nodeToInsert = TraverseLeftDown(aIteratorNode);
			if (nodeToInsert == nullptr)
			{
				return aIteratorNode;
			}
			else
			{
				aIteratorNode = nodeToInsert;
				TraverseToNode(aValue, aIteratorNode);
			}
		}
		else if (aValue > aIteratorNode->GetValue())
		{
			nodeToInsert = TraverseRightDown(aIteratorNode);
			if (nodeToInsert == nullptr)
			{
				return aIteratorNode;
			}
			else
			{
				aIteratorNode = nodeToInsert;
				TraverseToNode(aValue, aIteratorNode);
			}
		}
	}
	template<class T>
	inline SetNode<T>* BSTSet<T>::TraverseToNode(const T& aValue, SetNode<T>* aCurrentNode, bool& aHasFoundValue)
	{
		if (!(aValue < aCurrentNode->GetValue()) && !(aCurrentNode->GetValue() < aValue))
		{
			aHasFoundValue = true;
			return aCurrentNode;
		}

		SetNode<T>* nodeToInsert = nullptr;

		if (aValue < aCurrentNode->GetValue())
		{
			nodeToInsert = TraverseLeftDown(aCurrentNode);
			if (nodeToInsert == nullptr)
			{
				aHasFoundValue = false;
				return aCurrentNode;
			}
			else
			{
				aCurrentNode = nodeToInsert;
				TraverseToNode(aValue, aCurrentNode, aHasFoundValue);
			}
		}
		else if (aValue > aCurrentNode->GetValue())
		{
			nodeToInsert = TraverseRightDown(aCurrentNode);
			if (nodeToInsert == nullptr)
			{
				aHasFoundValue = false;
				return aCurrentNode;
			}
			else
			{
				aCurrentNode = nodeToInsert;
				TraverseToNode(aValue, aCurrentNode, aHasFoundValue);
			}
		}
	}
	template<class T>
	inline SetNode<T>* BSTSet<T>::TraverseLeftDown(SetNode<T>* someNodeAbove)
	{
		SetNode<T>* tempNode = someNodeAbove->GetLeft();
		if (tempNode == nullptr)
		{
			return nullptr;
		}
		else
		{
			return tempNode;
		}
	}
	template<class T>
	inline SetNode<T>* BSTSet<T>::TraverseRightDown(SetNode<T>* someNodeAbove)
	{
		SetNode<T>* tempNode = someNodeAbove->GetRight();
		if (tempNode == nullptr)
		{
			return nullptr;
		}
		else
		{
			return tempNode;
		}
	}
	template<class T>
	inline void BSTSet<T>::CreateBackbone(int& aNumberOfNodes)
	{
		SetNode<T>* currentNode = myHeadNode;
		SetNode<T>* parentNode = nullptr;
		while (currentNode != nullptr)
		{
			if (currentNode->GetLeft() != nullptr)
			{
				SetNode<T>* currentChild = currentNode->GetLeft();
				currentNode->SetLeft(currentChild->GetRight());
				currentChild->SetRight(currentNode);
				if (parentNode != nullptr)
				{
					parentNode->SetRight(currentChild);
				}
				currentNode = currentChild;
			}
			else
			{
				if (aNumberOfNodes == 0)
				{
					myHeadNode = currentNode;
				}
				parentNode = currentNode;
				currentNode = currentNode->GetRight();
				aNumberOfNodes++;
			}
		}
	}
	template<class T>
	inline void BSTSet<T>::CreatePerfectTree(const int aAmountOfNodes)
	{
		int leaves = ceil(log2(aAmountOfNodes)) - aAmountOfNodes;
		SetNode<T>* currentNode1 = myHeadNode;
		for (int i = 0; i < leaves; i++)
		{
			if (i == 0)
			{
				GRotateLeft(myHeadNode);
				currentNode1 = myHeadNode;
			}
			else
			{
				GRotateLeft(currentNode1->GetRight());
				currentNode1 = currentNode1->GetRight();
			}
		}
		int times = aAmountOfNodes;
		while (times > 1)
		{
			times *= 0.5f;

			GRotateLeft(myHeadNode);
			SetNode<T>* currentNode2 = myHeadNode;
			for (int i = 0; i < times - 1; i++)
			{
				GRotateLeft(currentNode2->GetRight());
				currentNode2 = currentNode2->GetRight();
			}
		}
	}
	template<class T>
	inline bool BSTSet<T>::GRotateLeft(SetNode<T>* aNode)
	{
		if (aNode == nullptr)
		{
			return false;
		}
		if (aNode->GetRight() == nullptr)
		{
			return false;
		}
		const T tempValue = aNode->GetValue();
		aNode->GetValue() = aNode->GetRight()->GetValue();
		aNode->GetRight()->GetValue() = tempValue;
		SetNode<T>* originalRight = aNode->GetRight();
		aNode->SetRight(originalRight->GetRight());
		originalRight->SetRight(originalRight->GetLeft());
		originalRight->SetLeft(aNode->GetLeft());
		aNode->SetLeft(originalRight);
		return true;
	}
}