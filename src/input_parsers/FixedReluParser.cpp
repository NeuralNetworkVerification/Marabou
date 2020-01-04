/*********************                                                        */
/*! \file FixedRluParser.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "File.h"
#include "MStringf.h"
#include "Map.h"
#include "FixedReluParser.h"
#include "InputParserError.h"

void FixedReluParser::parse( const String &fixedReluFilePath, Map<unsigned, unsigned> &fixedRelus )
{
    if ( !File::exists( fixedReluFilePath ) )
    {
        printf( "Error: the fixed relu file (%s) doesn't exist!\n", fixedReluFilePath.ascii() );
        throw InputParserError( InputParserError::FILE_DOESNT_EXIST, fixedReluFilePath.ascii() );
    }

    File fixedReluFile( fixedReluFilePath );
    fixedReluFile.open( File::MODE_READ );

    try
    {
        while ( true )
        {
            String line = fixedReluFile.readLine().trim();
            List<String> tokens = line.tokenize( " " );
            unsigned index = 0;
            unsigned id = 0;
            unsigned phase = 0;
            for ( const auto token : tokens )
            {
                if ( index == 0 )
                    id = atoi( token.ascii() );
                else
                    phase = atoi( token.ascii() );
                ++index;
            }
            fixedRelus[id] = phase;
        }
    }
    catch ( const CommonError &e )
    {
        // A "READ_FAILED" is how we know we're out of lines
        if ( e.getCode() != CommonError::READ_FAILED )
            throw e;
    }
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
