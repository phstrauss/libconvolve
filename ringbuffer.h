#ifndef RRRINGBUFFER_H
#define RRRINGBUFFER_H

typedef float  rb_data_t;

typedef struct ringbuffer {
	rb_data_t *buffer;
	unsigned int size;

	unsigned int read_index;
	unsigned int write_index;

	unsigned int data_count;
} ringbuffer_t;

void ringbuffer_init(ringbuffer_t *rb, unsigned int elements);

void ringbuffer_uninit(ringbuffer_t*rb);

int  ringbuffer_get_read_avail(ringbuffer_t *rb);

int  ringbuffer_get_write_space(ringbuffer_t *rb);

// copies the source data over to the ringbuffer and advances the write pointer
// and increases the data_count.
void ringbuffer_write(ringbuffer_t *rb, rb_data_t *source, unsigned int count);


// copies count elements to the buffer pointed to by target
void ringbuffer_read(ringbuffer_t *rb, rb_data_t *target, unsigned int count);

#endif
