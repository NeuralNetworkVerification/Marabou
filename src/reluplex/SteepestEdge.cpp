#include "SteepestEdge.h"
#include "FloatUtils.h"
#include "ITableau.h"
#include "ReluplexError.h"

bool SteepestEdge::select( ITableau &tableau )
{
    /* TODO
    tableau.computeCostFunction();

    List<unsigned> candidates;
    tableau.getCandidates(candidates);

    if ( candidates.empty() )
        return false;

    // Dantzig's rule
    const double *costFunction = tableau.getCostFunction();

    List<unsigned>::const_iterator candidate = candidates.begin();
    unsigned maxIndex = *candidate;
    double maxValue = FloatUtils::abs( costFunction[maxIndex] );
    ++candidate;

    while ( candidate != candidates.end() )
    {
        double contenderValue = FloatUtils::abs( costFunction[*candidate] );
        if ( FloatUtils::gt( contenderValue, maxValue ) )
        {
            maxIndex = *candidate;
            maxValue = contenderValue;
        }
        ++candidate;
    }

    tableau.setEnteringVariable(maxIndex);
    return true;
    */
}

void SteepestEdge::initialize( const ITableau & /* tableau */ )
{}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
