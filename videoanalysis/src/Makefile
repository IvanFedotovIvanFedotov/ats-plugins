CC = gcc

CFLAGS = -Wall -O3 -march=native -fPIC -Wall -c 
CFLAGS += `pkg-config --cflags gstreamer-1.0 gstreamer-video-1.0 gstreamer-gl-1.0 gl`

LDFLAGS = -shared -Wall
LDFLAGS += `pkg-config --libs gstreamer-1.0 gstreamer-video-1.0 gstreamer-gl-1.0 glib-2.0 gl`

PY=python3

all: videoanalysis.o
	@$(CC) $(LDFLAGS) videoanalysis.o -o ../../build/libvideoanalysis.so

error.o:
	@$(CC) $(CFLAGS) error.c -o error.o

videodata.o: analysis.h
	@$(CC) $(CFLAGS) videodata.c -o videodata.o

videoanalysis.o: #analysis.h
	@$(CC) $(CFLAGS) gstvideoanalysis.c -o videoanalysis.o

analysis.h:
	@$(PY) generate_array.py analysis.h.template analysis.h

clean:
	rm *.o
