#include "SplitProvidersManager.h"

bool SplitProvidersManager::ubsubscribeSplitProvider( std::shared_ptr<ISmtSplitProvider> provider ) {
    auto it = std::find( _splitProviders.begin(), _splitProviders.end(), provider );
    if ( it == _splitProviders.end() ) return false;
    _splitProviders.erase( it );
    return true;
}

bool SplitProvidersManager::subscribeSplitProvider( std::shared_ptr<ISmtSplitProvider> provider ) {
    for ( auto const& alreadySubscribed : _splitProviders )
    {
        if ( provider == alreadySubscribed ) return false;
    }
    _splitProviders.append( provider );
    return true;
}

void SplitProvidersManager::onStackPopPerformed( PopInfo const& pop )
{
    for ( auto const& splitProvider : _splitProviders )
    {
        splitProvider->onStackPopPerformed( pop );
    }
}

void SplitProvidersManager::onSplitPerformed( SplitInfo const& split )
{
    for ( auto const& splitProvider : _splitProviders )
    {
        splitProvider->onSplitPerformed( { split } );
    }
}

void SplitProvidersManager::letProvidersThink( SmtStack const& stack ) {
    for ( auto const& splitProvider : _splitProviders )
    {
        splitProvider->thinkBeforeSplit( stack );
    }
}

Optional<PiecewiseLinearCaseSplit> SplitProvidersManager::splitFromProviders() const {
    for ( auto const& splitProvider : _splitProviders )
    {
        auto const maybeSplits = splitProvider->needToSplit();
        if ( maybeSplits )
        {
            return maybeSplits;
        }
    }
    return nullopt;
}

void SplitProvidersManager::notifyUnsat() {
    for ( auto const& splitProvider : _splitProviders )
    {
        splitProvider->onUnsatReceived();
    }
}
