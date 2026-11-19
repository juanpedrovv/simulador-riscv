CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra
FUENTES  = $(wildcard src/*.cpp)

build/rvsim: $(FUENTES) $(wildcard src/*.h)
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(FUENTES) -o $@

test: build/rvsim
	./pruebas/correr.sh

clean:
	rm -rf build

.PHONY: test clean
