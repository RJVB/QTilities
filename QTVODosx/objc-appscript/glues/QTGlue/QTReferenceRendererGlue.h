/*
 * QTReferenceRendererGlue.h
 * /Volumes/Debian/Users/bertin/work/src/MacOSX/QTilities/build/Deployment/QTVOD.app
 * osaglue 0.5.4
 *
 */

#import <Foundation/Foundation.h>
#import "Appscript/Appscript.h"

@interface QTReferenceRenderer : ASReferenceRenderer
- (NSString *)propertyByCode:(OSType)code;
- (NSString *)elementByCode:(OSType)code;
- (NSString *)prefix;
@end

