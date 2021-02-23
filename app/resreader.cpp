#include "resreader.h"

ResultReader::ResultReader( const char* rpath, const char* ipath ) : index(ipath)
{
	file = fopen( rpath, "r" );
}

ResultReader::~ResultReader()
{
	Cleanup();
}

ResultReader& ResultReader::operator=( ResultReader&& other )
{
	Cleanup();

	file = other.file;
	other.file = nullptr;

	index = static_cast< Index&& >( other.index );

	return *this;
}

auto ResultReader::operator[]( size_t i ) -> Line
{
	assert( file && 0 <= i && i < index.total - 1 );

	long offset = index[i];
	long size = index[i+1] - offset;
	assert( size > 0 );

	if( fseek( file, offset, SEEK_SET ) == 0 )
	{
		buffer.Resize(size);
		if( fread( &buffer.front(), 1, size, file ) == size )
		{
			buffer[ --size ] = '\0';
			return buffer.data();
		}
	}

	return Line();
}

size_t ResultReader::total() const
{
	assert( index.total > 0 );
	return index.total - 1;
}

void ResultReader::Cleanup()
{
	if(file)
	{
		fclose(file);
	}
}

ResultReader::Index::Index()
{
	memset( buffer, 0, WINDOW * sizeof(size_t) );
}

ResultReader::Index::Index( const char* path )
{
	if( (file = fopen( path, "r" )) )
	{
		fseek( file, 0, SEEK_END );
		total = ftell(file) / sizeof(size_t);
		assert( total > 0 );

		to = total > WINDOW ? WINDOW : total;
		Load();
	}
}

ResultReader::Index::~Index()
{
	Cleanup();
}

auto ResultReader::Index::operator=( Index&& other ) -> Index&
{
	Cleanup();

	file = other.file;
	other.file = nullptr;

	total = other.total;

	from = other.from;
	to = other.to;

	memcpy( buffer, other.buffer, sizeof(size_t) * WINDOW );

	other.total = other.to = other.from = 0;

	return *this;
}

size_t ResultReader::Index::operator[]( size_t i )
{
	if( (i < from + MARGIN && from > 0) || (i + MARGIN >= to && to < total) )
	{
		from = i > WINDOW / 2 ? i - WINDOW / 2 : 0;
		to = from + WINDOW < total ? from + WINDOW : total;

		Load();
	}

	assert( from <= i && i < to );
	return buffer[ i - from ];
}


void ResultReader::Index::Load()
{
	if( fseek( file, from * sizeof(size_t), SEEK_SET ) == 0 )
	{
		[[maybe_unused]] auto res = fread( &buffer, sizeof(size_t), to - from, file );
		assert( res == to - from );
	}
}

void ResultReader::Index::Cleanup()
{
	if(file)
	{
		fclose(file);
	}

	total = 0;
}
