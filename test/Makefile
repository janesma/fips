# See Makefile.local for the actual compilation rules for this directory.

# Prefer to build "test" target by default, (which depends on
# top-level all), so that when running "make" from the test directory
# the test programs will actually by compiled.

all:
	$(MAKE) -C .. all

.DEFAULT:
	$(MAKE) -C .. $@