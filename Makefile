OUTDIR = out
SRCDIR = src

INCLUDE_DIRECTORIES = include vendor/linenoise

SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c,$(OUTDIR)/%.o,$(SRCS))
DEPS = $(OBJS:.o=.d)

CC = clang
PREFIX = /usr/local
CFLAGS = -std=c99 -g -O2 -MMD -Wall -Wextra
CFLAGS += $(addprefix -I, $(INCLUDE_DIRECTORIES))
LDFLAGS = -lm

TARGET = $(OUTDIR)/jam

OBJS += vendor/linenoise/linenoise.o

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(@D)
	@printf "\e[32mLINK\e[90m %s\e[0m\n" $@
	@$(CC) -o $@ $^ $(LDFLAGS)

$(OUTDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(@D)
	@printf "\e[36mCC\e[90m %s\e[0m\n" $@
	@$(CC) $(CFLAGS) -c -o $@ $<

install: $(TARGET)
	install -d $(PREFIX)/bin
	install -m 755 $(TARGET) $(PREFIX)/bin/$(TARGET)

uninstall:
	rm -f $(PREFIX)/bin/$(TARGET)

clean:
	rm -rf $(OUTDIR) $(TARGET)

-include $(DEPS)

.PHONY: all clean install uninstall