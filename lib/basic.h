#ifndef __BASIC_HEADER__
#define __BASIC_HEADER__

#include <assert.h>
#include <stddef.h>
#include <string.h>

template< typename T = char >
struct Sequence
{
	size_t length() const { return to - from; }
	bool empty() const { return to == from; }

	bool valid() const { return to >= from; }

	const T* begin() const { return from; }
	const T* end() const { return to; }

	const T& operator[]( size_t i ) const { return from[i]; }

	const T *from, *to;
};

template< typename T = char >
struct Buffer
{
	static constexpr size_t DEFAULT = 128;

public:
	Buffer();
	Buffer( Buffer&& );
	Buffer( const Buffer& ) = delete;

	~Buffer();

	Buffer& operator=( Buffer&& );
	Buffer& operator=( const Buffer& ) = delete;

	void Append( const Sequence<T>& );
	void Append( const T& );

	void Reserve(size_t);
	void Resize(size_t);

	void Clear();

	bool empty() const { return sz == 0; }
	size_t size() const { return sz; }
	Sequence<T> data() const;

	T& front() { return *dt; }

	T& operator[]( size_t i );

private:
	void Init();
	void Cleanup();

	T* dt = nullptr;
	size_t sz = 0, total = DEFAULT;
};

template< typename T >
inline Buffer<T>::Buffer()
{
	dt = new T[DEFAULT];
}

template< typename T >
inline Buffer<T>::Buffer( Buffer&& other )
{
	dt = other.dt;
	sz = other.sz;
	total = other.total;

	other.Init();
}

template< typename T >
inline Buffer<T>::~Buffer()
{
	Cleanup();
}

template< typename T >
inline Buffer<T>& Buffer<T>::operator=( Buffer&& other )
{
	Cleanup();

	dt = other.dt;
	sz = other.sz;
	total = other.total;

	other.Init();

	return *this;
}

template< typename T >
inline void Buffer<T>::Append( const T& item )
{
	Reserve( sz+1 );

	dt[sz++] = item;
}

template< typename T >
inline void Buffer<T>::Append( const Sequence<T>& seq )
{
	Reserve( sz + seq.length() );

	memcpy( dt+sz, seq.from, seq.length() );
	sz += seq.length();
}

template< typename T >
inline void Buffer<T>::Resize( size_t size )
{
	Reserve( sz = size );
}

template< typename T >
inline void Buffer<T>::Reserve( size_t required )
{
	if( total < required )
	{
		total = required * 2;

		auto newdt = new T[total];
		memcpy( newdt, dt, sz * sizeof(T) );

		delete[] dt;
		dt = newdt;
	}
}

template< typename T >
inline void Buffer<T>::Clear()
{
	sz = 0;
}

template< typename T >
inline Sequence<T> Buffer<T>::data() const
{
	return { dt, dt + sz };
}

template< typename T >
inline T& Buffer<T>::operator[]( size_t i )
{
	assert( i < sz );
	return dt[i];
}

template< typename T >
inline void Buffer<T>::Init()
{
	dt = new T[DEFAULT];
	sz = 0;
	total = DEFAULT;
}

template< typename T >
inline void Buffer<T>::Cleanup()
{
	delete[] dt;
}

#endif // __BASIC_HEADER__
