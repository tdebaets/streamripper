# Generated automatically from Makefile.in by configure.
# Generated automatically from Makefile.in by configure.
#Makefile for streamripper

INSTALL_PROGRAM = ${INSTALL}
bindir = /usr/local/bin
SHELL = /bin/sh

INSTALL = /usr/bin/install -c

all: consoleapp

consoleapp: 
	@echo "Compiling console frontend -"
	@cd console; make
	@mv console/streamripper .

clean:
	rm -f streamripper *.o lib/*.o console/*.o *.core core
	cd libmad; make clean

install:
	@echo "- Installing streamripper"
	@ $(INSTALL_PROGRAM) streamripper $(bindir)

