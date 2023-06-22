#pragma once

namespace CU
{
    template <class T>
    class SetNode
    {
    public:
        SetNode<T>(const SetNode<T>&) = delete;
        SetNode<T>& operator=(const SetNode<T>&) = delete;

        SetNode<T>* GetRight() const
        {
            return myRight;
        };
        SetNode<T>* GetLeft() const { return myLeft; };
        const T& GetValue() const { return myValue; };
        T& GetValue() { return myValue; };

    private:
        template <class T>
        friend class BSTSet;
        SetNode()
        {
            myLeft = nullptr;
            myRight = nullptr;
        };
        SetNode(const T& aValue)
        {
            myValue = aValue;
            myLeft = nullptr;
            myRight = nullptr;
        };
        ~SetNode()
        {
            myLeft = nullptr;
            myRight = nullptr;
        };

        void SetLeft(SetNode<T>* aNode)
        {
            myLeft = aNode;
        };

        void SetRight(SetNode<T>* aNode)
        {
            myRight = aNode;
        };

        SetNode* myLeft;
        SetNode* myRight;

        T myValue;
    };
}