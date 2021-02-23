//
//  ViewController.m
//  logreader
//
//  Created by Yakov Karnygin on 20.02.2021.
//

#include "resreader.h"
#include "processor.h"

#import "ViewController.h"

@interface ViewController ()

@end

@implementation ViewController
{
	IBOutlet UITableView *resultsView;

	IBOutlet UITextField *urlField;
	IBOutlet UILabel *urlStatusLabel;
	IBOutlet UIActivityIndicatorView *urlProgress;

	IBOutlet UITextField *filterField;
	IBOutlet UILabel *filterStatusLabel;
	IBOutlet UIActivityIndicatorView *filterProgress;

	NSString *input; // input file path
	NSString *indir, *outdir; // input and output directories paths

	Processor processor;
	ResultReader reader;

	NSURLSessionDownloadTask* downloadTask;

	NSFileManager* filemgr; // alias
}

- (void)viewDidLoad
{
	[super viewDidLoad];

	filemgr = NSFileManager.defaultManager;

	indir = [[[NSSearchPathForDirectoriesInDomains( NSCachesDirectory, NSUserDomainMask, YES) firstObject ] stringByAppendingPathComponent: @"input"] retain];
	outdir = [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject] retain];

	[filemgr createDirectoryAtPath: indir withIntermediateDirectories: YES attributes: nil error: nil];

	// look for a previously stored file
	NSDirectoryEnumerator* enumerator = [filemgr enumeratorAtPath: indir];
	NSString* name = [enumerator nextObject];
	if(name)
	{
		[self downloadComplete: [indir stringByAppendingPathComponent: name]];
	}
}

- (void)dealloc
{
	[self cancelDownload];

	[input release];

	[indir release];
	[outdir release];

	[resultsView release];
	[filterField release];
	[urlField release];
	[urlProgress release];
	[urlStatusLabel release];
	[filterStatusLabel release];
	[filterProgress release];
	[super dealloc];
}

#pragma mark - Table View Data source
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection: (NSInteger)section
{
	return reader.total();
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath: (NSIndexPath *)indexPath
{
	static NSString *cellId = @"mycell";

	UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier: cellId];
	if (cell == nil)
	{
		cell = [[UITableViewCell alloc] initWithStyle: UITableViewCellStyleDefault reuseIdentifier: cellId];

		cell.textLabel.font = [UIFont systemFontOfSize: 11.0];
		cell.textLabel.numberOfLines = 0;
		cell.textLabel.lineBreakMode = NSLineBreakByCharWrapping;

		[cell autorelease];
	}

	if( indexPath.row >= 0 && indexPath.row < reader.total() )
	{
		auto&& line = reader[ indexPath.row ];
		[cell.textLabel setText: [NSString stringWithUTF8String: line.from]];
	}

	return cell;
}

NSString* decorateFilename( NSString* filename )
{
	static const size_t LIMIT = 16;

	if( [filename length] > LIMIT )
	{
		return [NSString stringWithFormat: @"%@...", [filename substringToIndex: LIMIT]];
	}

	return filename;
}

- (void)download
{
	NSURL *url = [NSURL URLWithString: urlField.text];
	if(url)
	{
		urlField.enabled = NO;
		[urlProgress startAnimating];

		urlStatusLabel.hidden = YES;

		void (^handler)(NSURL*, NSURLResponse*, NSError*) = ^(NSURL *location, NSURLResponse *response, NSError *error)
		{
			NSString* path = [[indir stringByAppendingPathComponent: response.suggestedFilename] retain];

			if( !error )
			{
				[filemgr removeItemAtPath: path error: nil];
				[filemgr moveItemAtPath: [location path] toPath: path error: &error];
			}

			dispatch_block_t block = ^
			{
				if( [self cancelDownload] )
				{
					!error ? [self downloadComplete: path] : [self downloadFailed];
				}

				[path release];
			};

			dispatch_async( dispatch_get_main_queue(), block );
		};

		NSURLSession* session = [NSURLSession sharedSession];
		downloadTask = [[session downloadTaskWithURL: url completionHandler: handler] retain];
		[downloadTask resume];
	}
}

- (BOOL)cancelDownload
{
	if(downloadTask)
	{
		urlField.enabled = YES;
		[urlProgress stopAnimating];

		[downloadTask cancel];
		[downloadTask release];
		downloadTask = nil;

		return YES;
	}

	return NO;
}

- (void)downloadComplete: (NSString*) path
{
	int size = [[[filemgr attributesOfItemAtPath: path error: nil] valueForKey: NSFileSize] intValue];

	urlStatusLabel.text = [NSString stringWithFormat: @"downloaded %@, %d Mb", decorateFilename( [path lastPathComponent] ), unsigned(size / 1024 / 1024) ];
	urlStatusLabel.textColor = [UIColor systemBlueColor];
	urlStatusLabel.hidden = NO;

	if( ![input isEqualToString: path] )
	{
		[filemgr removeItemAtPath: input error: nil];
		[input release];
		input = [path retain];
	}

	filterField.enabled = YES;
}

- (void)downloadFailed
{
	NSURL *url = [NSURL URLWithString: urlField.text];
	if(url)
	{
		urlStatusLabel.text = [NSString stringWithFormat: @"failed downloading %@", decorateFilename( [urlField.text lastPathComponent] ) ];
		urlStatusLabel.textColor = [UIColor systemRedColor];
		urlStatusLabel.hidden = NO;
	}
}

- (void)search
{
	filterStatusLabel.hidden = YES;

	processor = Processor( [input UTF8String], [filterField.text UTF8String], [outdir UTF8String] );
	if( processor.ready() )
	{
		dispatch_block_t block = ^
		{
			time_t sec = processor.Process();
			dispatch_async( dispatch_get_main_queue(), ^{ [self searchCompleteIn: sec]; } );
		};

		dispatch_queue_global_t queue = dispatch_get_global_queue( QOS_CLASS_USER_INITIATED, 0 );
		dispatch_async( queue, block );

		filterField.enabled = NO;
		[filterProgress startAnimating ];
	}
	else
		[self searchFailed];
}

- (void)searchCompleteIn: (time_t)sec
{
	const char* rpath = [[outdir stringByAppendingPathComponent: @"results.log"] UTF8String];
	const char* ipath = [[outdir stringByAppendingPathComponent: @"results.idx"] UTF8String];
	reader = ResultReader( rpath, ipath );

	filterStatusLabel.text = [NSString stringWithFormat: @"found %d entries, %ds", unsigned( reader.total() ), int(sec)];
	filterStatusLabel.textColor = [UIColor systemBlueColor];
	filterStatusLabel.hidden = NO;

	filterField.enabled = YES;
	[filterProgress stopAnimating ];

	[resultsView reloadData];
}

- (void)searchFailed
{
	filterStatusLabel.text = @"error";
	filterStatusLabel.textColor = [UIColor systemRedColor];
	filterStatusLabel.hidden = NO;

	[filterProgress stopAnimating ];
}

- (BOOL)textFieldShouldReturn: (UITextField *) sender
{
	if( sender == urlField )
	{
		[filterField becomeFirstResponder];
		[self download];
	}
	else if( sender == filterField )
	{
		[resultsView becomeFirstResponder];
		[self search];
	}

	return NO;
}
@end
