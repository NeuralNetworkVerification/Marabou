/*
 * Copyright Nick Thompson, 2024
 * Use, modification and distribution are subject to the
 * Boost Software License, Version 1.0. (See accompanying file
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef DIFFERENTIAL_EVOLUTION
#define DIFFERENTIAL_EVOLUTION

#include "FloatUtils.h"

#include <algorithm> // for std::sort
#include <atomic>
#include <cmath>
#include <limits>
#include <mutex>
#include <random>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <type_traits>  // for std::false_type
#include <utility>
#include <vector>

namespace NLR {

    template <typename T, typename = void> struct has_resize : std::false_type {};

    template <typename T> struct has_resize<T, std::void_t<decltype( std::declval<T>().resize( size_t{} ) )>> : std::true_type {};

    template <typename T> constexpr bool has_resize_v = has_resize<T>::value;

    template <typename ArgumentContainer>
    void validate_bounds( ArgumentContainer const &lower_bounds,
                          ArgumentContainer const &upper_bounds )
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
            oss << upper_bounds.size() << " upper bounds, and " << lower_bounds.size() << " lower bounds.";
            throw std::domain_error( oss.str() );
        }
    
        for ( size_t i = 0; i < lower_bounds.size(); ++i ) 
        {
            auto lb = lower_bounds[i];
            auto ub = upper_bounds[i];
            if ( lb > ub )
            {
                oss << __FILE__ << ":" << __LINE__ << ":" << __func__;
                oss << ": The upper bound must be greater than or equal to the lower bound, but the upper bound is " << ub
                    << " and the lower is " << lb << ".";
                throw std::domain_error( oss.str() );
            }
    
            if ( !FloatUtils::isFinite( lb ) )
            {
              oss << __FILE__ << ":" << __LINE__ << ":" << __func__;
              oss << ": The lower bound must be finite, but got " << lb << ".";
              oss << " For infinite bounds, emulate with std::numeric_limits<Real>::lower() or use a standard infinite->finite "
                     "transform.";
              throw std::domain_error( oss.str() );
            }
    
            if ( !FloatUtils::isFinite( ub ) )
            {
                oss << __FILE__ << ":" << __LINE__ << ":" << __func__;
                oss << ": The upper bound must be finite, but got " << ub << ".";
                oss << " For infinite bounds, emulate with std::numeric_limits<Real>::max() or use a standard infinite->finite "
                       "transform.";
                throw std::domain_error( oss.str() );
            }
        }
    }

    template <typename ArgumentContainer, class URBG>
    std::vector<ArgumentContainer> random_initial_population( ArgumentContainer const &lower_bounds,
                                                              ArgumentContainer const &upper_bounds,
                                                              size_t initial_population_size, URBG &&gen )
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
                    oss << ": For containers which do not have resize, the default size must be the same as the dimension, ";
                    oss << "but the default container size is " << population[i].size() << " and the dimension of the problem is "
                        << dimension << ".";
                    oss << " The function argument container type is " << typeid( ArgumentContainer ).name() << ".\n";
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
        // That said, scipy uses Latin Hypercube sampling and says self-avoiding sequences are preferable.
        // So this is something that could be investigated and potentially improved.
        std::uniform_real_distribution<DimensionlessReal> dis( DimensionlessReal( 0 ), DimensionlessReal( 1 ) );
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
                                 ArgumentContainer const &upper_bounds)
    {
        std::ostringstream oss;
        auto const dimension = lower_bounds.size();
        
        if ( initial_guess.size() != dimension )
        {
            oss << __FILE__ << ":" << __LINE__ << ":" << __func__;
            oss << ": The initial guess must have the same dimensions as the problem,";
            oss << ", but the problem size is " << dimension << " and the initial guess has " << initial_guess.size()
                << " elements.";
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
                oss << ": At index " << i << " the initial guess " << initial_guess[i] << " is not in the bounds [" << lb << ", "
                    << ub << "].";
                throw std::domain_error( oss.str() );
            }
        }
    }

    // Return indices corresponding to the minimum function values.
    template <typename Real>
    std::vector<size_t> best_indices( std::vector<Real> const &function_values )
    {
        const size_t n = function_values.size();
        std::vector<size_t> indices( n );
        
        for ( size_t i = 0; i < n; ++i )
        {
            indices[i] = i;
        }

        std::sort( indices.begin(),
                   indices.end(),
                   [&]( size_t a, size_t b )
                   {
                       if ( FloatUtils::isNan( function_values[a] ) )
                       {
                           return false;
                       }
                       if ( FloatUtils::isNan( function_values[b] ) )
                       {
                           return true;
                       }
                       return function_values[a] < function_values[b];
                   });
        
        return indices;
    }

    template<typename RandomAccessContainer>
    auto weighted_lehmer_mean( RandomAccessContainer const & values,
                               RandomAccessContainer const & weights)
    {
        if ( values.size() != weights.size() )
        {
            std::ostringstream oss;
            oss << __FILE__ << ":" << __LINE__ << ":" << __func__;
            oss << ": There must be the same number of weights as values, but got " << values.size()
                << " values and " << weights.size() << " weights.";
            throw std::logic_error( oss.str() );
        }
        
        if ( values.size() == 0 )
        {
            std::ostringstream oss;
            oss << __FILE__ << ":" << __LINE__ << ":" << __func__;
            oss << ": There must at least one value provided.";
            throw std::logic_error( oss.str() );
        }
        
        using Real = typename RandomAccessContainer::value_type;
        Real numerator = 0;
        Real denominator = 0;
        
        for ( size_t i = 0; i < values.size(); ++i )
        {
            if ( weights[i] < 0 || !FloatUtils::isFinite( weights[i] ) )
            {
                std::ostringstream oss;
                oss << __FILE__ << ":" << __LINE__ << ":" << __func__;
                oss << ": All weights must be positive and finite, but got received weight " << weights[i]
                    << " at index " << i << " of " << weights.size() << ".";
                throw std::domain_error( oss.str() );
            }
            
            Real tmp = weights[i] * values[i];
            numerator += tmp * values[i];
            denominator += tmp;
        }
        return numerator / denominator;
    }

    // Storn, R., Price, K. (1997). Differential evolution-a simple and efficient heuristic for global optimization over
    // continuous spaces.
    // Journal of global optimization, 11, 341-359.
    // See:
    // https://www.cp.eng.chula.ac.th/~prabhas//teaching/ec/ec2012/storn_price_de.pdf

    // We provide the parameters in a struct-there are too many of them and they are too unwieldy to pass individually:
    template <typename ArgumentContainer> struct differential_evolution_parameters
    {
        using Real = typename ArgumentContainer::value_type;
        using DimensionlessReal = decltype( Real() / Real() );
        ArgumentContainer lower_bounds;
        ArgumentContainer upper_bounds;
        
        // mutation factor is also called scale factor or just F in the literature:
        DimensionlessReal mutation_factor = static_cast<DimensionlessReal>( 0.65 );
        DimensionlessReal crossover_probability = static_cast<DimensionlessReal>( 0.5 );
        
        // Population in each generation:
        size_t NP = 500;
        size_t max_generations = 1000;
        ArgumentContainer const *initial_guess = nullptr;
        unsigned threads = std::thread::hardware_concurrency();
    };

    template <typename ArgumentContainer>
    void validate_differential_evolution_parameters( differential_evolution_parameters<ArgumentContainer> const &de_params )
    {
        std::ostringstream oss;
        validate_bounds( de_params.lower_bounds, de_params.upper_bounds );
        if ( de_params.NP < 4 )
        {
            oss << __FILE__ << ":" << __LINE__ << ":" << __func__;
            oss << ": The population size must be at least 4, but requested population size of " << de_params.NP << ".";
            throw std::invalid_argument( oss.str() );
        }
        
        // From: "Differential Evolution: A Practical Approach to Global Optimization (Natural Computing Series)"
        // > The scale factor, F in (0,1+), is a positive real number that controls the rate at which the population evolves.
        // > While there is no upper limit on F, effective values are seldom greater than 1.0.
        // ...
        // Also see "Limits on F", Section 2.5.1:
        // > This discontinuity at F = 1 reduces the number of mutants by half and can result in erratic convergence...
        auto F = de_params.mutation_factor;
        if ( FloatUtils::isNan( F ) || F >= 1 || F <= 0 )
        {
            oss << __FILE__ << ":" << __LINE__ << ":" << __func__;
            oss << ": F in (0, 1) is required, but got F=" << F << ".";
            throw std::domain_error( oss.str() );
        }
        
        if ( de_params.max_generations < 1 )
        {
            oss << __FILE__ << ":" << __LINE__ << ":" << __func__;
            oss << ": There must be at least one generation.";
            throw std::invalid_argument( oss.str() );
        }
        
        if ( de_params.initial_guess )
        {
            validate_initial_guess( *de_params.initial_guess, de_params.lower_bounds, de_params.upper_bounds );
        }
        
        if ( de_params.threads == 0 )
        {
            oss << __FILE__ << ":" << __LINE__ << ":" << __func__;
            oss << ": There must be at least one thread.";
            throw std::invalid_argument( oss.str() );
        }
    }

    template <typename ArgumentContainer, class Func, class URBG>
    ArgumentContainer differential_evolution( const Func cost_function,
                                              differential_evolution_parameters<ArgumentContainer> const &de_params,
                                              URBG &gen,
                                              std::invoke_result_t<Func, ArgumentContainer> target_value
                                              = std::numeric_limits<std::invoke_result_t<Func, ArgumentContainer>>::quiet_NaN(),
                                              std::atomic<bool> *cancellation = nullptr,
                                              std::vector<std::pair<ArgumentContainer, std::invoke_result_t<Func, ArgumentContainer>>> *queries = nullptr,
                                              std::atomic<std::invoke_result_t<Func, ArgumentContainer>> *current_minimum_cost = nullptr)
    {
        using Real = typename ArgumentContainer::value_type;
        using DimensionlessReal = decltype( Real() / Real() );
        using ResultType = std::invoke_result_t<Func, ArgumentContainer>;
        
        validate_differential_evolution_parameters( de_params );
        const size_t dimension = de_params.lower_bounds.size();
        auto NP = de_params.NP;
        auto population = random_initial_population( de_params.lower_bounds, de_params.upper_bounds, NP, gen );
        
        if ( de_params.initial_guess )
        {
            population[0] = *de_params.initial_guess;
        }
        
        std::vector<ResultType> cost( NP, std::numeric_limits<ResultType>::quiet_NaN() );
        std::atomic<bool> target_attained = false;
        
        // This mutex is only used if the queries are stored:
        std::mutex mt;

        std::vector<std::thread> thread_pool;
        auto const threads = de_params.threads;
        for ( size_t j = 0; j < threads; ++j )
        {
            // Note that if some members of the population take way longer to compute,
            // then this parallelization strategy is very suboptimal.
            // However, we tried using std::async (which should be robust to this particular problem),
            // but the overhead was just totally unacceptable on ARM Macs (the only platform tested).
            // As the economists say "there are no solutions, only tradeoffs".
            
            thread_pool.emplace_back( [&, j]()
                                      {
                                          for ( size_t i = j; i < cost.size(); i += threads )
                                          {
                                              cost[i] = cost_function( population[i] );
                                              if ( current_minimum_cost && cost[i] < *current_minimum_cost )
                                              {
                                                  *current_minimum_cost = cost[i];
                                              }
                                              if ( queries )
                                              {
                                                  std::scoped_lock lock( mt );
                                                  queries->push_back( std::make_pair( population[i], cost[i] ) );
                                              }
                                              
                                              if ( !FloatUtils::isNan( target_value ) && cost[i] <= target_value )
                                              {
                                                  target_attained = true;
                                              }
                                          }
                                      });
        }
      
        for ( auto &thread : thread_pool )
        {
            thread.join();
        }

        std::vector<ArgumentContainer> trial_vectors( NP );
        for ( size_t i = 0; i < NP; ++i )
        {
            if constexpr ( has_resize_v<ArgumentContainer> )
            {
                trial_vectors[i].resize( dimension );
            }
        }
      
        std::vector<URBG> thread_generators( threads );
        for ( size_t j = 0; j < threads; ++j )
        {
            thread_generators[j].seed( gen() );
        }
      
        // std::vector<bool> isn't threadsafe!
        std::vector<int> updated_indices( NP, 0 );

        for ( size_t generation = 0; generation < de_params.max_generations; ++generation )
        {
            if ( cancellation && *cancellation )
            {
                break;
            }
            if ( target_attained )
            {
                break;
            }
            
            thread_pool.resize( 0 );
            for ( size_t j = 0; j < threads; ++j )
            {
                thread_pool.emplace_back( [&, j]()
                                          {
                                              auto& tlg = thread_generators[j];
                                              std::uniform_real_distribution<DimensionlessReal> unif01( DimensionlessReal( 0 ), DimensionlessReal( 1 ) );
                                                
                                              for ( size_t i = j; i < cost.size(); i += threads )
                                              {
                                                  if ( target_attained )
                                                  {
                                                      return;
                                                  }
                                                  if ( cancellation && *cancellation )
                                                  {
                                                      return;
                                                  }
                                              
                                                  size_t r1, r2, r3;
                                                  do
                                                  {
                                                      r1 = tlg() % NP;
                                                  } while ( r1 == i );
                                                  
                                                  do
                                                  {
                                                      r2 = tlg() % NP;
                                                  } while ( r2 == i || r2 == r1 );
                                                  
                                                  do
                                                  {
                                                      r3 = tlg() % NP;
                                                  } while ( r3 == i || r3 == r2 || r3 == r1 );
                                                  
  
                                                  for ( size_t k = 0; k < dimension; ++k )
                                                  {
                                                      // See equation (4) of the reference:
                                                      auto guaranteed_changed_idx = tlg() % dimension;
                                                      if ( unif01( tlg ) < de_params.crossover_probability || k == guaranteed_changed_idx )
                                                      {
                                                          auto tmp = population[r1][k] + de_params.mutation_factor * ( population[r2][k] - population[r3][k] );
                                                          auto const &lb = de_params.lower_bounds[k];
                                                          auto const &ub = de_params.upper_bounds[k];
                                                          // Some others recommend regenerating the indices rather than clamping;
                                                          // I dunno seems like it could get stuck regenerating . . .
                                                          trial_vectors[i][k] = std::clamp( tmp, lb, ub );
                                                      }
                                                      else
                                                      {
                                                          trial_vectors[i][k] = population[i][k];
                                                      }
                                                  }
  
                                                  auto const trial_cost = cost_function( trial_vectors[i] );
                                                  if ( FloatUtils::isNan( trial_cost ) )
                                                  {
                                                      continue;
                                                  }
                                                  if ( queries )
                                                  {
                                                      std::scoped_lock lock( mt );
                                                      queries->push_back( std::make_pair( trial_vectors[i], trial_cost ) );
                                                  }
                                                  
                                                  if ( trial_cost < cost[i] || FloatUtils::isNan( cost[i] ) )
                                                  {
                                                      cost[i] = trial_cost;
                                                      if ( !FloatUtils::isNan( target_value ) && cost[i] <= target_value )
                                                      {
                                                          target_attained = true;
                                                      }
                                                      
                                                      if ( current_minimum_cost && cost[i] < *current_minimum_cost )
                                                      {
                                                          *current_minimum_cost = cost[i];
                                                      }
                                                      
                                                      // Can't do this! It's a race condition!
                                                      //population[i] = trial_vectors[i];
                                                      // Instead mark all the indices that need to be updated:
                                                      updated_indices[i] = 1;
                                                  }
                                              }
                                          });
            }
            for ( auto &thread : thread_pool )
            {
                thread.join();
            }
            
            for ( size_t i = 0; i < NP; ++i )
            {
                if ( updated_indices[i] )
                {
                    population[i] = trial_vectors[i];
                    updated_indices[i] = 0;
                }
            }
        }

        auto it = std::min_element( cost.begin(), cost.end() );
        return population[std::distance( cost.begin(), it )];
    }

} // namespace NLR
#endif
