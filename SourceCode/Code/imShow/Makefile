MPATH=$(realpath $(lastword $(MAKEFILE_LIST)))
CXX = g++
CUDAROOT  = /usr/local/cuda
CUDAINC   = $(CUDAROOT)/lib64

INCLUDES = -Iinclude -I/usr/local/cuda/include -I../shared -I../shared/include -I../shared/FastAC -I../shared/squash/squash -I../shared/squash -I../shared/SplineGenerate -L../shared/squash/squash -Wl,-rpath=../shared/squash/squash -L$(CUDAINC) -L/usr/lib/x86_64-linux-gnu -L/usr/local/cuda-10.0/targets/x86_64-linux/lib 
CPPFLAGS = -Wall -lm -Wno-variadic-macros -std=c++11 -g
CPPFLAGS += $(INCLUDES)
CUDAFLAGS = -lcudart -lcuda -lGL -lGLU -lGLEW -lglut

LFLAG=-L../shared/SplineGenerate/lib/configuration -lgtapplications -lgtgraphics -lgtmathematicsgpu -lX11 -lXext -lGL -lEGL -lpng -lpthread -lm 

NOTHING = 
SPACE = $(NOTHING) $(NOTHING)

CPPFLAGS += -pg -g -O3 -Wall -Wextra -pedantic 
#ifdef GPROF
#	DBG = 1
#	CPPFLAGS += -pg
#endif
#
#ifdef DBG
#	CPPFLAGS += -Wall -pedantic -g 
#else
#	CPPFLAGS += -O3
#endif


#ADD VERBOSE
ifdef VERBOSE
	CPPFLAGS += -DSHOW_DEBUG_OUTPUT=1
endif

OUTDIR = obj/
OUTFILE = show_skeleton

SOURCE = $(wildcard *.cpp)
OBJ = $(OUTDIR)$(subst $(SPACE), $(OUTDIR),$(SOURCE:.cpp=.o))
EXTERNAL = ../shared/CUDASkel2D/CUDASkel2D.a ../shared/FastAC/arithmetic_codec.o ../shared/lodepng/lodepng.o ../shared/squash/squash/libsquash0.8.so ../shared/SplineGenerate/BSplineCurveFitter/configuration/BSplineCurveFitterWindow3.o

ABSOUT=$(MPATH:Makefile=)$(OUTDIR)

default:all

all:external $(OBJ)
	$(CXX) $(CPPFLAGS) -o $(OUTFILE) $(OBJ) $(EXTERNAL) $(CUDAFLAGS) $(LFLAG)
	
$(OUTDIR)%.o : %.cpp
	@mkdir -p $(OUTDIR)
	$(CXX) $(CPPFLAGS) -c $< -o $@

external:
	make -C ../shared/CUDASkel2D -f Makefile skeleton
	make -C ../shared/squash -f Makefile	
	make -C ../shared/lodepng -f Makefile
	make -C ../shared/SplineGenerate/BSplineCurveFitter CFG=configuration APP=BSplineCurveFitter -f ../makesample.gte
	g++ -g ../shared/FastAC/arithmetic_codec.cpp -c -o ../shared/FastAC/arithmetic_codec.o

clean:
	$(RM) *.gch
	$(RM) ../shared/FastAC/arithmetic_codec.o
	$(RM) $(OUTDIR)/*
	$(RM) -r $(OUTDIR)
	$(RM) -rf lib
	$(RM) $(OUTFILE)

clean-full: clean
	make -C ../shared/CUDASkel2D -f Makefile clean
	make -C ../shared/squash -f Makefile clean
	make -C ../shared/lodepng -f Makefile clean
