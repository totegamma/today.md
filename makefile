PROG := today
SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:%.cpp=%.o)
DEPS := $(SRCS:%.cpp=%.d)

CC := g++
CCFLAGS := -std=c++17
INCLUDEPATH := -I lib/CLI11/include/
LIBPATH := 
LIBS := 

all: $(DEPENDS) $(PROG)

$(PROG): $(OBJS)
		$(CC) $(CCFLAGS) -o $@ $^ $(LIBPATH) $(LIBS)

.cpp.o:
		$(CC) $(CCFLAGS) $(INCLUDEPATH) -MMD -MP -MF $(<:%.cpp=%.d) -c $< -o $(<:%.cpp=%.o)

.PHONY: clean
	clean:
		$(RM) $(PROG) $(OBJS) $(DEPS)

-include $(DEPS)
