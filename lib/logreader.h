#ifndef __LOGREADER_HEADER__
#define __LOGREADER_HEADER__

#include "basic.h"
#include "deque.h"

#include <pthread.h>
#include <stdio.h>

class CLogReader
{
	static constexpr size_t WORKERS = 4; // maximum workers number
	static constexpr size_t BLOCK = 256 * 1024; // minimal block size per worker

	using Text = Sequence<char>;
	using Line = Sequence<char>;

public:
	struct Handler;
	struct Printer; // a Handler which prints results to a specific file or stdout

	CLogReader( Handler* );
	CLogReader(); // uses the default printer
	~CLogReader();

	bool SetFilter( const char* );
	bool AddSourceBlock( const char*, const size_t );

private:
	using Results = Deque< Line, 128 >;

	// returns a pointer to unprocessed trailing piece
	static const char* Process( const Text&, const char* filter, Results& );

	// distributes work among the workers
	// returns the number of workers involved
	size_t Dispatch( const Text& );

	struct Worker
	{
		void Start( const Text&, const char* filter ); // starts asynchronous work
		void Wait(); // waits until finishes

		Results results;
		const char* rest; // points to end of the processed piece

		static void* Work( void* param ); // thread func

		const char* filter;
		Text text;

		pthread_t thread;
	};

	Worker workers[WORKERS];

	Handler* handler;

	char* filter = nullptr;

	Buffer<char> tail; // holds unprocessed piece from the previous
};

struct CLogReader::Handler
{
	using Line = Sequence<char>;
	virtual void Handle( const Sequence<Line>& ) = 0;
};

struct CLogReader::Printer : public Handler
{
public:
	Printer( const char* path );
	Printer();
	~Printer();

	void Handle( const Sequence<Line>& ) override;

	static Printer dflt;

private:
	FILE* file;
};

#endif // __LOGREADER_HEADER__
