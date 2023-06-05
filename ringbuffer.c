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
#include <stdlib.h>
#include "ringbuffer.h"

void ringbuffer_init(ringbuffer_t *rb, unsigned int elements)
{
	rb->buffer = (rb_data_t*)malloc(elements*sizeof(rb_data_t));
	rb->data_count = 0;
	rb->read_index = 0;
	rb->write_index = 0;
	rb->size = elements;
}

void ringbuffer_uninit(ringbuffer_t*rb)
{
	free(rb->buffer);
}

int  ringbuffer_get_read_avail(ringbuffer_t *rb)
{
	return rb->data_count;
}

int  ringbuffer_get_write_space(ringbuffer_t *rb)
{
	return rb->size - ringbuffer_get_read_avail(rb);
}

// copies the data over to the ringbuffer and advances the write pointer
// and increases the data_count.
void ringbuffer_write(ringbuffer_t *rb, rb_data_t *data, unsigned int count) 
{
	unsigned int index;

	for (index = 0; index < count; ++index) {
		rb->buffer[rb->write_index] = *(data + index);
		++(rb->data_count);
		rb->write_index++;
		rb->write_index %= rb->size - 1;
	}
}

// returns a pointer into the ringbuffer and increases the read_ptr and 
// decreases the data_count.
void ringbuffer_read(ringbuffer_t *rb, rb_data_t *target, unsigned int count) 
{
	unsigned int tmp_index, index;

	for (index = 0; index < count; ++index) {
		target[index] = rb->buffer[rb->read_index];
		--(rb->data_count);
		++(rb->read_index);
		rb->read_index %= rb->size - 1;
	}
}
