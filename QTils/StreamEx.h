/*!
 *  @file StreamEx.h
 *  QTMovieSink-106
 *
 *  Created by René J.V. Bertin on 20131021.
 *  Copyright 2013 RJVB. All rights reserved.
 *
 */

#ifndef _STREAMEX_H

#include <stdarg.h>
#include <string>
#include <sstream>

/*!
	a template class extending e.g. std::stringstream with printf formatting features
 */
template <typename StreamType>
class StreamEx : public StreamType {
public:

	inline int vasnprintf( size_t N, const char *fmt, va_list ap )
	{ int n = 0;
#if (defined(__APPLE_CC__) && defined(__MACH__)) || defined(linux)
		char *buf = NULL;
		if( fmt ){
			n = ::vasprintf( &buf, fmt, ap );
			if( buf && n >= 0 ){
				*this << buf;
				free(buf);
				return n;
			}
		}
#else
		char *buf = new char[N];
		while( buf && fmt ){
			n = ::vsnprintf( buf, N, fmt, ap );
			int len = strlen(buf);
			if( n > 0 && len == n ){
				*this << buf;
				delete[] buf;
				return n;
			}
			else{
				N *= 2;
				delete[] buf;
				buf = new char[N];
			}
		}
#endif
		return n;
	}

	inline int asnprintf( size_t N, const char *fmt, ... )
	{ va_list ap;
	  int n;
		va_start( ap, fmt );
		n = vasnprintf( N, fmt, ap );
		va_end(ap);
		return n;
	}

	inline int asprintf( const char *fmt, ... )
	{ va_list ap;
	  int n;
		va_start( ap, fmt );
		n = vasnprintf( (fmt)? strlen(fmt) : 256, fmt, ap );
		va_end(ap);
		return n;
	}

// constructors
	StreamEx()
		: StreamType()
	{}
	StreamEx(const std::string &s)
		: StreamType(s)
	{}
	StreamEx(StreamEx &p)
 		: StreamType((std::string)p.str())
	{}
	StreamEx( size_t N, const char *fmt, ... )
		: StreamType()
	{ va_list ap;
		va_start( ap, fmt );
		this->vasnprintf( N, fmt, ap );
		va_end(ap);
	}
	StreamEx( const char *fmt, ... )
		: StreamType()
	{ va_list ap;
		va_start( ap, fmt );
		this->vasnprintf( (fmt)? strlen(fmt) : 256, fmt, ap );
		va_end(ap);
	}
};

#define _STREAMEX_H
#endif