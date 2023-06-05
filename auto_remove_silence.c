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
#include <math.h>

void auto_remove_silence(response_t *response, float threshold, unsigned int fade_time, int startend) {
	int index, index2;

	// this is the frame where the threshold is violated
	int thresh_index;

	// this is that start frame for the fade
	int start_index;

	int found;

	// start looking for where the silence stops.
	// either from the start or from the end
	if (startend == 1) {
		// forward from start
		found = 0;
		thresh_index = 0;
		index = 0;
		while (!found && index < response->length) {
			for (index2 = 0; index2 < response->channels; ++index2) {
				if ((fabsf(response->channel_data[index2][index]) > threshold) && (thresh_index == 0))
					thresh_index = index;
					found = 1;
			}
			index++;
		}
	
		// ok, got him. Now do the fade thing. We start fade_time samples before 
		// the thresh_index
		start_index = thresh_index - fade_time;

		if (start_index < 0) {
			fade_time += start_index;
			start_index = 0;
		}
		
		for (index = 0; index < fade_time; ++index) {
			for (index2 = 0; index2 < response->channels; ++index2) {
				response->channel_data[index2][start_index + index] = response->channel_data[index2][start_index + index] * (float)index/(float)fade_time;
			}
		}
	} else {
		// backward from start
		found = 0;
		thresh_index = response->length - 1;
		index = response->length - 1;
		while (!found && index >= 0) {
			for (index2 = 0; index2 < response->channels; ++index2) {
				if ((fabsf(response->channel_data[index2][index]) > threshold) && (thresh_index == 0))
					thresh_index = index;
			}
			index--;
		}
	
		// ok, got him. Now do the fade thing. We start fade_time samples before 
		// the thresh_index
		start_index = thresh_index + fade_time;

		if (start_index >= response->length) {
			fade_time += response->length - start_index;
			start_index = response->length - 1;
		}
		
		for (index = 0; index < fade_time; ++index) {
			for (index2 = 0; index2 < response->channels; ++index2) {
				response->channel_data[index2][start_index - index] = response->channel_data[index2][start_index - index] * (float)index/(float)fade_time;
			}
		}
		
		response->length = start_index + 1;
	}
}
