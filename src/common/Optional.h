#ifndef __OPTIONAL_H__
#define __OPTIONAL_H__

#include <boost/optional.hpp>

struct nullopt_t {
    explicit constexpr nullopt_t(int) {}
};
constexpr nullopt_t nullopt{int()};

template <class T>
class Optional
{
public:
    using value_type = T;
    using container_type = boost::optional<T>;

    Optional() = default;
    Optional(value_type const& v) : _container{v} {}
    Optional(value_type &&value) : _container{std::move(value)} {}
    Optional(nullopt_t) {}
    ~Optional() = default;

    operator bool() const
    {
        return _container.has_value();
    }

    value_type const& operator*() const
    {
        return *_container;
    }

    value_type & operator*() 
    {
        return *_container;
    }

    value_type const* operator->() const
    {
        return _container.operator->();
    }

    value_type* operator->() 
    {
        return _container.operator->();
    }

private:
    container_type _container;
};

template <typename T>
inline Optional<T> makeOptional(T &&v)
{
    return Optional<T>(std::forward<T>(v));
}

#endif // __OPTIONAL_H__
