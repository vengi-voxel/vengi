/**
 * @file
 */

#include <Foundation/NSUserDefaults.h>
#include <Foundation/Foundation.h>

bool isOSXDarkMode() {
	NSString * appleInterfaceStyle = [[NSUserDefaults standardUserDefaults] stringForKey:@"AppleInterfaceStyle"];
	if (appleInterfaceStyle && [appleInterfaceStyle length] > 0) {
		return [[appleInterfaceStyle lowercaseString] containsString:@"dark"];
	}
	return false;
}
