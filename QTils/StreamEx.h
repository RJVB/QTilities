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
	typedef typename StreamType::char_type char_type;
	typedef StreamEx<StreamType> StreamEx_type;
private:
	typedef StreamType* StreamTypePtr;
	const StreamTypePtr parent;
	int lastLength;

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
		if( N == 0 ){
			N += 1;
		}
		char *buf = new char[N];
		while( buf && fmt ){
			*buf = '\0';
			n = ::vsnprintf( buf, N, fmt, ap );
			int len = strlen(buf);
			if( n >= 0 && len >= n ){
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

public:

	inline StreamEx_type& asnprintf( size_t N, const char *fmt, ... )
	{ va_list ap;
		va_start( ap, fmt );
		lastLength = vasnprintf( N, fmt, ap );
		va_end(ap);
		return *this;
	}

	inline StreamEx_type& asprintf( const char *fmt, ... )
	{ va_list ap;
		va_start( ap, fmt );
		lastLength = vasnprintf( (fmt)? strlen(fmt) : 256, fmt, ap );
		va_end(ap);
		return *this;
	}

// constructors
	StreamEx()
		: StreamType()
		, parent(NULL)
	{
		lastLength = 0;
	}
	//! create a StreamEx from a standard string
	StreamEx(const std::string &s)
		: StreamType(s)
		, parent(NULL)
	{
		lastLength = 0;
	}
	//! copy constructor
	StreamEx(StreamEx &p)
 		: StreamType((std::string)p.str())
		, parent(p.parent)
	{
		lastLength = 0;
	}
	//! creates a StreamEx of the StreamType variety and based on an existing StreamType instance
	//! This allows for instance to extend std::cout or std::cerr with StreamEx properties
	//! @n
	//! StreamEx<std::ostream> CErr(std::cerr);
	//! @n
	//! CErr << "This is a " << "test";
	//! @n
	//! CErr.asprintf( ", printing \"%%g\",exp(1)==%g to standard error", exp(1.0) ) << "\n";
	StreamEx(StreamType &s)
 		: StreamType(s.rdbuf())
		, parent(&s)
	{
		// copy the relevant data members from s (cf. http://stdcxx.apache.org/doc/stdlibug/34-2.html)
		StreamType::copyfmt(s);
		StreamType::clear(s.rdstate());
		StreamType::fill(s.fill());
		lastLength = 0;
	}

	StreamEx( size_t N, const char *fmt, ... )
		: StreamType()
	{ va_list ap;
		va_start( ap, fmt );
		lastLength = this->vasnprintf( N, fmt, ap );
		va_end(ap);
	}

	StreamEx( const char *fmt, ... )
		: StreamType()
		, parent(NULL)
	{ va_list ap;
		va_start( ap, fmt );
		lastLength = this->vasnprintf( (fmt)? strlen(fmt) : 256, fmt, ap );
		va_end(ap);
	}

	//! returns the length of the last generated formatted string
	int lastFormattedLength()
	{
		return lastLength;
	}

	//! defines a stream output operator that returns its input stream without modification. This allows to construct expressions like
	//! streamExInstance.asprintf( "string%d", 1) << ", string2, " << streamExInstance.asprintf( "and a final string%c", '\n' );
	template <typename CharT, typename Traits>
	friend std::basic_ostream<CharT, Traits>& operator <<(std::basic_ostream <CharT, Traits>& os, StreamEx_type& x)
	{
		return os;
	}
	
};

#define _STREAMEX_H
#endif