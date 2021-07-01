
CUDAROOT  = /usr/local/cuda
CUDAINC   = $(CUDAROOT)/include
CUDALIB   = $(CUDAROOT)/lib64
CCFLAGS   = -I. -Iinclude -O3 -I$(CUDAINC) -L/usr/lib/x86_64-linux-gnu -L/usr/local/cuda-10.0/targets/x86_64-linux/lib
#--------------------------------------------------------------------------------------------------------------------------------

CXX  = g++
NVCC = $(CUDAROOT)/bin/nvcc

OBJECTS = skelft.o skelft_core.o

all: skeleton

skeleton: $(OBJECTS)
	$(CXX) -c skelft_core.o $(CCFLAGS) skelft_core.cpp
	$(NVCC) -c skelft.o $(CCFLAGS) -I$(CUDAINC) -use_fast_math -gencode=arch=compute_52,code=\"sm_52,compute_52\" --ptxas-options=-v skelft.cu
	ar rvs CUDASkel2D.a *.o

skelft_core.o: skelft_core.cpp

skelft.o: skelft.cu

clean:
	-rm *.o *.a