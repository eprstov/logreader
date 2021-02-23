#ifndef __PROCESSOR_HEADER__
#define __PROCESSOR_HEADER__

#include <lib/basic.h>
#include <lib/logreader.h>

#include <stddef.h>
#include <stdio.h>
#include <time.h>

extern const char RESULT[];
extern const char INDEX[];

class Processor
{
public:
	using Index = Buffer<size_t>;

	Processor( const char* input, const char* filter, const char* outdir );
	Processor();
	~Processor();

	Processor& operator=( Processor&& );
	Processor& operator=( const Processor& ) = delete;

	time_t Process();

	Index&& index() const;
	bool ready() const;

private:
	struct Writer : public CLogReader::Handler
	{
		Writer( const char* dir );
		~Writer();

		void Handle( const Sequence<Line>& ) override;

		void StoreOffset();
		void Flush();

		bool ready() const { return fresult && findex; }

		FILE *fresult = nullptr, *findex = nullptr;

		size_t offset = 0;
	};

	void Cleanup();

	FILE* finput = nullptr;
	const char* outdir;

	Writer* writer = nullptr;
	CLogReader* reader = nullptr;
};

inline bool Processor::ready() const
{
	return finput && writer && writer->ready() && reader;
}

#endif // !__PROCESSOR_HEADER__
