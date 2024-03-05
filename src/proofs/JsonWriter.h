/*********************                                                        */
/*! \file JsonWriter.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Omri Isac, Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2023 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]
 **/

#ifndef __JsonWriter_h__
#define __JsonWriter_h__

#include "Contradiction.h"
#include "File.h"
#include "List.h"
#include "MString.h"
#include "PiecewiseLinearConstraint.h"
#include "PlcLemma.h"
#include "SparseUnsortedList.h"
#include "Tightening.h"
#include "UnsatCertificateNode.h"
#include "Vector.h"

#include <iomanip>

/*
  A class responsible for writing Marabou proof instances into JSON format
*/
class JsonWriter
{
public:
    /*
      Write an entire UNSAT proof to a JSON file.
      General remark - empty JSON properties (such as empty contradiction for non-leaves) will not
      be written.
    */
    static void writeProofToJson( const UnsatCertificateNode *root,
                                  unsigned explanationSize,
                                  const SparseMatrix *initialTableau,
                                  const Vector<double> &upperBounds,
                                  const Vector<double> &lowerBounds,
                                  const List<PiecewiseLinearConstraint *> &problemConstraints,
                                  IFile &file );
    /*
      Configure whether lemmas should be written proved as well
    */
    static const bool PROVE_LEMMAS;

    /*
      Configure writer precision
    */
    static const unsigned JSONWRITER_PRECISION;

    /*
      Configure proof file name
    */
    static const char PROOF_FILENAME[];

    /*
       JSON property names
    */
    static const char AFFECTED_VAR[];
    static const char AFFECTED_BOUND[];
    static const char BOUND[];
    static const char CAUSING_VAR[];
    static const char CAUSING_VARS[];
    static const char CAUSING_BOUND[];
    static const char CONTRADICTION[];
    static const char CONSTRAINT[];
    static const char CONSTRAINTS[];
    static const char CONSTRAINT_TYPE[];
    static const char CHILDREN[];
    static const char EXPLANATION[];
    static const char EXPLANATIONS[];
    static const char LEMMAS[];
    static const char LOWER_BOUND[];
    static const char LOWER_BOUNDS[];
    static const char PROOF[];
    static const char SPLIT[];
    static const char TABLEAU[];
    static const char UPPER_BOUND[];
    static const char UPPER_BOUNDS[];
    static const char VALUE[];
    static const char VARIABLE[];
    static const char VARIABLES[];

private:
    /*
      Write the initial tableau to a JSON list of Strings
    */
    static void writeInitialTableau( const SparseMatrix *initialTableau,
                                     unsigned explanationSize,
                                     List<String> &instance );

    /*
      Write variables bounds to a JSON String
    */
    static void writeBounds( const Vector<double> &bounds,
                             Tightening::BoundType isUpper,
                             List<String> &instance );

    /*
      Write a list a piecewise-linear constraints to a JSON String
    */
    static void
    writePiecewiseLinearConstraints( const List<PiecewiseLinearConstraint *> &problemConstraints,
                                     List<String> &instance );

    /*
      Write an UNSAT certificate node to a JSON String
    */
    static void writeUnsatCertificateNode( const UnsatCertificateNode *node,
                                           unsigned explanationSize,
                                           List<String> &instance );

    /*
      Write a list of PLCLemmas to a JSON String
    */
    static void writePLCLemmas( const List<std::shared_ptr<PLCLemma>> &PLCLemma,
                                List<String> &instance );

    /*
      Write a contradiction object to a JSON String
    */
    static void writeContradiction( const Contradiction *contradiction, List<String> &instance );

    /*
      Write a PiecewiseLinearCaseSplit to a JSON String
    */
    static void writeHeadSplit( const PiecewiseLinearCaseSplit &headSplit, List<String> &instance );

    /*
      Write an instance to a file
    */
    static void writeInstanceToFile( IFile &file, const List<String> &instance );

    /*
      Convert a double to a string
    */
    static String convertDoubleToString( double value );

    /*
      Write an array of doubles to a JSON String
    */
    static String convertDoubleArrayToString( const double *arr, unsigned size );

    /*
      Write a SparseUnsortedList object to a JSON String
    */
    static String convertSparseUnsortedListToString( SparseUnsortedList sparseList );
};

#endif //__JsonWriter_h__