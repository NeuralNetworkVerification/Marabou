/*********************                                                        */
/*! \file String.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluwise project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __String_h__
#define __String_h__

#include "List.h"
#include <cstring>
#include <string>

class String
{
public:
	typedef std::string Super;

	String( Super super );
	String();
    String( const char *string, unsigned length );
	String( const char *string );

	unsigned length() const;
    const char *ascii() const;
    char operator[]( int index ) const;
    char &operator[]( int index );
    String &operator=( const char *string );
    String &operator=( const String &other );
    bool operator==( const char *string ) const;
    String operator+( const String &other ) const;
    String operator+( const char *other ) const;
    String operator+=( const String &other );
    String operator+=( const char *other );
    bool operator==( const String &other ) const;
    bool operator!=( const String &other ) const;
    bool operator<( const String &other ) const;
    List<String> tokenize( String delimiter ) const;
    bool contains( const String &substring ) const;
    size_t find( const String &substring ) const;
    String substring( unsigned fromIndex, unsigned howMany ) const;
    void replace( const String &toReplace, const String &replaceWith );
    String trim() const;
    void replaceAll( const String &toReplace, const String &replaceWith );

protected:
	Super _super;
};

std::ostream &operator<<( std::ostream &stream, const String &string );

#ifdef CXXTEST_RUNNING
#include <cxxtest/ValueTraits.h>
#include <stdio.h>
namespace CxxTest
{
    CXXTEST_TEMPLATE_INSTANTIATION
    class ValueTraits<String>
    {
    public:
        ValueTraits( const String &string ) : _string( string )
        {
        }

        const char *asString() const
        {
            return _string.ascii();
        }

    private:
        const String &_string;
    };
}

#endif // CXXTEST_RUNNING

#endif // __String_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
