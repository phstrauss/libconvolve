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
    
    Philippe Strauss 2011: added if (... || samplerate == 0) to bypass risk of beeing resampled,
    especially to avoid an chicken and egg problem with jack
*/

#include "convolve.h"
#include <sndfile.h>
#include <samplerate.h>
#include <stdlib.h>
#include <math.h>

int load_response(response_t *response, char *filename, unsigned int samplerate) {
	struct SF_INFO  sf_info;
	SNDFILE        *response_file;
	float         **channel_data;
	float          *frame;
	SRC_DATA        src_data;
	unsigned int    index, index2;

	/* open response file */
	response_file = sf_open (filename, SFM_READ, &sf_info);	
	if (response_file == 0) {
		printf("[libconvolve::load_response]: couldn't load response file: %s\n", filename);
		return 0;
	}

	response->channels = sf_info.channels;

	/* allocate enough space so we can hold the data */
	channel_data = (float**)malloc(sf_info.channels * sizeof(float*));
	for (index = 0; index < sf_info.channels; ++index) {
		channel_data[index] = (float*)malloc(sf_info.frames * sizeof(float));
	}

	/* read data into channel_data array */
	frame = (float*)malloc(sf_info.channels * sizeof(float));
	for (index = 0; index < sf_info.frames; ++index) {
		/* read one frame from soundfile */
		if (sf_readf_float(response_file, frame, 1) != 1) {
			/* ouch error. bailing out */
			printf("[libconvolve::load_response]: problem reading the responsefile\n");

			/* clean up in case of errror */
			free(frame);
			for (index2 = 0; index2 < sf_info.channels; ++index2) {
				free(channel_data[index2]);
			}
			free(channel_data);
			sf_close(response_file);
			return 0;
		}
		/* store it in the channel data array */
		for (index2 = 0; index2 < sf_info.channels; ++index2) {
			channel_data[index2][index] = frame[index2];
		}
	}

	response->channel_data = (float**)malloc(response->channels * sizeof(float*));


	/* setup length info regarding to whether we have to resample */
	if (sf_info.samplerate == samplerate || samplerate == 0) {
		/* allocate space accordingly */
		response->length = sf_info.frames;
		for (index = 0; index < response->channels; ++index) {
			response->channel_data[index] = (float*)malloc(sf_info.frames * sizeof(float));
		}
	} else {
		/* ditto */
		response->length = (unsigned int)ceil((double)sf_info.frames * (double)samplerate / (double)sf_info.samplerate);
		for (index = 0; index < response->channels; ++index) {
			response->channel_data[index] = (float*)malloc(response->length * sizeof(float));
		}
			
	}

	sf_close(response_file);

	/* either copy channel_data straight or resample */
	if (sf_info.samplerate == samplerate || samplerate == 0) {
		/* straight copy */
		for (index = 0; index < response->channels; ++index) {
			for (index2 = 0; index2 < sf_info.frames; ++index2) {
				response->channel_data[index][index2] = channel_data[index][index2];
			}
		}
	} else {
		/* need to resample */
		printf("[libconvolve::load_response]: resampling response file...");
		fflush(stdout);
		for (index = 0; index < response->channels; ++index) {
			src_data.data_in       = channel_data[index];
			src_data.data_out      = response->channel_data[index];
			src_data.input_frames  = sf_info.frames;
			src_data.output_frames = response->length;
			src_data.src_ratio     = (float)(response->length) / (float)sf_info.frames;

			src_simple(&src_data, SRC_SINC_BEST_QUALITY, 1);
		}
		printf("done\n");
	}

	for (index = 0; index < sf_info.channels; ++index) 
		free(channel_data[index]);

	free(channel_data);
	return 1;
}
