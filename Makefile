program_NAME := lightboard
program_C_MAIN_SRCS := $(wildcard src/*.c)
program_C_LIB_SRCS := $(wildcard lib/*.c)
program_C_SRCS := $(program_C_MAIN_SRCS) $(program_C_LIB_SRCS)
CFLAGS = `pkg-config --cflags gtk+-3.0`#-O2
LDFLAGS = -ludev -lpthread -lX11 -lXtst `pkg-config --libs gtk+-3.0`
program_OBJS := ${program_C_SRCS:.c=.o}

.PHONY: all clean distclean

all: $(program_NAME)

$(program_NAME): $(program_OBJS)
	gcc $(program_OBJS) -o $(program_NAME) $(LDFLAGS)

clean:
	@- $(RM) $(program_NAME)
	@- $(RM) $(program_OBJS)

distclean: clean
