#ifndef SPLITPROVIDERSMANAGER_H
#define SPLITPROVIDERSMANAGER_H

#include <memory>

#include "List.h"
#include "ISmtSplitProvider.h"
#include "SmtStackEntry.h"

using SmtStack = List<SmtStackEntry*>;

class SplitProvidersManager
{
private:
    List<std::shared_ptr<ISmtSplitProvider>> _splitProviders;
public:

    bool subscribeSplitProvider( std::shared_ptr<ISmtSplitProvider> provider );
    bool ubsubscribeSplitProvider( std::shared_ptr<ISmtSplitProvider> provider );

    /*
      Let providers think before requesting split
    */
    void letProvidersThink( SmtStack const& stack );

    /*
        Request splits from providers
    */
    Optional<PiecewiseLinearCaseSplit> splitFromProviders() const;

    void notifyUnsat();

    void onSplitPerformed(SplitInfo const& split);

    void onStackPopPerformed(PopInfo const& pop);

    Optional<PiecewiseLinearCaseSplit> alternativeSplitsFromProviders() const;

    void letProvidersThinkForAlternatives( SmtStack const& stack );
};



#endif // SPLITPROVIDERSMANAGER_H
