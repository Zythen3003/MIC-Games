#ifndef GENERIC_STACK_H
#define GENERIC_STACK_H

// Define a simple size_t type for AVR if not available
typedef unsigned int size_t;

template <typename T, size_t StackSize>
class GenericStack {
private:
    T stack[StackSize];  // Array to store stack elements
    int top;             // Index of the top element (-1 when empty)

public:
    // Constructor: initialize the stack
    GenericStack() : top(-1) {}

    // Push a value onto the stack
    bool push(const T &value) {
        if (top >= static_cast<int>(StackSize) - 1) {
            return false; // Stack Overflow
        }
        stack[++top] = value;
        return true;
    }

    // Pop a value from the stack
    bool pop(T &value) {
        if (top < 0) {
            return false; // Stack Underflow
        }
        value = stack[top--];
        return true;
    }

    // Peek the top value without popping
    bool peek(T &value) const {
        if (top < 0) {
            return false; // Stack is empty
        }
        value = stack[top];
        return true;
    }

    // Check if the stack is empty
    bool isEmpty() const {
        return top < 0;
    }

    // Check if the stack is full
    bool isFull() const {
        return top >= static_cast<int>(StackSize) - 1;
    }

    // Get the number of elements in the stack
    size_t size() const {
        return static_cast<size_t>(top + 1);
    }

    // Clear the stack
    void clear() {
        top = -1;
    }
};

#endif // GENERIC_STACK_H
