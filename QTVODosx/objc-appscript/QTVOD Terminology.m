/*
 * QTVOD Terminology.m
 */

#include "QTVOD Terminology.h"




/*
 * Standard Suite
 */

@implementation QTVODTerminologyApplication

typedef struct { NSString *name; FourCharCode code; } classForCode_t;
static const classForCode_t classForCodeData__[] = {
	{ @"QTVODTerminologyApplication", 'capp' },
	{ @"QTVODTerminologyDocument", 'docu' },
	{ @"QTVODTerminologyWindow", 'cwin' },
	{ @"QTVODTerminologyDocument", 'docu' },
	{ @"QTVODTerminologyQtMovieView", 'QTMV' },
	{ nil, 0 } 
};

- (NSDictionary *) classNamesForCodes
{
	static NSMutableDictionary *dict__;

	if (!dict__) @synchronized([self class]) {
	if (!dict__) {
		dict__ = [[NSMutableDictionary alloc] init];
		const classForCode_t *p;
		for (p = classForCodeData__; p->name != nil; ++p)
			[dict__ setObject:p->name forKey:[NSNumber numberWithInt:p->code]];
	} }
	return dict__;
}

typedef struct { FourCharCode code; NSString *name; } codeForPropertyName_t;
static const codeForPropertyName_t codeForPropertyNameData__[] = {
	{ 'pnam', @"name" },
	{ 'pisf', @"frontmost" },
	{ 'vers', @"version" },
	{ 'pnam', @"name" },
	{ 'imod', @"modified" },
	{ 'file', @"file" },
	{ 'pnam', @"name" },
	{ 'ID  ', @"id" },
	{ 'pidx', @"index" },
	{ 'pbnd', @"bounds" },
	{ 'hclb', @"closeable" },
	{ 'ismn', @"miniaturizable" },
	{ 'pmnd', @"miniaturized" },
	{ 'prsz', @"resizable" },
	{ 'pvis', @"visible" },
	{ 'iszm', @"zoomable" },
	{ 'pzum', @"zoomed" },
	{ 'docu', @"document" },
	{ 'pnam', @"name" },
	{ 'file', @"file" },
	{ 'ID  ', @"id" },
	{ 'FTPc', @"path" },
	{ 'CURT', @"currentTime" },
	{ 'ACRT', @"absCurrentTime" },
	{ 'DUTN', @"duration" },
	{ 'STTM', @"startTime" },
	{ 'FMRT', @"frameRate" },
	{ 'TFRT', @"TCframeRate" },
	{ 'tINT', @"lastInterval" },
	{ 'CHNS', @"chapterNames" },
	{ 'CHTS', @"chapterTimes" },
	{ 'CHPS', @"chapters" },
	{ 'QTmv', @"movieView" },
	{ 0, nil } 
};

- (NSDictionary *) codesForPropertyNames
{
	static NSMutableDictionary *dict__;

	if (!dict__) @synchronized([self class]) {
	if (!dict__) {
		dict__ = [[NSMutableDictionary alloc] init];
		const codeForPropertyName_t *p;
		for (p = codeForPropertyNameData__; p->name != nil; ++p)
			[dict__ setObject:[NSNumber numberWithInt:p->code] forKey:p->name];
	} }
	return dict__;
}


- (SBElementArray *) documents
{
	return [self elementArrayWithCode:'docu'];
}


- (SBElementArray *) windows
{
	return [self elementArrayWithCode:'cwin'];
}



- (NSString *) name
{
	return [[self propertyWithCode:'pnam'] get];
}

- (BOOL) frontmost
{
	id v = [[self propertyWithCode:'pisf'] get];
	return [v boolValue];
}

- (NSString *) version
{
	return [[self propertyWithCode:'vers'] get];
}


- (id) open:(id)x
{
	id result__ = [self sendEvent:'aevt' id:'odoc' parameters:'----', x, 0];
	return result__;
}

- (void) print:(id)x withProperties:(NSDictionary *)withProperties printDialog:(BOOL)printDialog
{
	[self sendEvent:'aevt' id:'pdoc' parameters:'----', x, 'prdt', withProperties, 'pdlg', [NSNumber numberWithBool:printDialog], 0];
}

- (void) quitSaving:(QTVODTerminologySaveOptions)saving
{
	[self sendEvent:'aevt' id:'quit' parameters:'savo', [NSAppleEventDescriptor descriptorWithEnumCode:saving], 0];
}

- (BOOL) exists:(id)x
{
	id result__ = [self sendEvent:'core' id:'doex' parameters:'----', x, 0];
	return [result__ boolValue];
}


@end


@implementation QTVODTerminologyDocument


- (NSString *) name
{
	return [[self propertyWithCode:'pnam'] get];
}

- (BOOL) modified
{
	id v = [[self propertyWithCode:'imod'] get];
	return [v boolValue];
}

