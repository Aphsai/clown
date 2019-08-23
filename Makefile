PROG = clown
CC = ccache g++
CPPFLAGS = -w -lSDL2 -lSDL2_image -lSDL2_gfx -lGL -lGLEW -Wall 
OBJS = main.o system-manager.gch component-manager.gch coordinator.gch entity-manager.gch

$(PROG) : $(OBJS)
	$(CC) $(CPPFLAGS) -o $(PROG) $(OBJS)
main.o : main.cpp
	$(CC) $(CPPFLAGS) -c main.cpp
system-manager.gch : ecs/system-manager.hpp
	$(CC) $(CPPFLAGS) -c ecs/system-manager.hpp
entity-manager.o : ecs/entity-manager.cpp ecs/entity-manager.hpp
	$(CC) $(CPPFLAGS) -c ecs/entity-manager.cpp ecs/entity-manager.hpp
component-manager.gch : ecs/component-manager.hpp
	$(CC) $(CPPFLAGS) -c ecs/component-manager.hpp
coordinator.gch : ecs/coordinator.hpp
	$(CC) $(CPPFLAGS) -c ecs/coordinator.hpp


