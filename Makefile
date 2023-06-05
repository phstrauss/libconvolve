PREFIX=/opt/av

#COMPILE_FLAGS = -fPIC $(pkg-config fftw3f samplerate sndfile --cflags) -DDARWIN
COMPILE_FLAGS = $(CFLAGS) -fPIC -DDARWIN
#COMPILE_FLAGS += -O3 -funroll-loops -funroll-all-loops 
COMPILE_FLAGS += -g

#LINK_FLAGS = $(pkg-config fftw3 samplerate sndfile --libs) 
LINK_FLAGS = $(LDFLAGS)

# uncomment the following line if libDSP is not available for
# your platform (power pc etc.)...
COMPILE_FLAGS += -DC_CMUL

# ...and comment this one out:
#LINK_FLAGS += -L/usr/local/lib -ldsp -lstdc++

ST_TARGET = libconvolve.a

#TARGET = libconvolve.so.0.0.8
#SONAME = libconvolve.so.0
#SMALLNAME = libconvolve.so

TARGET = libconvolve-0.0.8.dylib
SONAME = libconvolve-0
SMALLNAME = libconvolve.dylib

STUFF = complex_mul convolution_init convolution_process convolution_destroy auto_remove_silence load_response libconvolve_init ringbuffer
# threaded_convolve

SOURCES = $(STUFF:%=%.c)
OBJECTS = $(STUFF:%=%.o)
HEADERS = convolve.h

all: $(TARGET) $(ST_TARGET)

$(TARGET): $(OBJECTS) $(HEADERS)
	/usr/bin/libtool -dynamic -current_version 0.0.8 -compatibility_version 0 $(LINK_FLAGS) -o $(TARGET) $(OBJECTS)
#	$(CC) -shared -Wl,-soname,$(SONAME) -o $(TARGET) $(OBJECTS) $(LINK_FLAGS)

$(ST_TARGET): $(OBJECTS) $(HEADERS)
	/usr/bin/libtool -static -o $(ST_TARGET) $(OBJECTS)
	#ar rcs $(ST_TARGET) $(OBJECTS)

$(OBJECTS): %.o: %.c 
	$(CC) -c $< $(COMPILE_FLAGS)

ringbuffer_test: ringbuffer.o ringbuffer_test.c convolve.c
	$(CC) -o ringbuffer_test ringbuffer_test.c ringbuffer.o -O3

.PHONY: clean
clean:
	rm -f $(TARGET) *~ *.o *.a core* *.lst ringbuffer_test
	rm -rf doc/*

.PHONY: doc
doc:
	doxygen doxygen.conf

.PHONY: install
install: $(TARGET)
	cp $(TARGET) $(PREFIX)/lib/
	cp $(ST_TARGET) $(PREFIX)/lib/
	cp convolve.h $(PREFIX)/include/
	#ldconfig -n $(PREFIX)/lib
	ln -s $(PREFIX)/lib/$(SONAME) $(PREFIX)/lib/$(SMALLNAME) || true
