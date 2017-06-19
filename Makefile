COMPILER = g++
FLAGS = -std=c++14 -O3 #-Wall -Werror -Wextra

SO_DEPS = $(shell pkg-config --libs --cflags libSimpleAmqpClient msgpack librabbitmq opencv)
SO_DEPS += -lboost_program_options -lpthread -lyaml-cpp
SO_DEPS += -lnana -lX11 -lpthread -lrt -ldl -lXft -lpng -lfontconfig -lstdc++fs

TARGETS = 4camera-viewer set-parameters get-parameters set-from-file slider-configure

all: $(TARGETS)

clean:
	rm -f $(TARGETS)
	
4camera-viewer: src/4camera-viewer.cpp
	$(COMPILER) $^ -o $@ $(FLAGS) $(SO_DEPS)

set-parameters: src/set-parameters.cpp
	$(COMPILER) $^ -o $@ $(FLAGS) $(SO_DEPS)

get-parameters: src/get-parameters.cpp
	$(COMPILER) $^ -o $@ $(FLAGS) $(SO_DEPS)

set-from-file: src/set-from-file.cpp
	$(COMPILER) $^ -o $@ $(FLAGS) $(SO_DEPS)

slider-configure: src/slider-configure.cpp
	$(COMPILER) $^ -o $@ $(FLAGS) $(SO_DEPS)