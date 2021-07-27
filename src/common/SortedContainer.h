#ifndef __SORTEDCONTAINER_H__
#define __SORTEDCONTAINER_H__
#include <iostream>

#include "CommonError.h"

#include <set>

template<class T, class Compare = std::less<T>>
class SortedContainer
{
    using Super = std::set<T, Compare> ;
public:
    using value_type = T;

    using iterator = typename Super::iterator;
    using const_iterator = typename Super::const_iterator;

    using reverse_iterator = typename Super::reverse_iterator;
    using const_reverse_iterator = typename Super::const_reverse_iterator;

    SortedContainer() = default; 

    SortedContainer(  std::initializer_list<value_type> initializerList )
     : _container( initializerList ) { }

    SortedContainer( std::initializer_list<value_type> init, Compare const& comp = Compare() );

    explicit SortedContainer(Compare const &comp);

    template< class InputIt >
    SortedContainer( InputIt first, InputIt last,  Compare const& comp = Compare());

    void inplaceUnion(  SortedContainer<value_type> const& other )
    {
        for (auto const &element : other)
            _container.insert( element );
    }

    void add(value_type const &value)
    {
        _container.insert( value );
    }

    iterator begin()
    {
        return _container.begin();
    }

    const_iterator begin() const
    {
        return _container.begin();
    }

    reverse_iterator rbegin()
    {
        return _container.rbegin();
    }
    const_reverse_iterator rbegin() const
    {
        return _container.rbegin();
    }

    iterator end()
    {
        return _container.end();
    }

    const_iterator end() const
    {
        return _container.end();
    }

    reverse_iterator rend()
    {
        return _container.rend();
    }

    const_reverse_iterator rend() const
    {
        return _container.rend();
    }

    iterator erase( iterator it )
    {
        return _container.erase( it );
    }

    void erase(value_type const &value)
    {
        return _container.erase(value);
    }

    void clear()
    {
        _container.clear();
    }

    typename Super::size_type size() const
    {
        return _container.size();
    }

    bool empty() const
    {
        return _container.empty();
    }

    bool contains(value_type const &value) const
    {
        return _container.find(value) != _container.end();    
    }

    bool contains(SortedContainer<value_type> const &other) const
    {
        auto it = begin();
        auto const endIt=  end();
        for(auto const & otherValue: other)
        {
            auto maybeValue = std::lower_bound(it, endIt, otherValue);
            if(*maybeValue != otherValue)
            {
                // otherValue is not in self, so other is not a subset of ourselves
                return false;
            }
        }
        return true;
    }

    bool operator==(SortedContainer<value_type> const &other) const
    {
        return _container == other._container;
    }

    bool operator!=(SortedContainer<value_type> const &other) const
    {
        return _container != other._container;
    }

protected:
    Super _container;
};


#endif // __SORTEDCONTAINER_H__