- (NSURL *) file
{
	return [[self propertyWithCode:'file'] get];
}


- (void) closeSaving:(QTVODTerminologySaveOptions)saving savingIn:(NSURL *)savingIn
{
	[self sendEvent:'core' id:'clos' parameters:'savo', [NSAppleEventDescriptor descriptorWithEnumCode:saving], 'kfil', savingIn, 0];
}

- (void) saveIn:(NSURL *)in_ as:(id)as
{
	[self sendEvent:'core' id:'save' parameters:'kfil', in_, 'fltp', as, 0];
}

- (void) printWithProperties:(NSDictionary *)withProperties printDialog:(BOOL)printDialog
{
	[self sendEvent:'aevt' id:'pdoc' parameters:'prdt', withProperties, 'pdlg', [NSNumber numberWithBool:printDialog], 0];
}

- (void) delete
{
	[self sendEvent:'core' id:'delo' parameters:0];
}

- (void) duplicateTo:(SBObject *)to withProperties:(NSDictionary *)withProperties
{
	[self sendEvent:'core' id:'clon' parameters:'insh', to, 'prdt', withProperties, 0];
}

- (void) moveTo:(SBObject *)to
{
	[self sendEvent:'core' id:'move' parameters:'insh', to, 0];
}

- (void) close
{
	[self sendEvent:'QVOD' id:'clse' parameters:0];
}

- (void) play
{
	[self sendEvent:'QVOD' id:'strt' parameters:0];
}

- (void) stop
{
	[self sendEvent:'QVOD' id:'stop' parameters:0];
}

- (void) stepForward
{
	[self sendEvent:'QVOD' id:'nfrm' parameters:0];
}

- (void) stepBackward
{
	[self sendEvent:'QVOD' id:'pfrm' parameters:0];
}

- (void) addChapterName:(NSString *)name startTime:(double)startTime duration:(double)duration
{
	[self sendEvent:'QVOD' id:'nchp' parameters:'pnam', name, 'NCST', [NSNumber numberWithDouble:startTime], 'NCDU', [NSNumber numberWithDouble:duration], 0];
}

- (void) markTimeIntervalReset:(BOOL)reset display:(BOOL)display
{
	[self sendEvent:'QVOD' id:'tINT' parameters:'rset', [NSNumber numberWithBool:reset], 'dply', [NSNumber numberWithBool:display], 0];
}

- (void) resetVideoComplete:(BOOL)complete
{
	[self sendEvent:'QVOD' id:'rset' parameters:'cmpl', [NSNumber numberWithBool:complete], 0];
}

- (void) readDesignName:(NSString *)name
{
	[self sendEvent:'QVOD' id:'dsgn' parameters:'pnam', name, 0];
}


@end


@implementation QTVODTerminologyWindow


- (NSString *) name
{
	return [[self propertyWithCode:'pnam'] get];
}

- (NSInteger) id
{
	id v = [[self propertyWithCode:'ID  '] get];
	return [v integerValue];
}

- (NSInteger) index
{
	id v = [[self propertyWithCode:'pidx'] get];
	return [v integerValue];
}

- (void) setIndex: (NSInteger) index
{
	id v = [NSNumber numberWithInteger:index];
	[[self propertyWithCode:'pidx'] setTo:v];
}

- (NSRect) bounds
{
	id v = [[self propertyWithCode:'pbnd'] get];
	return [v rectValue];
}

- (void) setBounds: (NSRect) bounds
{
	id v = [NSValue valueWithRect:bounds];
	[[self propertyWithCode:'pbnd'] setTo:v];
}

- (BOOL) closeable
{
	id v = [[self propertyWithCode:'hclb'] get];
	return [v boolValue];
}

- (BOOL) miniaturizable
{
	id v = [[self propertyWithCode:'ismn'] get];
	return [v boolValue];
}

- (BOOL) miniaturized
{
	id v = [[self propertyWithCode:'pmnd'] get];
	return [v boolValue];
}

- (void) setMiniaturized: (BOOL) miniaturized
{
	id v = [NSNumber numberWithBool:miniaturized];
	[[self propertyWithCode:'pmnd'] setTo:v];
}

- (BOOL) resizable
{
	id v = [[self propertyWithCode:'prsz'] get];
	return [v boolValue];
}

- (BOOL) visible
{
	id v = [[self propertyWithCode:'pvis'] get];
	return [v boolValue];
}

- (void) setVisible: (BOOL) visible
{
	id v = [NSNumber numberWithBool:visible];
	[[self propertyWithCode:'pvis'] setTo:v];
}

- (BOOL) zoomable
{
	id v = [[self propertyWithCode:'iszm'] get];
	return [v boolValue];
}

- (BOOL) zoomed
{
	id v = [[self propertyWithCode:'pzum'] get];
	return [v boolValue];
}

