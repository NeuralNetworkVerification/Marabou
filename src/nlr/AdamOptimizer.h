/*
 * Copyright Nick Thompson, 2024
 * Use, modification and distribution are subject to the
 * Boost Software License, Version 1.0. (See accompanying file
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef ADAM_OPTIMIZER
#define ADAM_OPTIMIZER

#include "FloatUtils.h"
#include "GlobalConfiguration.h"

#include <algorithm> // for std::sort
#include <cmath>
#include <limits>
#include <mutex>
#include <random>
#include <sstream>
#include <stdexcept>
#include <type_traits> // for std::false_type
#include <utility>
#include <vector>

namespace NLR {

template <typename T, typename = void> struct has_resize : std::false_type
{
};

template <typename T>
struct has_resize<T, std::void_t<decltype( std::declval<T>().resize( size_t{} ) )>> : std::true_type
{
};

template <typename T> constexpr bool has_resize_v = has_resize<T>::value;

template <typename ArgumentContainer>
void validate_bounds( ArgumentContainer const &lower_bounds, ArgumentContainer const &upper_bounds )
{
    std::ostringstream oss;

    if ( lower_bounds.size() == 0 )
    {
        oss << __FILE__ << ":" << __LINE__ << ":" << __func__;
        oss << ": The dimension of the problem cannot be zero.";
        throw std::domain_error( oss.str() );
    }

    if ( upper_bounds.size() != lower_bounds.size() )
    {
        oss << __FILE__ << ":" << __LINE__ << ":" << __func__;
        oss << ": There must be the same number of lower bounds as upper bounds, but given ";
        oss << upper_bounds.size() << " upper bounds, and " << lower_bounds.size()
            << " lower bounds.";
        throw std::domain_error( oss.str() );
    }

    for ( size_t i = 0; i < lower_bounds.size(); ++i )
    {
        auto lb = lower_bounds[i];
        auto ub = upper_bounds[i];
        if ( lb > ub )
        {
            oss << __FILE__ << ":" << __LINE__ << ":" << __func__;
            oss << ": The upper bound must be greater than or equal to the lower bound, but the "
                   "upper bound is "
                << ub << " and the lower is " << lb << ".";
            throw std::domain_error( oss.str() );
        }

        if ( !FloatUtils::isFinite( lb ) )
        {
            oss << __FILE__ << ":" << __LINE__ << ":" << __func__;
            oss << ": The lower bound must be finite, but got " << lb << ".";
            oss << " For infinite bounds, emulate with std::numeric_limits<Real>::lower() or use a "
                   "standard infinite->finite "
                   "transform.";
            throw std::domain_error( oss.str() );
        }

        if ( !FloatUtils::isFinite( ub ) )
        {
            oss << __FILE__ << ":" << __LINE__ << ":" << __func__;
            oss << ": The upper bound must be finite, but got " << ub << ".";
            oss << " For infinite bounds, emulate with std::numeric_limits<Real>::max() or use a "
                   "standard infinite->finite "
                   "transform.";
            throw std::domain_error( oss.str() );
        }
    }
}

template <typename ArgumentContainer, class URBG>
std::vector<ArgumentContainer> random_initial_population( ArgumentContainer const &lower_bounds,
                                                          ArgumentContainer const &upper_bounds,
                                                          size_t initial_population_size,
                                                          URBG &&gen )
{
    using Real = typename ArgumentContainer::value_type;
    using DimensionlessReal = decltype( Real() / Real() );
    constexpr bool has_resize = has_resize_v<ArgumentContainer>;
    std::vector<ArgumentContainer> population( initial_population_size );
    auto const dimension = lower_bounds.size();

    for ( size_t i = 0; i < population.size(); ++i )
    {
        if constexpr ( has_resize )
            population[i].resize( dimension );
        else
        {
            // Argument type must be known at compile-time; like std::array:
            if ( population[i].size() != dimension )
            {
                std::ostringstream oss;
                oss << __FILE__ << ":" << __LINE__ << ":" << __func__;
                oss << ": For containers which do not have resize, the default size must be the "
                       "same as the dimension, ";
                oss << "but the default container size is " << population[i].size()
                    << " and the dimension of the problem is " << dimension << ".";
                oss << " The function argument container type is "
                    << typeid( ArgumentContainer ).name() << ".\n";
                throw std::runtime_error( oss.str() );
            }
        }
    }

    // Why don't we provide an option to initialize with (say) a Gaussian distribution?
    // > If the optimum's location is fairly well known,
    // > a Gaussian distribution may prove somewhat faster, although it
    // > may also increase the probability that the population will converge prematurely.
    // > In general, uniform distributions are preferred, since they best reflect
    // > the lack of knowledge about the optimum's location.
    //  - Differential Evolution: A Practical Approach to Global Optimization
    // That said, scipy uses Latin Hypercube sampling and says self-avoiding sequences are
    // preferable. So this is something that could be investigated and potentially improved.
    std::uniform_real_distribution<DimensionlessReal> dis( DimensionlessReal( 0 ),
                                                           DimensionlessReal( 1 ) );
    for ( size_t i = 0; i < population.size(); ++i )
    {
        for ( size_t j = 0; j < dimension; ++j )
        {
            auto const &lb = lower_bounds[j];
            auto const &ub = upper_bounds[j];
            population[i][j] = lb + dis( gen ) * ( ub - lb );
        }
    }

    return population;
}

template <typename ArgumentContainer>
void validate_initial_guess( ArgumentContainer const &initial_guess,
                             ArgumentContainer const &lower_bounds,
                             ArgumentContainer const &upper_bounds )
{
    std::ostringstream oss;
    auto const dimension = lower_bounds.size();

    if ( initial_guess.size() != dimension )
    {
        oss << __FILE__ << ":" << __LINE__ << ":" << __func__;
        oss << ": The initial guess must have the same dimensions as the problem,";
        oss << ", but the problem size is " << dimension << " and the initial guess has "
            << initial_guess.size() << " elements.";
        throw std::domain_error( oss.str() );
    }

    for ( size_t i = 0; i < dimension; ++i )
    {
        auto lb = lower_bounds[i];
        auto ub = upper_bounds[i];

        if ( !FloatUtils::isFinite( initial_guess[i] ) )
        {
            oss << __FILE__ << ":" << __LINE__ << ":" << __func__;
            oss << ": At index " << i << ", the initial guess is " << initial_guess[i]
                << ", make sure all elements of the initial guess are finite.";
            throw std::domain_error( oss.str() );
        }

        if ( initial_guess[i] < lb || initial_guess[i] > ub )
        {
            oss << __FILE__ << ":" << __LINE__ << ":" << __func__;
            oss << ": At index " << i << " the initial guess " << initial_guess[i]
                << " is not in the bounds [" << lb << ", " << ub << "].";
            throw std::domain_error( oss.str() );
        }
    }
}

// Single-Threaded Adam Optimizer

// We provide the parameters in a struct-there are too many of them and they are too unwieldy to
// pass individually:
template <typename ArgumentContainer> struct adam_optimizer_parameters
{
    using Real = typename ArgumentContainer::value_type;
    ArgumentContainer lower_bounds;
    ArgumentContainer upper_bounds;

    Real step_size = GlobalConfiguration::PREIMAGE_APPROXIMATION_OPTIMIZATION_STEP_SIZE;
    Real lr = GlobalConfiguration::PREIMAGE_APPROXIMATION_OPTIMIZATION_LEARNING_RATE;
    Real lr = 0.05;
    Real epsilon = GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS;
    Real beta1 = 0.9;
    Real beta2 = 0.999;
    Real weight_decay = 0.2;
    bool maximize = false;
    size_t max_iterations = 100;
    ArgumentContainer const *initial_guess = nullptr;
};

template <typename ArgumentContainer>
void validate_adam_optimizer_parameters(
    adam_optimizer_parameters<ArgumentContainer> const &cd_params )
{
    std::ostringstream oss;
    validate_bounds( cd_params.lower_bounds, cd_params.upper_bounds );

    if ( FloatUtils::isNan( cd_params.step_size ) || cd_params.step_size <= 0 )
    {
        oss << __FILE__ << ":" << __LINE__ << ":" << __func__;
        oss << ": step_size > 0 is required, but got step_size=" << cd_params.step_size << ".";
        throw std::domain_error( oss.str() );
    }

    if ( FloatUtils::isNan( cd_params.epsilon ) || cd_params.epsilon <= 0 )
    {
        oss << __FILE__ << ":" << __LINE__ << ":" << __func__;
        oss << ": epsilon > 0 is required, but got epsilon=" << cd_params.epsilon << ".";
        throw std::domain_error( oss.str() );
    }

    if ( FloatUtils::isNan( cd_params.beta1 ) || cd_params.beta1 <= 0 )
    {
        oss << __FILE__ << ":" << __LINE__ << ":" << __func__;
        oss << ": beta1 > 0 is required, but got beta1=" << cd_params.beta1 << ".";
        throw std::domain_error( oss.str() );
    }

    if ( FloatUtils::isNan( cd_params.beta2 ) || cd_params.beta2 <= 0 )
    {
        oss << __FILE__ << ":" << __LINE__ << ":" << __func__;
        oss << ": beta2 > 0 is required, but got beta2=" << cd_params.beta2 << ".";
        throw std::domain_error( oss.str() );
    }

    if ( FloatUtils::isNan( cd_params.lr ) || cd_params.lr <= 0 )
    {
        oss << __FILE__ << ":" << __LINE__ << ":" << __func__;
        oss << ": lr > 0 is required, but got lr=" << cd_params.lr << ".";
        throw std::domain_error( oss.str() );
    }

    if ( FloatUtils::isNan( cd_params.weight_decay ) || cd_params.weight_decay < 0 )
    {
        oss << __FILE__ << ":" << __LINE__ << ":" << __func__;
        oss << ": weight_decay >= 0 is required, but got weight_decay=" << cd_params.weight_decay
            << ".";
        throw std::domain_error( oss.str() );
    }

    if ( cd_params.max_iterations < 1 )
    {
        oss << __FILE__ << ":" << __LINE__ << ":" << __func__;
        oss << ": There must be at least one generation.";
        throw std::invalid_argument( oss.str() );
    }

    if ( cd_params.initial_guess )
    {
        validate_initial_guess(
            *cd_params.initial_guess, cd_params.lower_bounds, cd_params.upper_bounds );
    }
}

template <typename ArgumentContainer, class Func, class URBG>
ArgumentContainer adam_optimizer(
    const Func cost_function,
    adam_optimizer_parameters<ArgumentContainer> const &cd_params,
    URBG &gen,
    std::invoke_result_t<Func, ArgumentContainer> target_value,
    bool *cancellation = nullptr,
    std::vector<std::pair<ArgumentContainer, std::invoke_result_t<Func, ArgumentContainer>>>
        *queries = nullptr,
    std::invoke_result_t<Func, ArgumentContainer> *current_minimum_cost = nullptr )
{
    using ResultType = std::invoke_result_t<Func, ArgumentContainer>;

    validate_adam_optimizer_parameters( cd_params );
    const size_t dimension = cd_params.lower_bounds.size();
    auto step_size = cd_params.step_size;
    auto max_iterations = cd_params.max_iterations;
    auto maximize = cd_params.maximize;
    auto epsilon = cd_params.epsilon;
    auto beta1 = cd_params.beta1;
    auto beta2 = cd_params.beta2;
    auto lr = cd_params.lr;
    auto weight_decay = cd_params.weight_decay;
    auto guess =
        random_initial_population( cd_params.lower_bounds, cd_params.upper_bounds, 1, gen );

    if ( cd_params.initial_guess )
    {
        guess[0] = *cd_params.initial_guess;
    }

    std::vector<ArgumentContainer> candidates( dimension );
    std::vector<ResultType> gradient( dimension );
    std::vector<ResultType> first_moment( dimension );
    std::vector<ResultType> second_moment( dimension );
    ArgumentContainer current_minimum = guess[0];

    for ( size_t j = 0; j < dimension; ++j )
    {
        first_moment[j] = 0;
        second_moment[j] = 0;
    }

    for ( size_t i = 0; i < max_iterations; ++i )
    {
        for ( size_t j = 0; j < dimension; ++j )
        {
            candidates[j] = guess[0];
            candidates[j][j] += step_size;

            size_t sign = ( maximize == false ? 1 : -1 );
            double cost = cost_function( candidates[j] );
            gradient[j] = sign * cost / step_size + weight_decay * guess[0][j];

            if ( !FloatUtils::isNan( target_value ) && cost <= target_value )
            {
                guess[0] = candidates[j];
                break;
            }
        }

        for ( size_t j = 0; j < dimension; ++j )
        {
            first_moment[j] = beta1 * first_moment[j] + ( 1 - beta1 ) * gradient[j];
            first_moment[j] /= ( 1 - std::pow( beta1, step_size ) );

            second_moment[j] = beta2 * first_moment[j] + ( 1 - beta2 ) * std::pow( gradient[j], 2 );
            second_moment[j] /= ( 1 - std::pow( beta2, step_size ) );

            guess[0][j] -= lr * first_moment[j] / ( std::sqrt( second_moment[j] ) + epsilon );
        }
    }

    return guess[0];
}

} // namespace NLR
#endif
