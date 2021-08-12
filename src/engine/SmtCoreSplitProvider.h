#ifndef __SMTCORESPLITPROVIDER_H__
#define __SMTCORESPLITPROVIDER_H__

#include "ISmtSplitProvider.h"
#include "IEngine.h"
#include "PiecewiseLinearConstraint.h"
#include "Queue.h"
#include "Stack.h"

class SmtCoreSplitProvider : public ISmtSplitProvider
{

public:
  explicit SmtCoreSplitProvider( IEngine* _engine );

  void thinkBeforeSplit( List<SmtStackEntry*> stack ) override;
  Optional<PiecewiseLinearCaseSplit> needToSplit() const override;
  void onSplitPerformed( SplitInfo const& ) override;
  void onStackPopPerformed( PopInfo const& ) override;
  void onUnsatReceived() override;

  /*
    Inform the SMT core that a PL constraint is violated.
  */
  void reportViolatedConstraint( PiecewiseLinearConstraint* constraint );

  /*
    Get the number of times a specific PL constraint has been reported as
    violated.
  */
  unsigned getViolationCounts( PiecewiseLinearConstraint* constraint ) const;


  /*
    Have the SMT core choose, among a set of violated PL constraints, which
    constraint should be repaired (without splitting)
  */
  PiecewiseLinearConstraint* chooseViolatedConstraintForFixing( List<PiecewiseLinearConstraint*>& _violatedPlConstraints ) const;

  void resetReportedViolations();


private:
  bool pickSplitPLConstraint();

  bool searchForAlternatives();

  /*
    The engine. TODO figure out if we can avoid having it as a member
  */
  IEngine* _engine;

  /*
    Collect and print various statistics.
  */
  Statistics* _statistics;

  /*
    Do we need to perform a split and on which constraint.
  */
  bool _needToSplit;
  Optional<PiecewiseLinearCaseSplit> _currentSuggestedSplit;
  List<PiecewiseLinearCaseSplit> _currentSuggestedSplitAlternatives;
  List<Pair<PiecewiseLinearCaseSplit, List<PiecewiseLinearCaseSplit>>> _splitsStack;
  PiecewiseLinearConstraint* _constraintForSplitting;

  /*
    Count how many times each constraint has been violated.
  */
  Map<PiecewiseLinearConstraint*, unsigned> _constraintToViolationCount;

  /*
    For debugging purposes only
  */
  Map<unsigned, double> _debuggingSolution;

  /*
    A unique ID allocated to every state that is stored, for
    debugging purposes.
  */
  unsigned _stateId;

  /*
    Split when some relu has been violated for this many times
  */
  unsigned _constraintViolationThreshold;
};

#endif // __SMTCORESPLITPROVIDER_H__
