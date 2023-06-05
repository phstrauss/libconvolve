/*
    Copyright (C) 2004 Florian Schmidt
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.
    
    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software 
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "convolveutil.h"


unsigned int inline input_rb_space(convolution_t *conv, int read_pos, int write_pos) 
{
	if (read_pos >= write_pos)
		return read_pos - write_pos;
	else
		return (read_pos + conv->chunk_length) - write_pos;
}



#define MIN(a, b) (a < b ? a : b)

int convolution_process_buffered(convolution_t *conv,
                                 float **in_data, 
                                 float **out_data,
	                             float gain,
	                             int min_bin,
	                             int max_bin,
	                             unsigned int frames) 
{
	int index, index2, frames_left, tmp, in_index;

	frames_left = frames;
	in_index = 0;

	while (frames_left > 0) {
		tmp = MIN(frames_left, conv->chunk_length - conv->extra_rb_index);

		for (index2 = 0; index2 < conv->number_of_responses; ++index2) {
			for (index = 0; index < tmp; ++index) {
				conv->input_rbs[index2][conv->extra_rb_index + index] = in_data[index2][index + in_index];
			}
		}

		for (index2 = 0; index2 < conv->number_of_response_channels; ++index2) {
			for (index = 0; index < tmp; ++index) {
				out_data[index2][in_index + index] = conv->output_rbs[index2][conv->extra_rb_index + index];
			}
		}
		conv->extra_rb_index += tmp;

		conv->extra_rb_index %= conv->chunk_length;

		if (conv->extra_rb_index == 0) {
			// do that thang input is full, output is empty
			convolution_process(conv, conv->input_rbs, conv->output_rbs, gain, min_bin, max_bin);
		}
		in_index += tmp;
		frames_left -= tmp;
	}
}
