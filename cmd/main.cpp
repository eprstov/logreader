#include "logreader.h"

#include <stdio.h>

int main( int argc, const char * argv[] )
{
	if( argc != 3 )
	{
		printf( "usage: logreader <filter> <path>\n" );
		return 1;
	}

	const char* filter = argv[1];
	const char* path = argv[2];

	CLogReader reader;
	if( !reader.SetFilter(filter) )
	{
		printf( "invalid filter: %s", filter );
		return 1;
	}

	if( FILE* file = fopen( path, "r" ) )
	{
		const size_t BUF = 10*1024*1024;
		char* buf = new char[BUF+1];

		while( !feof(file) )
		{
			auto sz = fread( buf, 1, BUF, file );
			if( sz > 0 )
			{
				if( feof(file) && buf[ sz-1 ] != '\n' )
				{
					buf[sz++] = '\n';
				}

				if( !reader.AddSourceBlock( buf, sz ) )
				{
					break;
				}
			}
		}

		delete[] buf;

		fclose(file);
	}
	else
	{
		printf( "cannot open the file %s\n", path );
		return 1;
	}

	return 0;
}
