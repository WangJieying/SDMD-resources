CXX = g++
CPPFLAGS = -fPIC -lGL -lGLU -lglut -lGLEW -Wall -Iinclude -lm -Wextra -pedantic -Werror -std=c++14
OUTDIR = obj/



MPATH=$(realpath $(lastword $(MAKEFILE_LIST)))
NOTHING = 
SPACE = $(NOTHING) $(NOTHING)

OUTFILE = main

SOURCE = $(wildcard *.cpp) 
OBJ = $(OUTDIR)$(subst $(SPACE), $(OUTDIR),$(SOURCE:.cpp=.o))

EXTERNAL =

ABSOUT=$(MPATH:Makefile=)$(OUTDIR)

default:lib

lib:$(OBJ)
#Make an output directory:
	mkdir -p $(OUTDIR)
	mkdir -p lib
# Combine all object files into library file.
	@echo Combining all object files into library file Config.a
	@ar rvs lib/Config.a $(OUTDIR)/*.o
#	@echo CONFIG-Library succesfully compiled. Files are in: $(ABSOUT)

all:$(OBJ)
	$(CXX) $(CPPFLAGS) -o $(OUTFILE) $(OBJ) $(EXTERNAL)
	
$(OUTDIR)%.o : %.cpp 
	@mkdir -p $(OUTDIR)
	$(CXX) $(CPPFLAGS) -c $< -o $@

clean:
	$(RM) *.o *.gch
	$(RM) -rf lib
	$(RM) -rf $(OUTDIR)
