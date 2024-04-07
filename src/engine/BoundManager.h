/*********************                                                        */
/*! \file BoundManager.h
 ** \verbatim
 ** Top contributors (to current version):
 **  Aleksandar Zeljic, Haoze Wu,
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** BoundManager class is a context-dependent implementation of a centralized
 ** variable registry and their bounds. The intent it so use a single
 ** BoundManager object between multiple bound tightener classes, which enables
 ** those classes to care only about bounds and forget about book-keeping.
 **
 ** BoundManager provides a method to obtain a new variable with:
 ** registerNewVariable().
 **
 ** The bound values are stored locally and copied over to and from
 ** context-dependent objects, which backtrack automatically with the central
 ** _context object. The pointers to local bounds are provided to the Tableau
 ** for efficiency of read operations.
 **
 ** There are two sets of methods to set bounds:
 **   * set*Bounds     - local method used to update bounds
 **   * tighten*Bounds - shared method to update bounds, propagates the new bounds
 **                        to the _tableau (if registered) to keep the assignment and
 **                      basic/non-basic variables updated accordingly.
 **
 ** When bounds become inconsistent, i.e. lowerBound > upperBound,
 ** _consistentBounds flag is set and the bound update that yielded
 ** inconsistency is stored for conflict analysis purposes.
 **
 ** It is assumed that variables are not introduced on the fly, and as such
 ** interaction with context-dependent features is not implemented.
 **/

#ifndef __BoundManager_h__
#define __BoundManager_h__

#include "IBoundManager.h"
#include "IEngine.h"
#include "IRowBoundTightener.h"
#include "ITableau.h"
#include "List.h"
#include "PlcLemma.h"
#include "Tightening.h"
#include "UnsatCertificateNode.h"
#include "Vector.h"
#include "context/cdo.h"
#include "context/context.h"

class ITableau;
class IEngine;
class BoundManager : public IBoundManager
{
public:
    BoundManager( CVC4::context::Context &ctx );
    ~BoundManager();

    /*
       Initialize BoundManager and register numberOfVariables of variables
     */
    void initialize( unsigned numberOfVariables );

    /*
       Registers a new variable, grows the BoundManager size and bound vectors,
       initializes new bounds to +/-inf, and returns the index of the new
       variable.
     */
    unsigned registerNewVariable();

    /*
       Returns number of registered variables
     */
    unsigned getNumberOfVariables() const;

    /*
       Communicates bounds to the bound Manager and informs _tableau of the
       changes, so that any necessary updates can be performed.
     */
    bool tightenLowerBound( unsigned variable, double value );
    bool tightenUpperBound( unsigned variable, double value );

    /*
       Silently sets bounds to the assigned value, checks bound consistency.
     */
    bool setLowerBound( unsigned variable, double value );
    bool setUpperBound( unsigned variable, double value );

    /*
       Return current bound value.
     */
    double getLowerBound( unsigned variable ) const;
    double getUpperBound( unsigned variable ) const;

    /*
      Get pointers to latest bounds used for access by tableau and tighteners
     */
    const double *getLowerBounds() const;
    const double *getUpperBounds() const;

    /*
       Store and restore local bounds after context advances/backtracks.
     */
    void storeLocalBounds();
    void restoreLocalBounds();

    /*
       Obtain a list of all the bound updates since the last call to
       getTightenings or clearTightenings or propagateTighetings.
     */
    void getTightenings( List<Tightening> &tightenings );

    /*
       Clear tightened flags.
     */
    void clearTightenings();

    /*
       Inform variable watchers of new tightenings.
     */
    void propagateTightenings();

    /*
      Returns true if the bounds of all variables are valid. Returns false in a conflict state.
    */
    bool consistentBounds() const;

    /*
      Returns true if the bounds for the variable is valid, used to
      detect a conflict state.
    */
    bool consistentBounds( unsigned variable ) const;

    /*
       Register Tableau pointer for callbacks from tighten*Bound methods.
     */
    void registerTableau( ITableau *tableau );

