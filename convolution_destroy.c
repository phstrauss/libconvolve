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

#include "convolve.h"

int convolution_destroy (convolution_t *conv)
{
	int index, index2, input_channels;

	if (conv->split_channels_mode) 
		input_channels = conv->number_of_responses * conv->number_of_response_channels;
	else
		input_channels = conv->number_of_responses;		

	for (index = 0; index < input_channels; ++index) 
		free (conv->input_chunk_ringbuffers[index]);

	for (index = 0; index < conv->number_of_responses; ++index) {

		for (index2 = 0; index2 < conv->number_of_response_channels; ++index2) {

			// free the chunk data
			free (conv->fft_responses[index]->channel_data[index2]);
		}

		free (conv->fft_responses[index]->channel_data);
	}


	for (index = 0; index < conv->number_of_response_channels; ++index) {
		free (conv->overlap_buffers[index]);

	}

	free (conv->overlap_buffers);
	free (conv->input_chunk_ringbuffers);
	free (conv->input_chunk_ringbuffer_indexes);
	
	free (conv->fft_responses);
	free (conv->fft_real);
	fftwf_free (conv->fft_complex);
	
	fftwf_destroy_plan (conv->fft_plan_forward);
	fftwf_destroy_plan (conv->fft_plan_backward);

	return 1;
}
