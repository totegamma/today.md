PROG := today
SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:%.cpp=%.o)
DEPS := $(SRCS:%.cpp=%.d)

CC := g++-10
CCFLAGS := -std=c++2a
INCLUDEPATH := -I lib/CLI11/include/
LIBPATH := 
LIBS := 
INSTALLPATH := /usr/local/bin

all: $(DEPENDS) $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CCFLAGS) -o $@ $^ $(LIBPATH) $(LIBS)

.cpp.o:
	$(CC) $(CCFLAGS) $(INCLUDEPATH) -MMD -MP -MF $(<:%.cpp=%.d) -c $< -o $(<:%.cpp=%.o)

.PHONY: clean
clean:
	$(RM) $(PROG) $(OBJS) $(DEPS)

install: $(PROG)
	install $(PROG) $(INSTALLPATH)

-include $(DEPS)
