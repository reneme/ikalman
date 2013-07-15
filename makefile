CC = clang
CFLAGS = -O3 -std=c99
CXX = clang++
CXXFLAGS = -O3 -std=c++11 -stdlib=libc++
KOBJECTS = matrix.o kalman.o gps.o
BINARY = hike

%.o : %.c
	$(CC) $(CFLAGS) -c $<

all : $(KOBJECTS)
	$(CXX) $(CXXFLAGS) -o $(BINARY) -I external/rapidxml smooth_hiking.cpp $(KOBJECTS)

clean :
	rm -f *.o $(BINARY)