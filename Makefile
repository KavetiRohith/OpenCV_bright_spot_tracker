INCLUDE_DIRS = -I/usr/include/opencv4
LIB_DIRS = 
CC=g++

CDEFS=
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= -L/usr/lib -lopencv_core -lopencv_flann -lopencv_video -lrt

HFILES= 
CFILES= track.cpp

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.cpp=.o}

all: track trackv2 debugdir compute_median_image

clean:
	-rm -f *.o *.d
	-rm -f track trackv2 compute_median_image

track: track.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o `pkg-config --libs opencv4` $(LIBS)

compute_median_image: compute_median_image.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o `pkg-config --libs opencv4` $(LIBS)

trackv2: trackv2.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o `pkg-config --libs opencv4` $(LIBS)

debugdir:
	-mkdir -p DEBUG_IMGS
	-rm -f DEBUG_IMGS/*
	

depend:

.cpp.o: $(SRCS)
	$(CC) $(CFLAGS) -c $<
