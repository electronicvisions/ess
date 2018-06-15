#pragma once

#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>

#include <boost/array.hpp>

#include "boost/numeric/odeint.hpp"


/// Implementation of the classical 2nd order Heun Method.
/// This implementation follows the example heun.cpp from odeint-v2
///
/// We have to define the coefficients of the Butcher's table:
/// c[0] |
/// c[1] | a1[0]
/// -------------------
///      | b[0]  b[1]
//
/// For Heun's method the Butcher table reads:
///  0   |
///  1/2 | 1/2
/// ---------------
///      | 1/2  1/2
///
/// cf. https://en.wikipedia.org/wiki/Heun's_method
namespace boost {
namespace numeric {
namespace odeint {

//[ heun_define_coefficients
template< class Value = double >
struct heun_a1 : array< Value , 1 > {
    heun_a1( void )
    {
        (*this)[0] = static_cast< Value >( 1 );
    }
};

template< class Value = double >
struct heun_b : array< Value , 2 >
{
    heun_b( void )
    {
        (*this)[0] = static_cast<Value>( 1 ) / static_cast<Value>( 2 );
        (*this)[1] = static_cast<Value>( 1 ) / static_cast<Value>( 2 );
    }
};

template< class Value = double >
struct heun_c : array< Value , 2 >
{
    heun_c( void )
    {
        (*this)[0] = static_cast< Value >( 0 );
        (*this)[1] = static_cast< Value >( 1 ) / static_cast< Value >( 2 );
    }
};
//]


//[ heun_stepper_definition
template<
    class State ,
    class Value = double ,
    class Deriv = State ,
    class Time = Value ,
    class Algebra = range_algebra ,
    class Operations = default_operations ,
    class Resizer = initially_resizer
>
class heun2 : public
explicit_generic_rk< 2 , 2 , State , Value , Deriv , Time ,
                                             Algebra , Operations , Resizer >
{

public:

    typedef explicit_generic_rk< 2 , 2 , State , Value , Deriv , Time ,
                                                         Algebra , Operations , Resizer > stepper_base_type;

    typedef typename stepper_base_type::state_type state_type;
    typedef typename stepper_base_type::wrapped_state_type wrapped_state_type;
    typedef typename stepper_base_type::value_type value_type;
    typedef typename stepper_base_type::deriv_type deriv_type;
    typedef typename stepper_base_type::wrapped_deriv_type wrapped_deriv_type;
    typedef typename stepper_base_type::time_type time_type;
    typedef typename stepper_base_type::algebra_type algebra_type;
    typedef typename stepper_base_type::operations_type operations_type;
    typedef typename stepper_base_type::resizer_type resizer_type;
    typedef typename stepper_base_type::stepper_type stepper_type;

    heun2( const algebra_type &algebra = algebra_type() )
    : stepper_base_type(
            fusion::make_vector( heun_a1<Value>() ) ,
            heun_b<Value>() , heun_c<Value>() , algebra )
    { }
};
//]

} // namespace odeint
} // namespace numeric
} // namespace boost
