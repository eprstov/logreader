#include "logreader.h"

CLogReader::Printer CLogReader::Printer::dflt;

inline bool Match( char ch, char pt )
{
	return ch == pt || pt == '?';
}

CLogReader::CLogReader( Handler* hdlr ) : handler(hdlr)
{
}

CLogReader::CLogReader() : CLogReader( &Printer::dflt )
{
}

CLogReader::~CLogReader()
{
	delete[] filter;
}

// finds a line break in the text around the given position
const char* SeekLn( const Sequence<char>& text, const char* p )
{
	assert( text.from <= p && p < text.to );
	for( auto pb = p; pb != text.from; --pb )
	{
		if( *pb == '\n' )
		{
			return pb;
		}
	}

	for( auto pf = p; pf != text.to; ++pf )
	{
		if( *pf == '\n' )
		{
			return pf;
		}
	}

	return text.to; // not found
}

size_t CLogReader::Dispatch( const Text& txt )
{
	auto text = txt;
	const auto total = text.length();

	auto n = total <= BLOCK ? 1 : total / BLOCK;
	n = n <= WORKERS ? n : WORKERS;

	assert( n > 0 );
	const auto block = total / n;

	for( int i = 0; i < n - 1; ++i )
	{
		const auto p = SeekLn( text, text.from + block );
		if( p == text.to )
		{
			assert( tail.empty() );
			tail.Append( {text.from, p} );

			return i;
		}

		workers[i].Start( {text.from, p+1}, filter );
		text.from = p+1;

		if( text.empty() )
		{
			return i;
		}
	}

	workers[ n-1 ].Start( text, filter );

	return n;
}

bool CLogReader::SetFilter( const char* fltr )
{
	delete filter;
	filter = nullptr;

	// check for invalid characters
	for( auto p = fltr; *p; ++p )
	{
		if( *p == '\n' )
		{
			return false;
		}
	}

	const char* pr = fltr;
	char* pw = filter = new char[ strlen(fltr)+1 ];

	while(true)
	{
		// copy all non-wildcard characters as is
		for( ; *pr != '*' && (*pw = *pr); ++pr, ++pw );

		if( !*pr ) break;

		// go through the wildcards calculating the number of '?'
		int q = 0;
		for( ; *pr == '*' || *pr == '?'; ++pr )
		{
			if( *pr == '?' )
			{
				++q;
			}
		}

		// put all '?' before the '*'
		while( q-- ) *pw++ = '?';
		*pw++ = '*';
	}

	return true;
}

bool CLogReader::AddSourceBlock( const char* block, const size_t block_size )
{
	assert( block && block_size );
	if( !filter || !block ||  !block_size )
	{
		return false;
	}

	Text text{ block, block + block_size };

	// look for the rest of the unprocessed piece left from the previour run
	if( !tail.empty() )
	{
		auto ln = SeekLn( text, text.from );

		if( ln == text.to )
		{
			tail.Append( {text.from, ln} );
			return true;
		}

		tail.Append( {text.from, ln+1} );
		text.from = ln+1;
	}

	Buffer<char> extra( static_cast< Buffer<char>&& >(tail) ); // clear tail before dispatching
	assert( tail.empty() );

	auto n = Dispatch(text);
	assert( n > 0 );

	// synchronously process the extra piece
	if( !extra.empty() )
	{
		const auto& seq = extra.data();

		Results results;
		[[maybe_unused]] auto p = Process( seq, filter, results );

		assert( p == seq.to );
		if( !results.empty() )
		{
			auto&& seq = *results.begin();
			assert( seq.length() == 1 );

			handler->Handle(seq);
		}
	}

	// wait for the workers to finish and print the results in proper order
	for( int i = 0; i < n; ++i )
	{
		auto& worker = workers[i];
		worker.Wait();

		assert( worker.rest == worker.text.to || i == n-1 );

		auto& results = worker.results;
		for( const auto& seq : results )
		{
			handler->Handle(seq);
		}

		results.Clear();
	}

	// store the unprocessed piece of the last forker
	auto& last = workers[ n-1 ];
	if( last.rest != text.to )
	{
		tail.Append( {last.rest, text.to} );
	}

	return true;
}

const char* CLogReader::Process( const Text& seq, const char* filter, Results& results )
{
	const char* ps = seq.from; // sliding pointer to the text
	const char* const end = seq.to;

	while( ps != end )
	{
		const char* ppx = nullptr; // pointer to the current movable (followind a '*') piece of the filter
		const char* pp = filter; // sliding pointer to the filter

		const char* const ln = ps; // points to the beggining of the line

		while(true) // goes through a single line
		{
			if( *pp == '*' )
			{
				ppx = ++pp;
			}

			if( !*pp || ps == end || *ps == '\n' )
			{
				break;
			}

			if(ppx)
			{
				// look for the first character of the piece
				for( ; ps != end && *ps != '\n' && *ps != *pp; ++ps );

				if( ps == end || *ps == '\n' )
				{
					break;
				}

				++ps;
				++pp;
			}

			const auto psx = ps; // store current position


			for( ; ps != end && *ps != '\n' && *pp != '*' && Match( *ps, *pp ); ++ps, ++pp ); // match the rest of the pattern piece
			if( *pp != '*' && *ps != '\n' )
			{
				if(ppx)
				{
					// move back
					pp = ppx;
					ps = psx;
				}
				else
					break;
			}
		}

		auto match = (ps != end && *ps == '\n' && !*pp) || (ppx && !*ppx);
		for( ; ps != end && *ps != '\n'; ++ps ); // skip to the end of the line
		if( ps != end )
		{
			if(match)
			{
				results.Push( {ln, ps} );
			}

			++ps;
		}
		else
			return ln; // return the unprocessed piece
	}

	return end;
}

void CLogReader::Worker::Start( const Text& seq, const char* fltr )
{
	text = seq;
	filter = fltr;

	[[maybe_unused]] auto err = pthread_create( &thread, nullptr, Work, this );
	assert( !err );
}

void CLogReader::Worker::Wait()
{
	pthread_join( thread, nullptr );
}

void* CLogReader::Worker::Work( void* param )
{
	auto ths = reinterpret_cast< Worker* >(param);
	ths->rest = Process( ths->text, ths->filter, ths->results );

	return nullptr;
}

CLogReader::Printer::Printer() : file(stdout)
{
}

CLogReader::Printer::Printer( const char* path )
{
	file = fopen( path, "w" );
}

CLogReader::Printer::~Printer()
{
	if( file != stdout )
	{
		fclose(file);
	}
}

void CLogReader::Printer::Handle( const Sequence<Line>& lines )
{
	for( const auto& line : lines )
	{
		fwrite( line.from, 1, line.length(), file );
		fputc( '\n', file );
	}
}
