#include "Preprocessor.h"
#include "MStringf.h"
#include "ReluplexError.h"
#include "FloatUtils.h"
#include "InputQuery.h"

#include <cfloat>

Preprocessor::Preprocessor()
{
}

void Preprocessor::tightenBounds( InputQuery &input )
{
	double min = -DBL_MAX;
    double max = DBL_MAX;

    for ( auto equation : input.getEquations() )
    {
        bool unbounded = false;
        for ( auto addend : equation._addends )
        {
                if ( input.getLowerBound( addend._variable ) == min || input.getUpperBound( addend._variable ) == max )
                {
                    //cannot tighten bounds if there are 2 variables that are unbounded on both sides
                    if ( input.getLowerBound( addend._variable ) == min && input.getUpperBound( addend._variable ) == max )
                    {
                        if ( unbounded )
                            break;
                        unbounded = true;
                    }
                    //if left-hand side coefficient is negative, [LB, UB] = [-UB, -LB]
                    bool pos =  FloatUtils::isPositive( addend._coefficient );

                    double scalarUB = equation._scalar;
                    double scalarLB = equation._scalar;

                    for ( auto bounded : equation._addends )
                    {
                        if ( addend._variable == bounded._variable ) continue;

                        if ( FloatUtils::isNegative( bounded._coefficient ) )
                        {
                            //coefficient is negative--subtract to simulate addition
                            scalarLB -= bounded._coefficient * input.getLowerBound( bounded._variable );
                            scalarUB -= bounded._coefficient * input.getUpperBound( bounded._variable );
                        }
                        if ( FloatUtils::isPositive( bounded._coefficient ) )
                        {
                            scalarLB -= bounded._coefficient * input.getUpperBound( bounded._variable );
                            scalarUB -= bounded._coefficient * input.getLowerBound( bounded._variable );
                        }
                    }

                    if ( !pos )
                    {
                        double temp = scalarUB;
                        scalarUB = -scalarLB;
                        scalarLB = -temp;
                    }

                    //divide by coefficient
                    if ( FloatUtils::gt( scalarLB / abs( addend._coefficient ), input.getLowerBound( addend._variable ) ) )
                            input.setLowerBound( addend._variable, scalarLB / abs( addend._coefficient ) );
                    if ( FloatUtils::lt( scalarUB / abs (addend._coefficient ), input.getUpperBound( addend._variable ) ) )
                            input.setUpperBound( addend._variable, scalarUB / abs ( addend._coefficient ) );
            }

            if ( FloatUtils::gt( input.getLowerBound( addend._variable), input.getUpperBound( addend._variable) ) )
                throw ReluplexError( ReluplexError::INVALID_BOUND_TIGHTENING, "preprocessing bound error" );

        }
    }
}
//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//

