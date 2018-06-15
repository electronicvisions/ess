#include "common.h"

#include <sysc/kernel/sc_object_manager.h>

Debug dbg;

void print_systemc_object_tree(std::ostream & out)
{
	out << "// begin systemc objects:\n";
	sc_object_manager * manager = sc_get_curr_simcontext()->get_object_manager();
	for(sc_object * o = manager -> first_object();
		o;
		o = manager -> next_object())
	{
		o -> print(out);
		sc_object * par = o -> get_parent();
		if(par)
		{
			out << " -> ";
			par -> print(out);
		}
		out << ";\n";
	}
	out << "// end systemc objects\n";
}
