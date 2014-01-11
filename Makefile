CC   = g++ -g
COMPILE.C  = $(CC) -c 
VPATH   = .
CXXFLAGS  =  -lgcc_s 
LDFLAGS = -lglog -lpthread -ldl -lrt
MAKEEXE   = $(CC)

SRC   = $(wildcard *.cpp)
OBJ   = $(patsubst %.cpp, %.o, $(SRC))
EXE   = amegia_pnp_server

all:   $(EXE)

$(EXE):   ${OBJ}
	$(MAKEEXE) $^ $(LDFLAGS) -o $@

%.o:   %.cpp
	$(COMPILE.C) $< -o $@

clean:
	rm -f *.o core $(EXE) 

