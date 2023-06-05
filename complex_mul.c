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

#include <fftw3.h>

void inline complex_mul(fftwf_complex *in1, fftwf_complex *in2, fftwf_complex *result, unsigned int count) {

	unsigned int index;

	for (index = 0; index < count; ++index) {

		result[index][0] += in1[index][0] * in2[index][0] - in1[index][1] * in2[index][1];
		result[index][1] += in1[index][0] * in2[index][1] + in1[index][1] * in2[index][0];
	}
}
