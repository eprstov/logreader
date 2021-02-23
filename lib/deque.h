#ifndef __DEQUE_HEADER__
#define __DEQUE_HEADER__

#include <assert.h>
#include <stddef.h>

template< typename T, size_t BLOCK = 128 >
class Deque
{
public:
	class Iterator;

	Deque();
	~Deque();

	Iterator begin() const;
	Iterator end() const;

	bool empty() const;

	void Push( T&& );
	void Clear();

private:
	struct Block;

	Block *first, *last;
};

template< typename T, size_t BLOCK >
struct Deque< T, BLOCK >::Block : Sequence<T>
{
	using Sequence<T>::from;
	using Sequence<T>::to;

	Block();
	void Push( T&& );
	void Reset();

	bool full() const;

	T items[BLOCK+1];
	T* end;

	Block* next = nullptr;
};

template< typename T, size_t BLOCK >
inline Deque< T, BLOCK >::Block::Block()
{
	to = from = end = items;
}

template< typename T, size_t BLOCK >
inline void Deque< T, BLOCK >::Block::Push( T&& item )
{
	assert( !full() );

	*end = item; // TODO: move
	to = ++end;
}

template< typename T, size_t BLOCK >
inline void Deque< T, BLOCK >::Block::Reset()
{
	next = nullptr;
	to = from = end = items;
}

template< typename T, size_t BLOCK >
inline bool Deque< T, BLOCK >::Block::full() const
{
	return Sequence<T>::length() == BLOCK;
}

template< typename T, size_t BLOCK >
class Deque< T, BLOCK >::Iterator
{
	friend class Deque;

public:
	Iterator& operator++() &;
	Iterator&& operator++() &&;

	Sequence<T>& operator*() { return *block; }
	Sequence<T>* operator->() { return block; }

	friend bool operator==( const Iterator& left, const Iterator& right )
	{
		return left.block == right.block;
	}

	friend bool operator!=( const Iterator& left, const Iterator& right )
	{
		return !operator==( left, right );
	}

private:
	Iterator( Block* );

	void MoveFwd();

	Block* block;
};

template< typename T, size_t BLOCK >
inline Deque< T, BLOCK >::Iterator::Iterator( Block* blk ) : block(blk)
{
}

template< typename T, size_t BLOCK >
inline void Deque< T, BLOCK >::Iterator::MoveFwd()
{
	assert(block);
	block = block->next;
}

template< typename T, size_t BLOCK >
auto Deque< T, BLOCK >::Iterator::operator++() & -> Iterator&
{
	MoveFwd();
	return *this;
}

template< typename T, size_t BLOCK >
auto Deque< T, BLOCK >::Iterator::operator++() && -> Iterator&&
{
	MoveFwd();
	return static_cast< Iterator&& >( *this );
}

template< typename T, size_t BLOCK >
inline Deque< T, BLOCK >::Deque()
{
	last = first = new Block();
}

template< typename T, size_t BLOCK >
inline Deque< T, BLOCK >::~Deque()
{
	Clear();
	delete first;
}

template< typename T, size_t BLOCK >
inline auto Deque< T, BLOCK >::begin() const -> Iterator
{
	return Iterator(first);
}

template< typename T, size_t BLOCK >
inline auto Deque< T, BLOCK >::end() const -> Iterator
{
	return Iterator(nullptr);
}

template< typename T, size_t BLOCK >
inline bool Deque< T, BLOCK >::empty() const
{
	return first->empty();
}

template< typename T, size_t BLOCK >
inline void Deque< T, BLOCK >::Push( T&& item )
{
	last->Push( static_cast< T&& >(item) );
	if( last->full() )
	{
		last = last->next = new Block();
	}
}

template< typename T, size_t BLOCK >
inline void Deque< T, BLOCK >::Clear()
{
	for( auto block = first->next; block; )
	{
		auto next = block->next;
		delete block;

		block = next;
	}

	first->Reset();
	last = first;
}

#endif // !__DEQUE_HEADER__
