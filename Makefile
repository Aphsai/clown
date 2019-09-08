PROG = clown
CC = ccache g++
CPPFLAGS = -w -lSDL2 -lSDL2_image -lSDL2_gfx -lGL -lGLEW -Wall 
OBJS = main.o ./ecs/system-manager.hpp ./ecs/component-manager.hpp ./ecs/coordinator.hpp entity-manager.o ./ecs/ecs.hpp game.o

$(PROG) : $(OBJS)
	$(CC) $(CPPFLAGS) -o $(PROG) $(OBJS)
main.o : main.cpp
	$(CC) $(CPPFLAGS) -c main.cpp
entity-manager.o : ecs/entity-manager.cpp ecs/entity-manager.hpp
	$(CC) $(CPPFLAGS) -c ecs/entity-manager.cpp ecs/entity-manager.hpp
component-manager.hpp.gch : ecs/component-manager.hpp
	$(CC) $(CPPFLAGS) -c ecs/component-manager.hpp
coordinator.hpp.gch : ecs/coordinator.hpp
	$(CC) $(CPPFLAGS) -c ecs/coordinator.hpp
ecs.hpp.gch : ecs/ecs.hpp
	$(CC) $(CPPFLAGS) -c ecs/ecs.hpp
system-manager.hpp.gch : ecs/system-manager.hpp
	$(CC) $(CPPFLAGS) -c ecs/system-manager.hpp
game.o : game.cpp game.hpp
	$(CC) $(CPPFLAGS) -c game.cpp
