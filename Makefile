CC   = g++
VPATH   = .
CXXFLAGS  =  -lgcc_s 
LDFLAGS = -lglog -lpthread -ldl -lrt
MAKEEXE   = $(CC) -g
AR        = ar cr 

SRC_ALL = $(wildcard *.cpp)
SRC   = $(filter-out main.cpp,$(SRC_ALL))
OBJ   = $(patsubst %.cpp, %.o, $(SRC))
EXE   = amegia_pnp_server
LIB   = libamegiapnp.a

all:   $(EXE) $(LIB)

$(EXE):   main.o $(LIB)
	$(MAKEEXE) $^ $(LDFLAGS) -o $@

$(LIB):   ${OBJ}
	$(AR) $@ $(OBJ)

%.o:   %.cpp
	$(CC) -c $< -o $@

clean:
	rm -f *.o core $(LIB) $(EXE) 

