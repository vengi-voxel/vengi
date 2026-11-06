/**
 * @file
 * @brief macOS Quick Look Thumbnail app extension (QLThumbnailProvider).
 *
 * Spawns the bundled vengi-thumbnailer CLI (same approach as the Windows COM handler)
 * and returns the rendered PNG to Finder.
 */

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#import <QuickLookThumbnailing/QuickLookThumbnailing.h>

#include "ThumbnailerRunner.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "io/Filesystem.h"

#include <math.h>
#include <unistd.h>

@interface ThumbnailProvider : QLThumbnailProvider
@end

@implementation ThumbnailProvider

- (void)provideThumbnailForFileRequest:(QLFileThumbnailRequest *)request
					 completionHandler:(void (^)(QLThumbnailReply *_Nullable, NSError *_Nullable))handler {
	NSURL *fileURL = request.fileURL;
	if (fileURL == nil || fileURL.path == nil) {
		handler(nil, [NSError errorWithDomain:NSPOSIXErrorDomain code:EINVAL userInfo:nil]);
		return;
	}

	const CGFloat pixelWidth = request.maximumSize.width * request.scale;
	const CGFloat pixelHeight = request.maximumSize.height * request.scale;
	int size = (int)ceil(fmax(pixelWidth, pixelHeight));
	if (size < 16) {
		size = 16;
	}
	if (size > 2048) {
		size = 2048;
	}

	char templatePath[] = "/tmp/vengi-voxelthumb.XXXXXX";
	if (mkdtemp(templatePath) == nullptr) {
		handler(nil, [NSError errorWithDomain:NSPOSIXErrorDomain code:errno userInfo:nil]);
		return;
	}
	const core::String workDir(templatePath);
	const core::String outputFile = core::string::path(workDir, "thumbnail.png");

	NSString *executablePath = [[NSBundle mainBundle] executablePath];
	thumbnailer::RenderRequest renderRequest;
	renderRequest.modulePath = executablePath != nil ? executablePath.UTF8String : "";
	renderRequest.inputFile = fileURL.path.UTF8String;
	renderRequest.outputFile = outputFile;
	renderRequest.workingDirectory = workDir;
	renderRequest.size = size;
	renderRequest.useSceneCamera = true;

	core::String errorMessage;
	const bool ok = thumbnailer::renderThumbnail(renderRequest, &errorMessage);
	if (!ok) {
		io::Filesystem::sysRemoveDir(workDir, true);
		NSString *message = [NSString stringWithUTF8String:errorMessage.c_str()];
		handler(nil, [NSError errorWithDomain:@"com.github.vengi.thumbnailer"
										 code:1
									 userInfo:@{NSLocalizedDescriptionKey : message ?: @"Thumbnailer failed"}]);
		return;
	}

	NSImage *image = [[NSImage alloc] initWithContentsOfFile:[NSString stringWithUTF8String:outputFile.c_str()]];
	io::Filesystem::sysRemoveDir(workDir, true);
	if (image == nil || image.size.width <= 0.0 || image.size.height <= 0.0) {
		handler(nil, [NSError errorWithDomain:@"com.github.vengi.thumbnailer"
										 code:2
									 userInfo:@{NSLocalizedDescriptionKey : @"Failed to load thumbnail PNG"}]);
		return;
	}

	const CGSize contextSize = request.maximumSize;
	QLThumbnailReply *reply = [QLThumbnailReply replyWithContextSize:contextSize
										currentContextDrawingBlock:^BOOL {
		const NSSize imageSize = image.size;
		const CGFloat scale = fmin(contextSize.width / imageSize.width, contextSize.height / imageSize.height);
		const CGFloat drawWidth = imageSize.width * scale;
		const CGFloat drawHeight = imageSize.height * scale;
		const CGFloat originX = (contextSize.width - drawWidth) * 0.5;
		const CGFloat originY = (contextSize.height - drawHeight) * 0.5;
		[image drawInRect:NSMakeRect(originX, originY, drawWidth, drawHeight)
				 fromRect:NSZeroRect
				operation:NSCompositingOperationCopy
				 fraction:1.0];
		return YES;
	}];

	handler(reply, nil);
}

@end
