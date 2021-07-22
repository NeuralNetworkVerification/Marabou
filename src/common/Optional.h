#ifndef __OPTIONAL_H__
#define __OPTIONAL_H__

#include <boost/optional.hpp>

template <class T>
class Optional
{
public:
    using value_type = T;
    using container_type = boost::optional<T>;

    Optional() = default;
    template <class U>
    Optional(U &&value) : _container{std::forward<U>(value)} {}
    ~Optional() = default;

    operator bool() const
    {
        return _container.has_value();
    }

    value_type operator*() const
    {
        return *_container;
    }

    value_type* operator->() const
    {
        return _container.operator->();
    }

private:
    container_type _container;
};

template <typename T>
inline Optional<T> make_optional(T &&v)
{
    return Optional<T>(std::forward<T>(v));
}

#endif // __OPTIONAL_H__
