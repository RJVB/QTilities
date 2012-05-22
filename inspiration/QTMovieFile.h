#pragma once

#ifndef __QTMOVIEFILE_H
#define __QTMOVIEFILE_H

#include <osg/Camera>

#include <qtml.h>
#include <movies.h>
#include <tchar.h>
#include <Gestalt.h>
#include <wtypes.h>
#include <FixMath.h>
#include <Script.h>
#include <stdlib.h>
#include <TextUtils.h>
#include <NumberFormatting.h>

#ifdef __cplusplus

struct CQTMovieFile : public osg::Camera::DrawCallback
{
    CQTMovieFile(const std::string& filename);
   ~CQTMovieFile(void);

    virtual void operator () (const osg::Camera&) const;

protected:
    void Initialize(const osg::Camera& camera);

protected:
    CGrafPtr	m_savedPort;
    GDHandle	m_savedGD;
    GWorldPtr	m_gWorld;
    Track		m_pTrack;
    Media		m_pMedia;
    Movie		m_pMovie;
    Str255		m_FileName;
    short		m_resRefNum;
    Rect		m_trackFrame ;
    Handle		m_compressedData;
    Ptr			m_compressedDataPtr;
    ImageDescriptionHandle m_imageDesc;

protected:
    bool _initialized;
    int _x,_y; unsigned _w,_h; 
    std::string _filename;
    std::vector<RGBQUAD> _imageD;
};

#endif

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif


#endif 
