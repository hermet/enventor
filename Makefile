SRCDIR=src
HEADERDIR=include
EDJDIR=data/edc
OBJECTS=$(SRCDIR)/main.o \
        $(SRCDIR)/menu.o \
        $(SRCDIR)/edj_viewer.o \
        $(SRCDIR)/edc_editor.o \
        $(SRCDIR)/statusbar.o \
        $(SRCDIR)/syntax_color.o \
        $(SRCDIR)/config_data.o \
        $(SRCDIR)/edc_parser.o \
        $(SRCDIR)/panes.o \
        $(SRCDIR)/dummy_obj.o
EDJS=$(EDJDIR)/enventor.edj
BINARY=enventor
DIRNAME=enventor

PREFIX=/usr/local
BINDIR=$(PREFIX)/bin
DATADIR=$(PREFIX)/share/enventor
PROTODIR=/tmp

CC = gcc

CFLAGS = `pkg-config --cflags elementary evas eina eio`
CFLAGS +=  -g -W -Wextra -Wall
LDFLAGS = `pkg-config --libs elementary evas eina eio`

EDJE_CC = edje_cc
EDJE_FLAGS = -id data/edc/images

all: $(OBJECTS) $(BINARY) $(EDJS)

%.o : %.c
	@echo "  Compilation of $(@D)/$(<F)"
	@$(CC) -c $(CFLAGS) $< -o $@  -I $(HEADERDIR)

$(BINARY): $(OBJECTS)
	@echo "  Linking  $(@F)"
	@$(CC) -o $(BINARY) $(OBJECTS) $(LDFLAGS)

$(EDJDIR)/enventor.edj: $(EDJDIR)/enventor.edc
	@echo "  Compilation of $(@D)/$(<F)"
	@$(EDJE_CC) $(EDJE_FLAGS) $(EDJDIR)/enventor.edc $(EDJDIR)/enventor.edj

install: $(BINARY)
	@echo "installation of executables"
	@mkdir -p $(BINDIR)
	@install -m 0755 $(BINARY) $(BINDIR)
	@echo "installation of data"
	@mkdir -p $(DATADIR)/edj
	@install $(EDJDIR)/enventor.edj  $(DATADIR)/edj
	@mkdir -p $(DATADIR)/images
	@install data/images/* $(DATADIR)/images
	@mkdir -p $(DATADIR)/docs
	@install README $(DATADIR)/docs

uninstall:
	rm -rf $(DATADIR)
	rm -rf $(BINDIR)/$(BINARY)

clean:
	@rm -f $(EDJDIR)/*.edj *~ $(BINARY)
	@rm -f $(PROTODIR)/.proto.edc $(PROTODIR)/.proto.edj
