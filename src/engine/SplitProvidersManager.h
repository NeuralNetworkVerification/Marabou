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
    bool unsubscribeSplitProvider( std::shared_ptr<ISmtSplitProvider> provider );

    /*
      Let providers think before requesting split
    */
    void letProvidersThink( SmtStack const& stack );

    /*
        Request splits from providers
    */
    Optional<PiecewiseLinearCaseSplit> splitFromProviders() const;

    void notifyUnsat(SmtStack const& stack);

    void onSplitPerformed(SplitInfo const& split);

    void onStackPopPerformed(PopInfo const& pop);

    Optional<PiecewiseLinearCaseSplit> alternativeSplitsFromProviders() const;

    void letProvidersThinkForAlternatives( SmtStack const& stack );

    template <class Func>
    void forEachProvider(Func&& func)
    {
        for(auto& provider : _splitProviders)
        {
            std::forward<Func>(func)(provider.get());
        }
    }
};



#endif // SPLITPROVIDERSMANAGER_H
