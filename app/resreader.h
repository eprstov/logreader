#ifndef __RESREADER_HEADER__
#define __RESREADER_HEADER__

#include <lib/basic.h>

#include <stdio.h>

class ResultReader
{
public:
	using Line = Sequence<char>;

	ResultReader() {}
	ResultReader( const char* rpath, const char* ipath );
	~ResultReader();

	ResultReader& operator=( ResultReader&& );
	ResultReader& operator=( const ResultReader& ) = delete;

	Line operator[](size_t);
	size_t total() const;

	bool ready() const;

private:
	void Cleanup();

	FILE *file = nullptr;

	Buffer<char> buffer;

	struct Index
	{
		Index();
		Index( const char* path );
		~Index();

		Index& operator=( Index&& );
		Index& operator=( const Index& ) = delete;

		bool ready() const { return file; }

		size_t operator[]( size_t i );

		void Load();
		void Cleanup();

		static constexpr size_t WINDOW = 1024;
		static constexpr size_t MARGIN = 0;

		size_t from = 0, to = 1;
		size_t total = 1;

		size_t buffer[WINDOW];

		FILE *file = nullptr;
	};

	Index index;
};


inline bool ResultReader::ready() const
{
	return file && index.ready();
}

#endif // !__RESREADER_HEADER__
