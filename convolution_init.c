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
#include <malloc.h>
#include <math.h>

int convolution_init (convolution_t *conv,
                      int number_of_responses,
                      int number_of_response_channels,
                      response_t **responses,
                      unsigned int chunk_length,
	                  int split_channels_mode)
{
	int index, index2, index3, index4;
	float tmp_float;

	if (conv == 0) return 0;

	tmp_float = 0;

	conv->split_channels_mode = split_channels_mode;

	conv->chunk_length = chunk_length;
	conv->number_of_responses = number_of_responses;
	conv->number_of_response_channels = number_of_response_channels;
//	conv->normalization_factor = sqrt(chunk_length * 2);
	conv->normalization_factor = chunk_length * 6;
//	conv->normalization_factor = (chunk_length * 2)*(chunk_length * 2);


	// allocate FFT buffers
	// as r2c stores only N/2+1 results, we don't need the padded size for this
#ifdef DARWIN
	conv->fft_complex = (fftwf_complex*)malloc (sizeof(fftwf_complex) * (conv->chunk_length + 1));
#else
	posix_memalign(&conv->fft_complex, 16, sizeof(fftwf_complex) * (conv->chunk_length + 1));
#endif


	// this one still needs to be 2 * chunk_length as we zero pad it
#ifdef DARWIN
	conv->fft_real = (float*)malloc (sizeof(float) * 2 * conv->chunk_length);
#else
	posix_memalign(&conv->fft_real, 16, sizeof(float) * 2 * conv->chunk_length);
#endif

	// create fftw plans
	conv->fft_plan_forward = fftwf_plan_dft_r2c_1d (2 * conv->chunk_length, 
	                                           conv->fft_real, conv->fft_complex, 
                                               FFTW_MEASURE);

	conv->fft_plan_backward = fftwf_plan_dft_c2r_1d (2 * conv->chunk_length, 
	                                            conv->fft_complex, conv->fft_real, 
	                                            FFTW_MEASURE);

	// First Step: chunk, pad, fft and store the responses

	// allocate array of response pointers
	conv->fft_responses = (fft_response_t**)malloc (sizeof(fft_response_t*)*conv->number_of_responses);

	// process input responses to padded, chunked, fft'ed internal
	// representation
	conv->max_chunks = 0;
	for (index = 0; index < conv->number_of_responses; ++index) {

		// create new response struct
		conv->fft_responses[index] = (fft_response_t*)malloc (sizeof(fft_response_t));

		conv->fft_responses[index]->length = responses[index]->length;

		// make channel_data array big enough to hold pointers for all channels
		conv->fft_responses[index]->channel_data 
		    = (fftwf_complex**)malloc (sizeof(fftwf_complex*) * conv->number_of_response_channels);

		// how many chunks do we need for this response?
		conv->fft_responses[index]->number_of_chunks 
		    = (int)ceil((responses[index]->length)/(float)(conv->chunk_length));

		// new maximum?
		if (conv->fft_responses[index]->number_of_chunks > conv->max_chunks) {
			conv->max_chunks = conv->fft_responses[index]->number_of_chunks;
		}

		// allocate per channel data buffer)
		// no need for double size, as we use r2c/c2r
		for (index2 = 0; index2 < conv->number_of_response_channels; ++index2) {
#ifdef DARWIN
			conv->fft_responses[index]->channel_data[index2]
			    = (fftwf_complex*)malloc(sizeof(fftwf_complex) 
                                         * (conv->chunk_length + 1)
                                         * conv->fft_responses[index]->number_of_chunks);
#else
			posix_memalign(&conv->fft_responses[index]->channel_data[index2],
			               16,
			               sizeof(fftwf_complex) 
			               * (conv->chunk_length + 1)
			               * conv->fft_responses[index]->number_of_chunks);
#endif
		}

		// for each  channel
		for (index2 = 0; index2 < conv->number_of_response_channels; ++index2) {
	
			// for each chunk
			for (index3 = 0; index3 < conv->fft_responses[index]->number_of_chunks; ++index3) {

				// copy original chunk to fft_real
				for (index4 = 0; index4 < conv->chunk_length; ++index4) {

					if (index4 + index3 * conv->chunk_length < conv->fft_responses[index]->length) {

						conv->fft_real[index4]
//						        = responses[index]->channel_data[index2][index4+index3*chunk_length] / conv->normalization_factor;
						        = responses[index]->channel_data[index2][index4+index3*chunk_length];
					}
					else {
						conv->fft_real[index4] = 0;
					}

					// and zero pad the second half of the chunk at the same time
					conv->fft_real[index4+conv->chunk_length] = 0;
				}
			
				// do tha fft
				fftwf_execute (conv->fft_plan_forward);

				// copy fft output to our privat chunk store :)
				for (index4 = 0; index4 < conv->chunk_length + 1; ++index4) {

					conv->fft_responses[index]->channel_data[index2][index4+index3 * (conv->chunk_length + 1)][0]
//						    = conv->fft_complex[index4][0] / conv->normalization_factor;
						    = conv->fft_complex[index4][0];

					conv->fft_responses[index]->channel_data[index2][index4+index3 * (conv->chunk_length + 1)][1]
//						    = conv->fft_complex[index4][1] / conv->normalization_factor;
						    = conv->fft_complex[index4][1];
					
				}				
			}
		}
	}

	// Second Step:
	// setup ringbuffers for storing FFT'ed input.. 
	// the chunks are size chunk_length + 1 thus
	// we need number_of_chunks * (chunk_length + 1)
	// frames

	// one for each response, but one for every channel in every response
	// in split_channel_mode
	if (conv->split_channels_mode) {
		// printf("%i %i\n", conv->number_of_responses, conv->number_of_response_channels);
		conv->input_chunk_ringbuffers = (fftwf_complex**)malloc (sizeof(fftwf_complex*) * conv->number_of_responses * conv->number_of_response_channels);
		for (index = 0; index < conv->number_of_responses * conv->number_of_response_channels; ++index) {
#ifdef DARWIN
			conv->input_chunk_ringbuffers[index] 
			    = (fftwf_complex*)malloc (sizeof(fftwf_complex) * conv->fft_responses[index]->number_of_chunks
			                              * (conv->chunk_length + 1));
#else
			// need to divide (integer divide) the index by number_of_responses to get
			// a valid index into the list of responses.
			posix_memalign(&conv->input_chunk_ringbuffers[index], 16,
			               sizeof(fftwf_complex) * conv->fft_responses[index/conv->number_of_response_channels]->number_of_chunks
			               * (conv->chunk_length + 1));
#endif
			// zero out
			for (index2 = 0; index2 < (conv->chunk_length + 1) * conv->fft_responses[index/conv->number_of_response_channels]->number_of_chunks; ++index2) {
			
				conv->input_chunk_ringbuffers[index][index2][0] = 0;
				conv->input_chunk_ringbuffers[index][index2][1] = 0;
			}
		}
	}
	else {
		conv->input_chunk_ringbuffers = (fftwf_complex**)malloc (sizeof(fftwf_complex*)*conv->number_of_responses);
		for (index = 0; index < conv->number_of_responses; ++index) {
#ifdef DARWIN
			conv->input_chunk_ringbuffers[index] 
			    = (fftwf_complex*)malloc (sizeof(fftwf_complex) * conv->fft_responses[index]->number_of_chunks
			                              * (conv->chunk_length + 1));
#else
			posix_memalign(&conv->input_chunk_ringbuffers[index], 16,
			               sizeof(fftwf_complex) * conv->fft_responses[index]->number_of_chunks
			               * (conv->chunk_length + 1));

#endif
			// zero out
			for (index2 = 0; 
			     index2 < (conv->chunk_length + 1) * conv->fft_responses[index]->number_of_chunks; 
			     ++index2) {
			
				conv->input_chunk_ringbuffers[index][index2][0] = 0;
				conv->input_chunk_ringbuffers[index][index2][1] = 0;
			}
		}
	}



	
	// allocate input_chunk_ringbuffer_index arrays
	conv->input_chunk_ringbuffer_indexes = (int*)malloc (sizeof(int) * conv->number_of_responses);

	// zero out	
	for (index = 0; index < conv->number_of_responses; ++index) {
			conv->input_chunk_ringbuffer_indexes[index] = 0;
	}

	// Third Step: setup overlap buffers
	// these need only be chunk_length sized

	conv->overlap_buffers = (float**)malloc (sizeof(float*) * conv->number_of_response_channels);
	for (index = 0; index < conv->number_of_response_channels; ++index) { 
#ifdef DARWIN
		conv->overlap_buffers[index] = (float*)malloc (sizeof(float) * conv->chunk_length);
#else
		posix_memalign(&conv->overlap_buffers[index], 16, sizeof(float) * conv->chunk_length);
#endif
		// zero out
		for (index2 = 0; index2 < conv->chunk_length; ++index2) {
		conv->overlap_buffers[index][index2] = 0;
		}
	}

	return 1;
}
