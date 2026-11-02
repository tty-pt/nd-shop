all := libnd-shop

LDLIBS-libnd-shop := -lxylem

# macOS ld rejects undefined symbols in shared libs, but WARN needs
# stllog: an engine-provided function pointer resolved at dlopen (Linux
# allows this by default). dynamic_lookup is the Darwin equivalent.
LDFLAGS-libnd-shop-Darwin += -undefined dynamic_lookup
FOLDER := nd

-include ./../mk/include.mk

