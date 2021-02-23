#include "processor.h"

#include <lib/basic.h>
#include <lib/logreader.h>

#include <stdio.h>

const char RESULT[] = "results.log";
const char INDEX[] = "results.idx";

Processor::Processor()
{
}

Processor::Processor( const char* input, const char* filter, const char* outdir ) : writer( new Writer(outdir) )
{
	finput = fopen( input, "r" );

	reader = new CLogReader(writer);
	if( !reader->SetFilter(filter) )
	{
		delete reader;
	}
}

Processor::~Processor()
{
	Cleanup();
}

Processor& Processor::operator=( Processor&& other )
{
	Cleanup();

	writer = other.writer;
	other.writer = nullptr;

	reader = other.reader;
	other.reader = nullptr;

	finput = other.finput;
	other.finput = nullptr;

	return *this;
}

time_t Processor::Process()
{
	assert( ready() );

	time_t started, finished;
	time( &started );

	const size_t BUF = 10*1024*1024;
	char* buf = new char[BUF+1];

	while( !feof(finput) )
	{
		auto sz = fread( buf, 1, BUF, finput );
		if( sz > 0 )
		{
			if( feof(finput) && buf[ sz-1 ] != '\n' )
			{
				buf[sz++] = '\n';
			}

			if( !reader->AddSourceBlock( buf, sz ) )
			{
				break;
			}
		}
	}

	writer->Flush();

	delete[] buf;

	time( &finished );
	return finished - started;
}

void Processor::Cleanup()
{
	if(finput)
	{
		fclose(finput);
	}

	delete reader;
	delete writer;
}

Processor::Writer::Writer( const char* dir )
{
	assert(dir);

	constexpr size_t szname = (sizeof(RESULT) > sizeof(INDEX) ? sizeof(RESULT) : sizeof(INDEX)) - 1;
	auto sz = strlen(dir) + sizeof( '/' ) + szname;

	auto path = new char[ sz+1 ];
	[[maybe_unused]] auto res = snprintf( path, sz+1, "%s/%s", dir, RESULT );
	assert( res == sz );

	remove(path);
	if( (fresult = fopen( path, "w" )) )
	{
		[[maybe_unused]] auto res = snprintf( path, sz+1, "%s/%s", dir, INDEX );
		assert( res == sz );

		remove(path);
		if( (findex = fopen( path, "wb" )) )
		{
			StoreOffset();
		}
	}

	delete[] path;
}

Processor::Writer::~Writer()
{
	if(findex)
	{
		fclose(findex);
	}

	if(fresult)
	{
		fclose(fresult);
	}
}

void Processor::Writer::Handle( const Sequence<Line>& lines )
{
	for( const auto& line : lines )
	{
		fwrite( line.from, 1, line.length(), fresult );
		fputc( '\n', fresult );

		offset += line.length() + 1;
		StoreOffset();
	}
}

void Processor::Writer::StoreOffset()
{
	[[maybe_unused]] auto res = fwrite( &offset, sizeof(offset), 1, findex );
	assert( res == 1 );
}

void Processor::Writer::Flush()
{
	if(fresult)
	{
		fflush(fresult);
	}

	if(findex)
	{
		fflush(findex);
	}
}