- (void) setZoomed: (BOOL) zoomed
{
	id v = [NSNumber numberWithBool:zoomed];
	[[self propertyWithCode:'pzum'] setTo:v];
}

- (QTVODTerminologyDocument *) document
{
	return (QTVODTerminologyDocument *) [self propertyWithClass:[QTVODTerminologyDocument class] code:'docu'];
}


- (void) closeSaving:(QTVODTerminologySaveOptions)saving savingIn:(NSURL *)savingIn
{
	[self sendEvent:'core' id:'clos' parameters:'savo', [NSAppleEventDescriptor descriptorWithEnumCode:saving], 'kfil', savingIn, 0];
}

- (void) saveIn:(NSURL *)in_ as:(id)as
{
	[self sendEvent:'core' id:'save' parameters:'kfil', in_, 'fltp', as, 0];
}

- (void) printWithProperties:(NSDictionary *)withProperties printDialog:(BOOL)printDialog
{
	[self sendEvent:'aevt' id:'pdoc' parameters:'prdt', withProperties, 'pdlg', [NSNumber numberWithBool:printDialog], 0];
}

- (void) delete
{
	[self sendEvent:'core' id:'delo' parameters:0];
}

- (void) duplicateTo:(SBObject *)to withProperties:(NSDictionary *)withProperties
{
	[self sendEvent:'core' id:'clon' parameters:'insh', to, 'prdt', withProperties, 0];
}

- (void) moveTo:(SBObject *)to
{
	[self sendEvent:'core' id:'move' parameters:'insh', to, 0];
}


@end




/*
 * QTVOD Scripting Suite
 */

@implementation QTVODTerminologyDocument(QTVODScriptingSuite)


- (NSString *) name
{
	return [[self propertyWithCode:'pnam'] get];
}

- (NSURL *) file
{
	return [[self propertyWithCode:'file'] get];
}

- (NSString *) id
{
	return [[self propertyWithCode:'ID  '] get];
}

- (NSString *) path
{
	return [[self propertyWithCode:'FTPc'] get];
}

- (double) currentTime
{
	id v = [[self propertyWithCode:'CURT'] get];
	return [v doubleValue];
}

- (void) setCurrentTime: (double) currentTime
{
	id v = [NSNumber numberWithDouble:currentTime];
	[[self propertyWithCode:'CURT'] setTo:v];
}

- (double) absCurrentTime
{
	id v = [[self propertyWithCode:'ACRT'] get];
	return [v doubleValue];
}

- (void) setAbsCurrentTime: (double) absCurrentTime
{
	id v = [NSNumber numberWithDouble:absCurrentTime];
	[[self propertyWithCode:'ACRT'] setTo:v];
}

- (double) duration
{
	id v = [[self propertyWithCode:'DUTN'] get];
	return [v doubleValue];
}

- (double) startTime
{
	id v = [[self propertyWithCode:'STTM'] get];
	return [v doubleValue];
}

- (double) frameRate
{
	id v = [[self propertyWithCode:'FMRT'] get];
	return [v doubleValue];
}

- (double) TCframeRate
{
	id v = [[self propertyWithCode:'TFRT'] get];
	return [v doubleValue];
}

- (double) lastInterval
{
	id v = [[self propertyWithCode:'tINT'] get];
	return [v doubleValue];
}

- (NSArray *) chapterNames
{
	return [[self propertyWithCode:'CHNS'] get];
}

- (NSArray *) chapterTimes
{
	return [[self propertyWithCode:'CHTS'] get];
}

- (NSArray *) chapters
{
	return [[self propertyWithCode:'CHPS'] get];
}


@end


@implementation QTVODTerminologyQtMovieView


- (void) closeSaving:(QTVODTerminologySaveOptions)saving savingIn:(NSURL *)savingIn
{
	[self sendEvent:'core' id:'clos' parameters:'savo', [NSAppleEventDescriptor descriptorWithEnumCode:saving], 'kfil', savingIn, 0];
}

- (void) saveIn:(NSURL *)in_ as:(id)as
{
	[self sendEvent:'core' id:'save' parameters:'kfil', in_, 'fltp', as, 0];
}

- (void) printWithProperties:(NSDictionary *)withProperties printDialog:(BOOL)printDialog
{
	[self sendEvent:'aevt' id:'pdoc' parameters:'prdt', withProperties, 'pdlg', [NSNumber numberWithBool:printDialog], 0];
}

- (void) delete
{
	[self sendEvent:'core' id:'delo' parameters:0];
}

- (void) duplicateTo:(SBObject *)to withProperties:(NSDictionary *)withProperties
{
	[self sendEvent:'core' id:'clon' parameters:'insh', to, 'prdt', withProperties, 0];
}

- (void) moveTo:(SBObject *)to
{
	[self sendEvent:'core' id:'move' parameters:'insh', to, 0];
}


@end


