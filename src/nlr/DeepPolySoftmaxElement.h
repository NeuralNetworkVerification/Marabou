/*********************                                                        */
/*! \file DeepPolySoftmaxElement.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __DeepPolySoftmaxElement_h__
#define __DeepPolySoftmaxElement_h__

#include "DeepPolyElement.h"
#include "Layer.h"
#include "MStringf.h"
#include "NLRError.h"
#include <climits>

namespace NLR {

class DeepPolySoftmaxElement : public DeepPolyElement
{
public:

    DeepPolySoftmaxElement( Layer *layer );
    ~DeepPolySoftmaxElement();

    void execute( const Map<unsigned, DeepPolyElement *> &deepPolyElements );
    void symbolicBoundInTermsOfPredecessor
    ( const double *symbolicLb, const double*symbolicUb, double
      *symbolicLowerBias, double *symbolicUpperBias, double
      *symbolicLbInTermsOfPredecessor, double *symbolicUbInTermsOfPredecessor,
      unsigned targetLayerSize, DeepPolyElement *predecessor );

private:

    /*
      Memory allocated to store concrete bounds computed at different stages
      of back substitution.
    */
    double *_workLb;
    double *_workUb;

    Set<unsigned>  _residualLayerIndices;
    Map<unsigned, double *>  _residualLb;
    Map<unsigned, double *>  _residualUb;

    /*
      Compute the concrete upper- and lower- bounds of this layer by concretizing
      the symbolic bounds with respect to every preceding element.
    */
    void computeBoundWithBackSubstitution( const Map<unsigned, DeepPolyElement *>
                                           &deepPolyElementsBefore );

    /*
      Compute concrete bounds using symbolic bounds with respect to a
      sourceElement.
    */
    void concretizeSymbolicBound( const double *symbolicLb, const double
                                  *symbolicUb, const double *symbolicLowerBias,
                                  const double *symbolicUpperBias,
                                  DeepPolyElement *sourceElement,
                                  const Map<unsigned, DeepPolyElement *>
                                  &deepPolyElementsBefore );

    void concretizeSymbolicBoundForSourceLayer( const double *symbolicLb,
                                                const double*symbolicUb,
                                                const double *symbolicLowerBias,
                                                const double *symbolicUpperBias,
                                                DeepPolyElement *sourceElement );

    void allocateMemoryForResidualsIfNeeded( unsigned residualLayerIndex,
                                             unsigned residualLayerSize );
    void allocateMemory();
    void freeMemoryIfNeeded();
    void log( const String &message );

  double L_LSE1( const Vector<double> &input,
                        const Vector<double> &inputLb,
                        const Vector<double> &inputUb,
                        unsigned i );
  double dL_LSE1dx( const Vector<double> &c,
               const Vector<double> &inputLb,
               const Vector<double> &inputUb,
               unsigned i, unsigned di);
  double L_LSE2( const Vector<double> &input,
                 const Vector<double> &inputLb,
                 const Vector<double> &inputUb,
                 unsigned i );
  double dL_LSE2dx( const Vector<double> &c,
                    const Vector<double> &inputLb,
                    const Vector<double> &inputUb,
                    unsigned i, unsigned di);
  double L_ER( const Vector<double> &input,
                 const Vector<double> &inputLb,
                 const Vector<double> &inputUb,
                 unsigned i );
  double dL_ERdx( const Vector<double> &c,
                    const Vector<double> &inputLb,
                    const Vector<double> &inputUb,
                    unsigned i, unsigned di);
  double U_LSE( const Vector<double> &input, unsigned i );
  double dU_LSEdx( const Vector<double> &c,
               unsigned i, unsigned di);
  double U_ER( const Vector<double> &input, unsigned i );
  double dU_ERdx( const Vector<double> &c,
                   unsigned i, unsigned di);
  double L_Linear( const Vector<double> &inputLb,
                   const Vector<double> &inputUb,
                   unsigned i );
  double U_Linear( const Vector<double> &inputLb,
                   const Vector<double> &inputUb,
                   unsigned i );
};

} // namespace NLR

#endif // __DeepPolySoftmaxElement_h__
