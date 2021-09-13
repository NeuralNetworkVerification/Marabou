/*********************                                                        */
/*! \file ConstraintBoundTightener.h
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

#ifndef __ConstraintBoundTightener_h__
#define __ConstraintBoundTightener_h__

#include <map>
#include "IConstraintBoundTightener.h"

class ConstraintBoundTightener : public IConstraintBoundTightener
{
public:
    ConstraintBoundTightener( ITableau &tableau, IEngine &engine );
    ~ConstraintBoundTightener();

    /*
      Allocate internal work memory according to the tableau size.
    */
    void setDimensions();

    /*
      Initialize tightest lower/upper bounds using the talbeau.
    */
    void resetBounds();

    /*
      Callback from the Tableau, to inform of a change in dimensions
    */
    void notifyDimensionChange( unsigned m, unsigned n );

    /*
      Callbacks from the Tableau, to inform of bounds tightened by,
      e.g., the PL constraints.
    */
    void notifyLowerBound( unsigned variable, double bound );
    void notifyUpperBound( unsigned variable, double bound );

    /*
      Have the Bound Tightener start reporting statistics.
    */
    void setStatistics( Statistics *statistics );

    /*
      This method can be used by clients to tell the bound tightener
      about a tighter bound
    */
    void registerTighterLowerBound( unsigned variable, double bound );
    void registerTighterUpperBound( unsigned variable, double bound );

	/*
 	As previous methods, but with additional Tableau row for explaining the bound tightening.
	*/
	void registerTighterLowerBound( unsigned variable, double bound, const SparseUnsortedList& row );
	void registerTighterUpperBound( unsigned variable, double bound, const SparseUnsortedList& row );

    /*
      Get the tightenings previously registered by the constraints
    */
    void getConstraintTightenings( List<Tightening> &tightenings ) const;


	std::map<unsigned , double> getUGBUpdates() const;
	std::map<unsigned, double> getLGBUpdates() const;
	std::list<std::vector<double>> getTableauUpdates() const;
	void clearEngineUpdates();

	/*
	 * Replaces the indicating row by equation which is added to the Tableau
	 */
	void externalExplanationUpdate( unsigned var, double value, bool isUpper );


private:
    ITableau &_tableau;
    unsigned _n;
    unsigned _m;

    /*
      Work space for the tightener to derive tighter bounds. These
      represent the tightest bounds currently known, either taken
      from the tableau or derived by the tightener. The flags indicate
      whether each bound has been tightened by the tightener.
    */
    double *_lowerBounds;
    double *_upperBounds;
    bool *_tightenedLower;
    bool *_tightenedUpper;

    /*
      Statistics collection
    */
    Statistics *_statistics;

    /*
      Free internal work memory.
    */
    void freeMemoryIfNeeded();

	std::map<unsigned, double> _lowerGBUpdates;
	std::map<unsigned, double> _upperGBUpdates;

	IEngine &_engine; // TODO Consider design
};

#endif // __ConstraintBoundTightener_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
