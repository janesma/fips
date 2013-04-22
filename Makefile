# Default target
all:

# List all subdirectores here. Each contains its own Makefile.local
subdirs :=

# We make all targets depend on the Makefiles themselves.
global_deps = Makefile Makefile.config Makefile.local \
	$(subdirs:%=%/Makefile) $(subdirs:%=%/Makefile.local)

# Get settings from the output of configure by running it to generate
# Makefile.config if it doesn't exist yet.

# If Makefile.config doesn't exist, then srcdir won't be
# set. Conditionally set it (assuming a plain srcdir build) so that
# the rule to generate Makefile.config can actually work.
srcdir ?= .

include Makefile.config
Makefile.config: $(srcdir)/configure
ifeq ($(configure_options),)
	@echo ""
	@echo "Note: Calling ./configure with no command-line arguments. This is often fine,"
	@echo "      but if you want to specify any arguments (such as an alternate prefix"
	@echo "      into which to install), call ./configure explicitly and then make again."
	@echo "      See \"./configure --help\" for more details."
	@echo ""
endif
	$(srcdir)/configure $(configure_options)

# Finally, include all of the Makefile.local fragments where all the
# real work is done.

include $(subdirs:%=%/Makefile.local) Makefile.local
