all := libnd-shop

LDLIBS-libnd-shop := -lxylem

# nd-shop is nd-core's only in-tree consumer of its public header. A sibling -I
# lets a dev build see it before `make install` has run; in CI the sibling does
# not exist and the header comes from the installed nd-core package.
CFLAGS += -I$(shell cd .. && pwd)/axil-nd-core/include

-include ./../mk/include.mk
