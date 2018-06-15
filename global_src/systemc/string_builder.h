//_____________________________________________
// Last Change  :   mehrlich 03/27/2008
// Module Name  :   string_builder
// Filename     :   string_builder.h
// Project Name	:   p_facets/s_systemsim
//_____________________________________________

#ifndef __STRING_BUILDER_H__
#define __STRING_BUILDER_H__

#include "systemc.h"

#include "stdio.h"
#include "stdlib.h"

namespace ess
{

/// This class appends a uint to a string.
/// It overloads the operator sc_module_name() to
/// initalize a module with the string built
struct string_builder
{
	char buffer[1024]; ///< holds the string
	string_builder(const char* str, uint num)
	{
		snprintf(this->buffer, sizeof(this->buffer), "%s%i", str?str:"",num);
	}
	operator const char*() const { return buffer; }
	operator sc_module_name() const { return sc_module_name(buffer); }
};

} // namespace ess

#endif//__STRING_BUILDER_H__
