# This is the primary top-level makefile.
# Now ones that you probably want to tweak

INSTALL_PREFIX=bs-install
HDRS:=src/include
OPTIM:=-g
CFLAGS:=-Wall -pthread
CXXFLAGS:=-Wall -pthread
LDFLAGS:=-pthread

# Tell bs where to find inferior Makefiles.

SUBDIRS=src

# Finally, invoke bs.

include bs/Make.main
