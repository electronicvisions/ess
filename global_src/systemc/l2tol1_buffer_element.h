//_____________________________________________
// Company      :	TU-Dresden
// Author       :	Stefan Scholze
// Refined		: 	Matthias Ehrlich
// E-Mail   	:	scholze@iee.et.tu-dresden.de
//
// Date         :   Tue Apr 18 14:08:03 2007
// Last Change  :   mehrlich 03/27/2008
// Module Name  :   l2tol1_buffer_element
// Filename     :   l2tol1_buffer_element.h
// Project Name	:   p_facets/s_systemsim
// Description	:   layer 1 to layer 2 buffer element
//
//_____________________________________________

#ifndef __L2L1BUFF_H__
#define __L2L1BUFF_H__

namespace ess{
/// This struct provides a Layer2 to Layer1 buffer element.
struct l2tol1_buffer_element{
		bool valid;
		sc_uint <TIMESTAMP_WIDTH+L1_ADDR_WIDTH> value;
		};
}

#endif // __L2L1BUFF_H__

