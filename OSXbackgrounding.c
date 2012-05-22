/*
Jan E. Schotsman <jeschot@xs4all.nl>
Hello RenŽ,

This kind of code is platform specific. It has little to do with QuickTime,
except for the "attach/detach movie to thread" API which you may not need for compression.

Of course writing threaded mode is possible on each platform.
I only know the Carbon way but it shouldn't be hard to write similar code for Cocoa or Windows.

On the main thread you create the compression thread(s).
Use MPNotifyQueue to send the frames to be encoded to the input queue. You can pass on three pointers to data.
I use the source tracking callback to release the pixel buffer and free the input buffer.
In the output callback I call ICMEncodedFrameGetImageDescription once and ICMEncodedFrameGetDataPtr and ICMEncodedFrameGetDataSize for each frame. You could send an event to the main thread and call AddMediaSample there. Or attach the movie to the thread and do it on the thread.
If you have multiple threads it is probably better to build the movie on the main thread.
BTW some codecs profit from multiple tasks, others don't. ProRes 422 is well optimized - a single thread is enough. For DVCProHD I use two copies of MyCompressTask. Then you have to make sure the frames are added to the movie in the right order, of course.


A barebones version of my code looks like this:
 */

typedef struct MyTaskData {
	TaskProc					entryPoint;
	MPTaskID					ID;
	void *					termMessage1;
	void *					termMessage2;
	MPQueueID				inputQueue;
	MPQueueID				termQueue;	//	Notified automatically on exit by system
	SInt32					number;
	OSType					type;
	void *					data;
} MyTaskData;



typedef struct MyCompressData {
	UPtr					outBuffer;
	UPtr					inBuffer;
	SInt32					dataSize;
	ICMSourceTrackingFlags		srcTrackingFlags;
	CVPixelBufferRef			pixelBuffer;
} MyCompressData;


MyTaskData taskData;

taskData->entryPoint =	MyCompressTask;
err = MPCreateQueue( &taskData->inputQueue );
err = MPCreateQueue( &taskData->termQueue );

err = MPCreateTask( taskData->entryPoint, taskData, stackSize, taskData->termQueue,
				taskData->termMessage1, taskData->termMessage2, 0, &taskData->ID );


/* ------------------------------------------------------------------------------------------------------------------------------------ */
OSStatus MyCompressTask( void *parameter )
{
	OSStatus		err =	noErr;
	
	MyTaskData *	taskData = (MyTaskData *)parameter;
	MyCompressData *compressData;
	
	
	err	= EnterMoviesOnThread(0);
	
	CSSetComponentsThreadMode( kCSAcceptThreadSafeComponentsOnlyMode );
	
	err = MyCreateCompressionSession(	outCodecType, spatialQuality, codecSettings, &compressionSession );
	
	srcTrackingCb.sourceTrackingCallback =	MySrcTrackingCallback;
	srcTrackingCb.sourceTrackingRefCon = nil;
	
	while ( 1)
	{
		err	= MPWaitOnQueue( taskData->inputQueue, nil, (void **)&compressData, nil, kDurationForever );
		if ( compressData == nil ) break;	//	end signal
		
		err = CVPixelBufferCreateWithBytes( nil, width, height, '2vuy', compressData->inBuffer, 2*width, nil, nil,
											nil, &compressData->pixelBuffer );
		
		compressData->srcTrackingFlags =		0;
		
		err = ICMCompressionSessionEncodeFrame( compressionSession, compressData->pixelBuffer,
												displayTime, displayDuration, validTimeFlags,							
												nil, &srcTrackingCb, (void *)compressData );
	}
	
	if ( compressionSession )
	{
		ICMCompressionSessionCompleteFrames( compressionSession, true, 0, 0 );
		ICMCompressionSessionRelease( compressionSession );
	}
	
	ExitMoviesOnThread();
	return err;		
}

/* ------------------------------------------------------------------------------------------------------------------------------------ */
OSStatus MyCreateCompressionSession( DE_ProjectPtr project, CodecType cType, CodecQ quality, Handle codecSett,
							SInt32 width, SInt32 height, TimeScale movieTimeScale, ICMCompressionSessionRef *compressionSession )
{
	OSStatus					err = noErr;
	ICMEncodedFrameOutputRecord	encodedFrameOutputRecord = {};
	ICMCompressionSessionOptionsRef sessionOptions = nil;
	CompressorComponent		ci = anyCodec;
	
	
	err = ICMCompressionSessionOptionsCreate( nil, &sessionOptions );
	
	err = MyICMCompressionSessionOptionsSetComprComp( sessionOptions, ci );
	
	err = MyICMCompressionSessionOptionsSetComprQuality( sessionOptions, quality );
	
	if ( codecSett ) err = MyICMCompressionSessionOptionsSetCodecSett( sessionOptions, codecSett );
	
	encodedFrameOutputRecord.encodedFrameOutputCallback = MyComprOutputCallback;
	encodedFrameOutputRecord.encodedFrameOutputRefCon = nil;
	
	err = ICMCompressionSessionCreate( nil, width, height, cType, movieTimeScale, sessionOptions, nil, &encodedFrameOutputRecord, compressionSession );
	
	ICMCompressionSessionOptionsRelease( sessionOptions );
	
	return err;
}

The MyICMCompressionSessionOptionsSet... functions go like this:

/* ------------------------------------------------------------------------------------------------------------------------------------ */
OSStatus MyICMCompressionSessionOptionsSetComprComp( ICMCompressionSessionOptionsRef sessionOptions, CompressorComponent ci )
{
	OSStatus		err = noErr;
	
	err = ICMCompressionSessionOptionsSetProperty( sessionOptions, kQTPropertyClass_ICMCompressionSessionOptions,
												kICMCompressionSessionOptionsPropertyID_CompressorComponent,
												sizeof(CompressorComponent), (ConstComponentValuePtr)&ci );
	return err;
}
