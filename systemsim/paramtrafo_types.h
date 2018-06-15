#ifndef __HWTRANS_H__
#define __HWTRANS_H__

/*

 Copyright: TUD/UHEI 2007 - 2012
 License: GPL
 Description: hardware transformation functions

*/

#include "common2.h"

/// Translation types for the translation of biological parameters to hardware parameters.
/// 	possible transformations describing the relation between desired and necessary parameters are:
///     	CONST      = constant value
///         LINEAR     = linear relationship
///         LINEAR_REV = inverse linear transformation (linear)
///         POLY       = polynomial
namespace parameter_translation_types {
enum trans_type {
	CONST = 0, //!< parameter is fixed
	LINEAR = 1, //!< multiplication with factor + offset
	LINEAR_REV = 2, //!< reverse the linear transformation
	POLY = 3 //!< polynomial transformation, polynomial order depending on transformation parameter vector length
	};
}

///  Translation direction for the translation of biological parameters to hardware parameters and vice versa.
namespace parameter_translation_directions {
enum trans_direction {
	BIO2HW = 0,  //!< transform biological parameters to hw parameters
	HW2BIO = 1 //!< transform hw parameters to biological parameters
	};
}

template<typename input_T, typename output_T>
	static output_T trafo_const(input_T input, std::vector<input_T>& params)
{
	UNUSED(input);
	return static_cast<output_T>(params[0]);
}

template<typename input_T, typename output_T>
	static output_T trafo_linear(input_T input, std::vector<input_T>& params)
{
	return static_cast<output_T>(params[1]*input+params[0]);
}

template<typename input_T, typename output_T>
	static output_T trafo_linear_reverse(input_T input, std::vector<input_T>& params)
{
	return static_cast<output_T>(input-params[0])/params[1];
}

template<typename input_T, typename output_T>
	static output_T trafo_polynomial(input_T input, const std::vector<input_T>& params)
{
	input_T res=0;
	for (size_t i=0; i<params.size(); ++i)
		res += params[i] * pow(input, i);
	return static_cast<output_T>(res);
}

#endif // __HWTRANS_H__
