#include "qtmoviefile.h"
#include <iostream>

#define		kVideoTimeScale 	600
//#define		kNumVideoFrames 	70
#define		kPixelDepth 		32	/* use 32-bit depth */
#define		kNoOffset 			0
#define		kMgrChoose			0
#define		kSyncSample 		0
#define		kAddOneVideoSample	1
#define		kSampleDuration 	20	/* frame duration = 600 = 1 sec */
#define		kTrackStart			0
#define		kMediaStart			0

#ifndef		VIDEO_CODEC_TYPE
#define		VIDEO_CODEC_TYPE	kJPEGCodecType
#endif
#ifndef		VIDEO_CODEC_QUALITY
#define		VIDEO_CODEC_QUALITY	codecNormalQuality
#endif

CQTMovieFile::CQTMovieFile (const std::string& datafile) : _initialized(false)
{	
    _filename = datafile.substr(0, datafile.find_last_of('.')) + ".mov";

    m_pMovie=NULL;
    m_pTrack=NULL;
    m_pMedia=NULL;
    m_resRefNum=0;
    m_gWorld=NULL;
    m_savedGD=NULL;	
    m_savedPort=NULL;
    m_imageDesc=NULL;
    m_compressedData=NULL;;
    m_compressedDataPtr=NULL;
}
    
void CQTMovieFile::Initialize (const osg::Camera& camera)
{
	const osg::Viewport* v = camera.getViewport();
	_x = v->x();
	_y = v->y();
	_w = /*v->width()*/  640 + _x;
	_h = /*v->height()*/ 480 + _y;

    m_trackFrame.top    = (short)_x; 
    m_trackFrame.left   = (short)_y; 
    m_trackFrame.right  = (short)(_x+_w);
    m_trackFrame.bottom = (short)(_y+_h); 

    _imageD.resize(_w*_h);

    if (noErr != InitializeQTML(0)) {
        std::cerr << "QTMovieFile::Initialize(), InitializeQTML(0) failed." << std::endl;
        exit(-1);
    }

    char tmp[MAX_PATH]; if (NULL == _fullpath (tmp,_filename.c_str(),sizeof(tmp))) {
        std::cerr << "QTMovieFile::Initialize(), _fullpath() failed - " << strerror(errno) << std::endl;
        exit (-1);
    } c2pstrcpy(m_FileName, tmp);
    FSSpec fileSpec; FSMakeFSSpec(0,0,m_FileName,&fileSpec);

    if (noErr != EnterMovies()) {
        std::cerr << "QTMovieFile::Initialize(), EnterMovies() failed." << std::endl;
        exit(-1);
    }

    if (noErr != QTNewGWorldFromPtr (&m_gWorld,k32RGBAPixelFormat,&m_trackFrame,NULL,NULL,0,&_imageD[0],_w*4)) {
        std::cerr << "QTMovieFile::Initialize(), QTNewGWorldFromPtr() failed." << std::endl;
        exit(-1);
    }

    if (noErr != CreateMovieFile(&fileSpec,FOUR_CHAR_CODE('TVOD'),smCurrentScript,createMovieFileDeleteCurFile | createMovieFileDontCreateResFile,&m_resRefNum,&m_pMovie)) {
        std::cerr << "QTMovieFile::Initialize(), CreateMovieFile() failed." << std::endl;
        exit(-1);
    }
    // or CreateMovieStorage(dataRef,dataRefType, FOUR_CHAR_CODE('TVOD'),smCurrentScript,createMovieFileDeleteCurFile | createMovieFileDontCreateResFile,&m_dh,&m_pMovie)
    
    MatrixRecord m; SetIdentityMatrix(&m);
    ScaleMatrix(&m,Long2Fix(1),Long2Fix(-1),Long2Fix(1),Long2Fix(1));
    SetMovieMatrix (m_pMovie, &m);

    if (NULL == (m_pTrack=NewMovieTrack (m_pMovie, FixRatio(_w,1), FixRatio(_h,1), 0))) {
        std::cerr << "QTMovieFile::Initialize(), NewMovieTrack() failed." << std::endl;
        exit(-1);
    }

    if (NULL == (m_pMedia=NewTrackMedia (m_pTrack, VideoMediaType,kVideoTimeScale,nil,0))) {
        std::cerr << "QTMovieFile::Initialize(), NewTrackMedia() failed." << std::endl;
        exit(-1);
    }

    long maxCompressedSize;

    if (noErr != GetMaxCompressionSize(m_gWorld->portPixMap, &m_trackFrame, kMgrChoose, VIDEO_CODEC_QUALITY, VIDEO_CODEC_TYPE, (CompressorComponent)anyCodec, &maxCompressedSize)) {
        std::cerr << "QTMovieFile::Initialize(), GetMaxCompressionSize() failed." << std::endl;
        exit(-1);
    }

    if (NULL == (m_compressedData=NewHandle(maxCompressedSize))) {
        std::cerr << "QTMovieFile::Initialize(), NewHandle() failed." << std::endl;
        exit(-1);
    }

    MoveHHi(m_compressedData); HLock(m_compressedData);
    m_compressedDataPtr = StripAddress(*m_compressedData);

    if (NULL == (m_imageDesc=(ImageDescriptionHandle)NewHandle(4))) {
        std::cerr << "QTMovieFile::Initialize(), NewHandle() failed." << std::endl;
        exit(-1);
    }

    if (noErr != BeginMediaEdits (m_pMedia)) {
        std::cerr << "QTMovieFile::Initialize(), BeginMediaEdits() failed." << std::endl;
        exit(-1);
    }

    _initialized = true;
}

