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

#ifdef C_CMUL
#else
#include <dsp/dspop.h>
#endif

/* #include <malloc.h> */
#include <math.h>


int convolution_process(convolution_t *conv,
	                    float **in_data, 
	                    float **out_data,
	                    float gain,
	                    int min_bin,
	                    int max_bin)
{
	int index, index2, index3, index4, tmp_rb_chunk_index, number_of_inputs;

	if (min_bin == -1) min_bin = 0;
	if (max_bin == -1) max_bin = conv->chunk_length + 1;

	if (conv->split_channels_mode) 
		number_of_inputs = conv->number_of_responses * conv->number_of_response_channels;
	else
		number_of_inputs = conv->number_of_responses;


	// fft the input chunks
	for (index = 0; index < number_of_inputs; ++index) {
		
		// copy input chunk into fft_real
		for (index2 = 0; index2 < conv->chunk_length; ++index2) {
			
			conv->fft_real[index2] = in_data[index][index2];
//			conv->fft_real[index2] = in_data[index][index2] / conv->normalization_factor;
			
			// and pad at the same time
			conv->fft_real[index2+conv->chunk_length] = 0;
		}

		// do tha fft
		fftwf_execute (conv->fft_plan_forward);

		// copy output into input_chunks ringbuffers
		if (conv->split_channels_mode) {
			for (index2 = 0; index2 < conv->chunk_length + 1; ++index2) {
			
				conv->input_chunk_ringbuffers[index][index2 + conv->input_chunk_ringbuffer_indexes[index/conv->number_of_response_channels]][0]
//				    = conv->fft_complex[index2][0] / conv->normalization_factor;
				    = conv->fft_complex[index2][0];
				conv->input_chunk_ringbuffers[index][index2 + conv->input_chunk_ringbuffer_indexes[index/conv->number_of_response_channels]][1]
//				    = conv->fft_complex[index2][1] / conv->normalization_factor;
				    = conv->fft_complex[index2][1];
			}

		}
		else {
			for (index2 = 0; index2 < conv->chunk_length + 1; ++index2) {
			
				conv->input_chunk_ringbuffers[index][index2 + conv->input_chunk_ringbuffer_indexes[index]][0]
//				    = conv->fft_complex[index2][0] / conv->normalization_factor;
				    = conv->fft_complex[index2][0];
				conv->input_chunk_ringbuffers[index][index2 + conv->input_chunk_ringbuffer_indexes[index]][1]
//				    = conv->fft_complex[index2][1] / conv->normalization_factor;
				    = conv->fft_complex[index2][1];
			}
		}

	} // end of fft'ing inputs

	// do the complex multiplications for all response input channels 
	for (index = 0; index < conv->number_of_response_channels; ++index) {

		// zero out the reverse fft input buffer
		for (index2 = 0; index2 < conv->chunk_length + 1; ++index2) {

			conv->fft_complex[index2][0] = 0;
			conv->fft_complex[index2][1] = 0;
		}

		// for all responses (of this output channel)
		for (index2 = 0; index2 < conv->number_of_responses; ++index2) {
			
			// for all chunks
			for (index3 = 0; index3 < conv->fft_responses[index2]->number_of_chunks; ++index3) {
				
				// we go backward in time (from current chunk in input_chunnks_ringbuffers[]
				// to oldest)
				tmp_rb_chunk_index  = conv->input_chunk_ringbuffer_indexes[index2]
				                      - (index3 * (conv->chunk_length + 1))
				                      + ((conv->chunk_length + 1)
				                         * conv->fft_responses[index2]->number_of_chunks);

				// constraint to the actuall data length ("%")				
				tmp_rb_chunk_index %= (conv->chunk_length + 1)
				                      * conv->fft_responses[index2]->number_of_chunks;

				// we do the complex multiplication only for those bins requested
				if (conv->split_channels_mode) {
#ifdef C_CMUL
					complex_mul(conv->input_chunk_ringbuffers[index2 * conv->number_of_response_channels + index] + tmp_rb_chunk_index + min_bin,
					            conv->fft_responses[index2]->channel_data[index] + index3 * (conv->chunk_length + 1) + min_bin,
					            conv->fft_complex + min_bin,
					            max_bin - min_bin);            
#else
					// pointer arithmetics are fun
					// complex_mul(conv->input_chunk_ringbuffers[index2] + tmp_rb_chunk_index, 
					dsp_cmuladdf((stpSCplx)(conv->fft_complex + min_bin),
					             (stpSCplx)(conv->input_chunk_ringbuffers[index2 * conv->number_of_response_channels + index] + tmp_rb_chunk_index + min_bin), 
					             (stpSCplx)(conv->fft_responses[index2]->channel_data[index] + (index3 * (conv->chunk_length + 1)) + min_bin),
					             max_bin - min_bin);
#if 0
					for (index4 = 0; index4 < conv->chunk_length+1; ++index4) {
						conv->fft_complex[index4][0] /= conv->normalization_factor;
						conv->fft_complex[index4][1] /= conv->normalization_factor;
					}
#endif
#endif
				}
				else {
#ifdef C_CMUL
					complex_mul(conv->input_chunk_ringbuffers[index2] + tmp_rb_chunk_index + min_bin,
					            conv->fft_responses[index2]->channel_data[index] + index3 * (conv->chunk_length + 1) + min_bin,
					            conv->fft_complex + min_bin,
					            max_bin - min_bin);            
#else
					// pointer arithmetics are fun
					// complex_mul(conv->input_chunk_ringbuffers[index2] + tmp_rb_chunk_index, 
					dsp_cmuladdf((stpSCplx)(conv->fft_complex + min_bin),
					             (stpSCplx)(conv->input_chunk_ringbuffers[index2] + tmp_rb_chunk_index + min_bin), 
					             (stpSCplx)(conv->fft_responses[index2]->channel_data[index] + (index3 * (conv->chunk_length + 1)) + min_bin),
					             max_bin - min_bin);
#endif
#if 0
					for (index4 = 0; index4 < conv->chunk_length+1; ++index4) {
						conv->fft_complex[index4][0] /= conv->normalization_factor;
						conv->fft_complex[index4][1] /= conv->normalization_factor;
					}
#endif
				} // else
			} // chunks
		} // responses

		// reverse fft the results
		fftwf_execute (conv->fft_plan_backward);

		// copy to out_buffers, save current overlap and add previous overlap
		for (index2 = 0; index2 < conv->chunk_length; ++index2) {

			out_data[index][index2] = gain * (conv->fft_real[index2] / conv->normalization_factor);
//			out_data[index][index2] = gain * (conv->fft_real[index2]);

			out_data[index][index2] += gain * (conv->overlap_buffers[index][index2]);

			conv->overlap_buffers[index][index2]
			    = conv->fft_real[index2 + conv->chunk_length] / conv->normalization_factor;
//			    = conv->fft_real[index2 + conv->chunk_length];
		}
	} // channels

	// advance input_chunk_ringbuffer_indexes
	for (index = 0; index < conv->number_of_responses; ++index) {

		conv->input_chunk_ringbuffer_indexes[index] += conv->chunk_length + 1;	
		conv->input_chunk_ringbuffer_indexes[index] %= conv->fft_responses[index]->number_of_chunks
		                                               * (conv->chunk_length + 1);
	}
	return 1;
}
