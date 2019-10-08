/*********************                                                        */
/*! \file MString.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "MString.h"
#include <vector>

String::String( Super super ) : _super( super )
{
}

String::String()
{
}

String::String( const char *string, unsigned length ) : _super( string, length )
{
}

String::String( const char *string ) : _super( string )
{
}

unsigned String::length() const
{
    return _super.length();
}

const char *String::ascii() const
{
    return _super.c_str();
}

char String::operator[]( int index ) const
{
    return _super[index];
}

char &String::operator[]( int index )
{
    return _super[index];
}

String &String::operator=( const char *string )
{
    *this = String( string );
    return *this;
}

String &String::operator=( const String &other )
{
    _super = other._super;
    return *this;
}

bool String::operator==( const char *string ) const
{
    return _super.compare( string ) == 0;
}

String String::operator+( const String &other ) const
{
    return _super + other._super;
}

String String::operator+( const char *other ) const
{
    return _super + other;
}

String String::operator+=( const String &other )
{
    return _super += other._super;
}

String String::operator+=( const char *other )
{
    return _super += other;
}

bool String::operator==( const String &other ) const
{
    return _super.compare( other._super ) == 0;
}

bool String::operator!=( const String &other ) const
{
    return _super.compare( other._super ) != 0;
}

bool String::operator<( const String &other ) const
{
    return _super < other._super;
}

List<String> String::tokenize( String delimiter ) const
{
    List<String> tokens;

    std::vector<char> c(length() + 1);
    char* copy(c.data());
    memcpy( copy, ascii(), sizeof(char)*(length()+1) );

    char *token = strtok( copy, delimiter.ascii() );

    while ( token != NULL )
    {
        tokens.append( String( token ) );
        token = strtok( NULL, delimiter.ascii() );
    }

    return tokens;
}

bool String::contains( const String &substring ) const
{
    return find( substring ) != Super::npos;
}

size_t String::find( const String &substring ) const
{
    return _super.find( substring._super );
}

String String::substring( unsigned fromIndex, unsigned howMany ) const
{
    return _super.substr( fromIndex, howMany );
}

void String::replace( const String &toReplace, const String &replaceWith )
{
    if ( find( toReplace ) != Super::npos )
    {
        _super.replace( find( toReplace ), toReplace.length(), replaceWith._super );
    }
}

String String::trim() const
{
    unsigned firstNonSpace = 0;
    unsigned lastNonSpace = 0;

    bool firstNonSpaceFound = false;

    for ( unsigned i = 0; i < length(); ++i )
    {
        if ( ( !firstNonSpaceFound ) && ( _super[i] != ' ' ) )
        {
            firstNonSpace = i;
            firstNonSpaceFound = true;
        }

        if ( ( _super[i] != ' ' ) && ( _super[i] != '\n' ) )
        {
            lastNonSpace = i;
        }
    }

    if ( lastNonSpace == 0 )
    {
        return "";
    }

    return substring( firstNonSpace, lastNonSpace - firstNonSpace + 1 );
}

void String::replaceAll( const String &toReplace, const String &replaceWith )
{
    while ( find( toReplace ) != Super::npos )
        _super.replace( find( toReplace ), toReplace.length(), replaceWith._super );
}

std::ostream &operator<<( std::ostream &stream, const String &string );

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
