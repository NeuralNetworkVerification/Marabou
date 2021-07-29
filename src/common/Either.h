#ifndef __EITHER_HPP__
#define __EITHER_HPP__

#include "Optional.h"
#include <boost/variant.hpp>

template <class T>
struct Left
{
    using value_type = T;
    T value;
};
template <class T>
struct Right
{
    using value_type = T;
    T value;
};

template <class T>
auto left(T &&v) -> Left<T>
{
    return Left<T>{std::forward<T>(v)};
}
template <class T>
auto right(T &&v) -> Right<T>
{
    return Right<T>{std::forward<T>(v)};
}

// fwd declare
template <class L, class R>
class Either;

namespace detail
{
    template <class F, class... Args>
    using result_of_t = typename std::result_of<F(Args...)>::type;

    template <class LF, class RF, class L, class R>
    using EitherApplyRes = Either<result_of_t<LF, L>, result_of_t<RF, R>>;
}

template <class L, class R>
class Either
{

   
public:
    using left_value_type = L;
    using right_value_type = R;
    using left_type = Left<L>;
    using right_type = Right<R>;
    using container_t = boost::variant<left_type, right_type>;
    using this_type = Either<L, R>;

private:
    bool _is_left;
    container_t _container;

public:

    constexpr Either(Left<L> const &l)
        : _is_left{true}, _container{l}
    { }
    constexpr Either(Right<R> const &r)
        :_is_left{false}, _container{r}
    { }
    constexpr Either(Left<L> &&l)
        : _is_left{true}, _container{std::move(l)}
    { }
    constexpr Either(Right<R> &&r)
        : _is_left{false}, _container{std::move(r)}
    { }

    static constexpr auto left_of(L &&l) -> Either<L, R>
    {
        return Either(Left<L>{std::move(l)});
    }
    static constexpr auto left_of(L const &l) -> Either<L, R>
    {
        return Either(Left<L>{l});
    }
    static constexpr auto right_of(R &&r) -> Either<L, R>
    {
        return Either(Right<R>{std::move(r)});
    }
    static constexpr auto right_of(R const &r) -> Either<L, R>
    {
        return Either(Right<R>{r});
    }

    constexpr bool is_left() const
    {
        return _is_left;
    }
    constexpr bool is_right() const
    {
        return !_is_left;
    }

    constexpr auto left_value() const -> Optional<L>
    {
        return is_left() ? Optional<L>(boost::get<left_type>(_container).value) : nullopt;
    }
    constexpr auto right_value() const -> Optional<R>
    {
        return is_right() ? Optional<R>(boost::get<right_type>(_container).value) : nullopt;
    }

    template <class LF, class RF, class Ret = detail::EitherApplyRes<LF, RF, L, R>>
    constexpr auto map(LF &&left_case, RF &&right_case) const -> Ret
    {
        return is_left() ? 
            Ret::left_of(std::forward<LF>(left_case)(boost::get<left_type>(_container).value)) : 
            Ret::right_of(std::forward<RF>(right_case)(boost::get<right_type>(_container).value));
    }

};

#endif // __EITHER_HPP__
