PROG = clown
CC = ccache g++
CPPFLAGS = -w -lSDL2 -lSDL2_image -lSDL2_gfx -lGL -lGLEW -Wall 
OBJS = main.o

$(PROG) : $(OBJS)
	$(CC) $(CPPFLAGS) -o $(PROG) $(OBJS)
main.o : main.cpp
	$(CC) $(CPPFLAGS) -c main.cpp