    /*
       Register RowBoundTightener for updates to local bound pointers.
     */
    void registerRowBoundTightener( IRowBoundTightener *ptrRowBoundTightener );

    /*
      Return the content of the object containing all explanations for variable bounds in the
      tableau.
    */
    const BoundExplainer *getBoundExplainer() const;

    /*
      Deep-copy the BoundExplainer object content
     */
    void copyBoundExplainerContent( const BoundExplainer *boundsExplainer );

    /*
      Initialize the boundExplainer
     */
    void initializeBoundExplainer( unsigned numberOfVariables, unsigned numberOfRows );

    /*
      Given a row, updates the values of the bound explanations of a var according to the row
    */
    void updateBoundExplanation( const TableauRow &row, bool isUpper, unsigned var );

    /*
      Given a row as SparseUnsortedList, updates the values of the bound explanations of a var
      according to the row
    */
    void updateBoundExplanationSparse( const SparseUnsortedList &row, bool isUpper, unsigned var );

    /*
      Reset a bound explanation
    */
    void resetExplanation( unsigned var, bool isUpper ) const;

    /*
      Return the bounds explanation of a variable in the tableau to the argument vector
    */
    const SparseUnsortedList &getExplanation( unsigned variable, bool isUpper ) const;

    /*
      Artificially update an explanation, without using the recursive rule
    */
    void setExplanation( const SparseUnsortedList &explanation, unsigned var, bool isUpper ) const;

    /*
      Register Engine pointer for callbacks
    */
    void registerEngine( IEngine *engine );

    /*
      Get the index of a variable with inconsistent bounds, if exists, or -1 otherwise
    */
    unsigned getInconsistentVariable() const;

    /*
      Computes the bound imposed by a row's rhs on its lhs
    */
    double computeRowBound( const TableauRow &row, bool isUpper ) const;

    /*
      Computes the bound imposed by a row on a variable
    */
    double computeSparseRowBound( const SparseUnsortedList &row, bool isUpper, unsigned var ) const;

    /*
      Return true iff an explanation is trivial (i.e. the zero vector)
    */
    bool isExplanationTrivial( unsigned var, bool isUpper ) const;

    /*
      Return true iff boundManager should produce proofs
    */
    bool shouldProduceProofs() const;

private:
    CVC4::context::Context &_context;
    unsigned _size;
    unsigned _allocated;
    ITableau *_tableau;                     // Used only by callbacks
    IRowBoundTightener *_rowBoundTightener; // Used only by callbacks
    IEngine *_engine;

    CVC4::context::CDO<bool> _consistentBounds;
    Tightening _firstInconsistentTightening;

    double *_lowerBounds;
    double *_upperBounds;

    Vector<CVC4::context::CDO<double> *> _storedLowerBounds;
    Vector<CVC4::context::CDO<double> *> _storedUpperBounds;

    Vector<CVC4::context::CDO<bool> *> _tightenedLower;
    Vector<CVC4::context::CDO<bool> *> _tightenedUpper;

    /*
       Record first tightening that violates bounds
     */
    void recordInconsistentBound( unsigned variable, double value, Tightening::BoundType type );

    void allocateLocalBounds( unsigned size );

    /*
      Tighten bounds and update their explanations according to some object representing the row
     */
    bool tightenLowerBound( unsigned variable, double value, const TableauRow &row );
    bool tightenUpperBound( unsigned variable, double value, const TableauRow &row );

    bool tightenLowerBound( unsigned variable, double value, const SparseUnsortedList &row );
    bool tightenUpperBound( unsigned variable, double value, const SparseUnsortedList &row );

    /*
      Adds a lemma to the UNSATCertificateNode object
     */
    bool addLemmaExplanationAndTightenBound( unsigned var,
                                             double value,
                                             Tightening::BoundType affectedVarBound,
                                             const List<unsigned> &causingVars,
                                             Tightening::BoundType causingVarBound,
                                             PiecewiseLinearFunctionType constraintType );

    /*
      Explainer of all bounds
    */
    BoundExplainer *_boundExplainer;
};

#endif // __BoundManager_h__
