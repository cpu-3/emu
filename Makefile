build: src/main.cpp
	g++ -std=c++14 -O3 src/main.cpp -o emu

test: build
	cd test; ./test.sh
clean:
	rm -f emu
