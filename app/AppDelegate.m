//
//  AppDelegate.m
//  logreader
//
//  Created by Yakov Karnygin on 20.02.2021.
//

#import "AppDelegate.h"
#import "ViewController.h"

@interface AppDelegate ()
{
	ViewController* viewController;
}

@end

@implementation AppDelegate


- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
	UIStoryboard *storyboard = [UIStoryboard storyboardWithName: @"Main" bundle: nil];
	viewController = [[storyboard instantiateViewControllerWithIdentifier: @"Main"] retain];

	self.window = [[UIWindow alloc] initWithFrame: [[UIScreen mainScreen] bounds]];
	self.window.backgroundColor = [UIColor whiteColor];

	self.window.rootViewController = viewController;
	[self.window makeKeyAndVisible];

	return YES;
}

-(void)dealloc
{
	[viewController release];
	[self.window release];

	[super dealloc];
}

@end
