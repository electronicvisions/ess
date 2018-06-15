#ifndef _merger_config_if_h_
#define _merger_config_if_h_

// pre-declaration
class merger;

/** config interface for a single merger in the spl1-merger tree.
 * offers various methods for configuration the interconnection of mergers.
 */
class merger_config_if {
	public:
		/** connects output register to the input of a target merger.*/
		virtual void connect_output_to(
				merger* target_merger,  //!< target merger
				bool input  //!< input register (0 or 1) of target merger to which this merger connects
				) = 0;

		/** connects output register of a source merger to the input register[input] of this merger.*/
		virtual void connect_input_to(
				merger* source_merger,  //!< source merger
				bool input  //!< specifies to which input register the source merger is connected
				) = 0;

		/* sets select bit.
		 * selects either input 0 or 1*/
		virtual void set_select(
				bool select  //!< input to be selected (0 or 1)
				) = 0;

		/* sets enable bit.
		 * enables merging if true */
		virtual void set_enable(
				bool enable  //!< enable merging
				) = 0;

		/* sets slow bit.
		 * assures that output rate is 1(2*clk-cycle)
		 * if slow = true. otherwise output-rate = 1/clk-cycle*/
		virtual void set_slow(
				bool slow  //!< enable slow mode
				) = 0;
};
#endif //_merger_config_if_h_
