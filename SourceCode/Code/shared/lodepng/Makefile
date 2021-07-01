CXX = g++
CPPFLAGS = -lGL -lGLU -lglut -lGLEW -Wall -Iinclude -O3 -lm 

default:lodepng.o
all:lodepng.o
	
lodepng.o : lodepng.cpp
	$(CXX) $(CPPFLAGS) -c -I../include $< -o $@

clean:
	$(RM) *.o *.gch
