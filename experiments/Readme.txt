Property 01:
---

Network: tiny4, 5-50-50-50-5 (150 ReLUs)

Input property:
        inputQuery.setUpperBound( acasParser.getInputVariable( 0 ), 0.0 );
        inputQuery.setUpperBound( acasParser.getInputVariable( 1 ), 0.0 );
        inputQuery.setLowerBound( acasParser.getInputVariable( 2 ), 0.0 );
        inputQuery.setUpperBound( acasParser.getInputVariable( 3 ), 0.0 );
        inputQuery.setUpperBound( acasParser.getInputVariable( 4 ), 0.0 );

Output property:
        unsigned variable = acasParser.getOutputVariable( 0 );
        inputQuery.setLowerBound( variable, 0.0 );

This property is SAT. Normal run takes about 40 minutes.
