/*********************                                                        */
/*! \file MockTableau.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Reluplex project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __MockTableau_h__
#define __MockTableau_h__

class MockTableau : public ITableau
{
public:
	MockTableau()
	{
		wasCreated = false;
		wasDiscarded = false;
	}

	bool wasCreated;
	bool wasDiscarded;

	void mockConstructor()
	{
		TS_ASSERT( !wasCreated );
		wasCreated = true;
	}

	void mockDestructor()
	{
		TS_ASSERT( wasCreated );
		TS_ASSERT( !wasDiscarded );
		wasDiscarded = true;
	}


    void setDimensions( unsigned /* m */, unsigned /* n */ ) {}
    void setEntryValue( unsigned /* row */, unsigned /* column */, double /* value */ ) {}
    void setRightHandSide( const double */* b */ ) {}
    void initializeBasis( const Set<unsigned> &/* basicVariables */ ) {}
    double getValue( unsigned /* variable */ ) { return 0; }
    void setLowerBound( unsigned /* variable */, double /* value */ ) {}
    void setUpperBound( unsigned /* variable */, double /* value */ ) {}
    unsigned getBasicStatus( unsigned /* basic */ ) { return 0; }
    bool existsBasicOutOfBounds() const { return false; }
    void computeBasicStatus() {}
    void computeBasicStatus( unsigned /* basic */ ) {}
    bool pickEnteringVariable() { return false; }
    bool eligibleForEntry( unsigned /* nonBasic */ ) { return false; }
    unsigned getEnteringVariable() const { return 0; }
    void pickLeavingVariable() {};
    void pickLeavingVariable( double */* d */ ) {}
    unsigned getLeavingVariable() const { return 0; }
    double getChangeRatio() const { return 0; }
    void performPivot() {}
    double ratioConstraintPerBasic( unsigned /* basicIndex */, double /* coefficient */, bool /* decrease */ ) { return 0;}
    bool isBasic( unsigned /* variable */ ) const { return false; }
    void computeCostFunction() {}
    const double *getCostFunction() { return 0; }
    void computeD() {}
    void computeAssignment() {}
    void dump() const {}
};

#endif // __MockTableau_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
