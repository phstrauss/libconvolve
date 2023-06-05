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
#include <stdio.h>


int main() {
	int index1, index2, tmp1, tmp2;
	ringbuffer_t rb;
	float tmp;
	ringbuffer_init(&rb, 1000);
	srand(1);	

	// test run of 2000 write/reads
	for (index1 = 0; index1 < 20; index1++) {	

		tmp1 = (int)(1000.0*(((float)rand())/(float)RAND_MAX));
		printf("run %i  write_space: %i   read_avail: %i\n", tmp1, ringbuffer_get_write_space(&rb), ringbuffer_get_read_avail(&rb));
		for (index2 = 0; index2 < tmp1; ++index2) {
			tmp = (float)index2;
			ringbuffer_write(&rb, &tmp, 1);
		}
		printf("run %i  write_space: %i   read_avail: %i\n", tmp1, ringbuffer_get_write_space(&rb), ringbuffer_get_read_avail(&rb));
		for (index2 = 0; index2 < tmp1; ++index2) {
			ringbuffer_read(&rb, &tmp, 1);
			printf("%i.", (int)tmp);
		}
		printf("\n");
	}
	
	return EXIT_SUCCESS;
}
