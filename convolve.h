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

#ifndef LIBCONVOLVE_H
#define LIBCONVOLVE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <fftw3.h>




/*	
	type for specifying a [multichannel] response.
	this is user managed [not opaque] 
*/
struct convolution_response {
	int channels;

	unsigned int length;

	/*	
		an array of pointers to float data
		holding the individual response channels.
		allocated by user 
	*/
	float **channel_data;
};
typedef struct convolution_response response_t;





/*	
	type for specifying a [multichannel] response.
	this is for internal use only 
*/
struct fft_response {
	int channels;

	unsigned int length;

	/*	
		an array of pointers to fftwf_complex data
		holding the individual response channels. 
	*/
	fftwf_complex **channel_data;

	int number_of_chunks;
};
typedef struct fft_response fft_response_t;






/*	
	"opaque" type only to be accessed by the below functions. all members
	strictly for internal use. 
*/
struct convolution {
	int chunk_length;

	float normalization_factor;

	/* 
		the number of chunks needed for the longest 
		response 
	*/
	int max_chunks;

	/* 
		the chopped up, padded and fft'ed responses 
	*/
	int number_of_responses;
	int number_of_response_channels;
	fft_response_t **fft_responses;
	
	/*	
		array of ringbuffers for the fft'ed input.
		in split_channels_mode these are
		organized:
		response0,channel0
		response0,channel1
		..
		response0,channelN
		..
		responseM,channelN 
	*/
	fftwf_complex **input_chunk_ringbuffers;

	/*	
		array of indexes into the input_chunk_ringbuffers
		used to store past chunks which then get convolved
		with the corresponding response chunks. 
	*/
	int *input_chunk_ringbuffer_indexes;

	/*	
		array of overlap buffers 
	*/
	float **overlap_buffers;

	/*	
		we need buffers for the FFT 
	*/
	float *fft_real;
	fftwf_complex *fft_complex;

	/*	
		the plans for our forward and backward FFT's 
	*/
	fftwf_plan fft_plan_forward;
	fftwf_plan fft_plan_backward;

	int split_channels_mode;
};
typedef struct convolution convolution_t;




/*	
	this needs to be called once per app lifetime 
*/
void libconvolve_init();




/*	
	returns non zero on succes, zero on failure..

	conv must be not null and point to allocated space
	of size sizeof(convoution_t)

	responses is an array of pointers to response_t
	structs allocated and filled by the user. The 
	number of response_t pointers must
	equal the argument number_of_responses.
	this array and its contents can be freed after
	convolution_init has returned as it uses an
	internal FFT'ed, zero padded version now.

	chunk_length _must_ be a power of two . This
	is the number of frames consumed/produced
	by a convolution_process() call

	number_of_response_channes must be equal
	for all responses. This is because 
	all convolution outputs are mixed.
	This approach saves some IFFT's.

	If you don't want the outputs of your 
	convolutions to be mixed, use a second
	instance of convolution_t. Then you can 
	convolution_process() all responses 
	individually

	set split_channels to 1 to get pseudomultichannel
	behaviour. If you choose to use this you must provide
	number_of_responses * number_of_response_channels 
	buffers worth of input data (for each channel of
	each response) to the convolution_process() function.
	set to 0 for normal one-input-per-response behaviour 
*/
int convolution_init (convolution_t *conv,
                      int number_of_responses,
                      int number_of_response_channels,
                      response_t **responses,
                      unsigned int chunk_lenngth,
	                  int split_channels_mode);




/*
	returns non zero on succes, zero on failure..

	frees all internal memory. the arrays
	in_data, out_data and responses need to be
	freed by the user (including all buffers 
	pointed to by the pointers in the arrays). 
*/
int convolution_destroy (convolution_t *conv);                      




/*
	returns non zero on succes, zero on failure..

	in_data is an array of pointers to float buffers
	allocated by the user. the number of in_data buffers must
	be equal to the number of responses. In split_channel mode
	it must be equal to the number of responses multiplied with 
	the number of channels. The number of out_data buffers 
	must be equal to the channel count which this convolution_t 
	*conv was initialized with.

	out_data is an array of pointers to float buffers
	allocated by the user. the number of buffers must
	be equal to the number of channels which was passed
	to convolution_init.

	process one process_buffer_size of frames.
	The buffers are user managed. Make sure they are
	big enough to hold chunk_length frames

	gain is a factor which will be applied to all
	output..

	min_bin and max_bin determine which bins of the fft
	are actually used. The rest is filled with 0's
	the range for both is 0..periodsize+1,
	where min_bin must always be < max_bin.
	passing -1 yields the default (0 and chunk_length+1)
*/
int convolution_process(convolution_t *conv, 
                        float **in_data, 
                        float **out_data,
						float gain,
	                    int min_bin,
	                    int max_bin);

/* 
	this function loads a responsefile from a format supported by
	libsndfile and resamples it if nessecary.
	
	response_t struct response is usermanaged (needs to be allocate before
	calling this function. 

	samplerate argument is the nominal samplerate. response files will be 
	resampled tomatch this as nessecary

	return code:

	1 - success
	0 - error
*/
int load_response(response_t *response, char *filename, unsigned int samplerate);




/*
	this funtion tries to detect long parts of silence at the
	start (or end) of the response.

	response_t struct response is usermanaged (needs to be allocate before
	calling this function. 

	the threshold is the absolute sample value above which the signal has
	to go to trigger the fade in/out.

	then a fade with fade_time samples is applied and the sample data truncated

	the startend parameter:

	 1 - start mode
	-1 - end mode

	in start mode the function starts to look from the start
	of the sample data and goes forward. in end mode it starts
	at the end and goes backward.
*/
void auto_remove_silence(response_t *response, float threshold, unsigned int fade_time, int startend);



#ifdef __cplusplus
}
#endif

#endif
