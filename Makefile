default:
	g++ main.cc screen.cc world.cc -o vfk -g -std=c++0x -lSDL2 -lGLEW -lGL
	./vfk

