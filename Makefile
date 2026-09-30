all := libnd-shop

LDLIBS-libnd-shop := -lxylem

# nd-shop is nd-core's only in-tree consumer of its public header. A sibling -I
# lets a dev build see it before `make install` has run; in CI the sibling does
# not exist and the header comes from the installed nd-core package.
CFLAGS += -I$(shell cd .. && pwd)/axil-nd-core/include


# macOS ld rejects undefined symbols in shared libs, but WARN needs
# qsyslog: an engine-provided function pointer resolved at dlopen (Linux
# allows this by default). dynamic_lookup is the Darwin equivalent.
LDFLAGS-libnd-shop-Darwin += -undefined dynamic_lookup
-include ./../mk/include.mk
