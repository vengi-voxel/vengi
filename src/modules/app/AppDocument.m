#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

extern void setLoadingDocument(const char *path);

@interface AppDocument : NSDocument

@end

@implementation AppDocument

- (BOOL) readFromURL:(NSURL *)url ofType:(NSString *)typeName error:(NSError **)outError {
	NSString* urlString = [url absoluteString];
	const char* utf8String = [urlString UTF8String];
	setLoadingDocument(utf8String);
	return YES;
}

@end