void CQTMovieFile::operator()(const osg::Camera& camera) const
{
    if (!_initialized) { const_cast<CQTMovieFile*>(this)->Initialize (camera); return; }

#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadPixels(_x,_y,_w,_h,GL_RGBA,GL_UNSIGNED_INT_8_8_8_8_REV,(GLvoid*)&_imageD.front());

    OSErr err=noErr;
    err = CompressImage(m_gWorld->portPixMap, 
                       &m_trackFrame, 
                        VIDEO_CODEC_QUALITY,
                        VIDEO_CODEC_TYPE,
                        m_imageDesc, 
                        m_compressedDataPtr );
    if(err!=noErr)	return;

    err = AddMediaSample(m_pMedia, 
                         m_compressedData,
                         kNoOffset,	        /* no offset in data */
                         (**m_imageDesc).dataSize, 
                         kSampleDuration,	/* frame duration = 1/10 sec */
                         (SampleDescriptionHandle)m_imageDesc, 
                         kAddOneVideoSample,	/* one sample */
                         kSyncSample,	    /* self-contained samples */
                         nil);
    if(err!=noErr)	return;
}


CQTMovieFile::~CQTMovieFile(void)
{
    if (!_initialized) return;

    if (m_pMedia) EndMediaEdits (m_pMedia);
    if (m_pTrack) InsertMediaIntoTrack (m_pTrack, kTrackStart,/* track start time */
                                        kMediaStart, /* media start time */
                                        GetMediaDuration (m_pMedia),
                                        fixed1);
    short resId = movieInDataForkResID;
    if (m_pMovie) AddMovieResource(m_pMovie,m_resRefNum, &resId,m_FileName);
    // or AddMovieToStorage(m_pMovie, m_dh);

    if (m_imageDesc)      DisposeHandle ((Handle)m_imageDesc);
    if (m_compressedData) DisposeHandle (m_compressedData);
    if (m_resRefNum)	  CloseMovieFile(m_resRefNum);
    if (m_pMovie)         DisposeMovie  (m_pMovie);
    if (m_savedPort)	  SetGWorld     (m_savedPort,m_savedGD);
    if (m_gWorld)         DisposeGWorld (m_gWorld);

    ExitMovies();
    TerminateQTML();
}
