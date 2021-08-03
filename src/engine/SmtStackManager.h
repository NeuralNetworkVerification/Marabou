#ifndef __SMTSTACKMANAGER_H__
#define __SMTSTACKMANAGER_H__

#include "PiecewiseLinearCaseSplit.h"
#include "PiecewiseLinearConstraint.h"
#include "SmtState.h"
#include "Stack.h"
#include "SmtStackEntry.h"
#include "Statistics.h"
#include "ISmtSplitProvider.h"

#include <memory>

#define SMT_LOG( x, ... ) LOG( GlobalConfiguration::SMT_CORE_LOGGING, "SmtCore: %s\n", x )

using SmtStack = List<SmtStackEntry*>;
using SplitProviders = List<std::shared_ptr<ISmtSplitProvider>>;

class SmtStackManager
{
public:
   SmtStackManager( IEngine* engine );
  ~SmtStackManager() = default;

  /*
    Reset the SmtStackManager
  */
  void reset();

  /*
    Perform the split according to the constraint marked for
    splitting. Update bounds, add equations and update the stack.
  */
  void performSplit( PiecewiseLinearCaseSplit const& split );

  /*
    Pop an old split from the stack.
    Return true if successful, false if the stack is empty.
  */
  bool popSplit();

  /*
    The current stack depth.
  */
  unsigned getStackDepth() const;

  /*
    The current stack.
  */
  SmtStack const& getStack() const;

  /*
    Split Provider subscription
  */
  bool subscribeSplitProvider( std::shared_ptr<ISmtSplitProvider> provider );
  bool ubsubscribeSplitProvider( std::shared_ptr<ISmtSplitProvider> provider );
  SplitProviders const& splitProviders() const;

  /*
    Have the SMT core start reporting statistics.
  */
  void setStatistics( Statistics* statistics );

  /*
    For debugging purposes only - store a correct possible solution
  */
  void storeDebuggingSolution( const Map<unsigned, double>& debuggingSolution );
  bool checkSkewFromDebuggingSolution();
  bool splitAllowsStoredSolution( const PiecewiseLinearCaseSplit& split, String& error ) const;

  void allSplitsSoFar( List<PiecewiseLinearCaseSplit> &result ) const;

  void recordImpliedValidSplit( PiecewiseLinearCaseSplit &validSplit );

  void replaySmtStackEntry( SmtStackEntry * stackEntry );

  void storeSmtState( SmtState &smtState );


private:
  /*
      Valid splits that were implied by level 0 of the stack.
    */
    List<PiecewiseLinearCaseSplit> _impliedValidSplitsAtRoot;

  /*
    Collect and print various statistics.
  */
  Statistics* _statistics;

  /*
    The case-split stack.
  */
  SmtStack _stack;

  /*
    The engine.
  */
  IEngine* _engine;

  /*
    For debugging purposes only
  */
  Map<unsigned, double> _debuggingSolution;

  /*
    A unique ID allocated to every state that is stored, for
    debugging purposes.
  */
  unsigned _stateId;

  SplitProviders _splitProviders;

};


#endif // __SMTSTACKMANAGER_H__
