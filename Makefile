all := libnd-shop

LDLIBS-libnd-shop := -lxylem

# macOS ld rejects undefined symbols in shared libs, but WARN needs
# qsyslog: an engine-provided function pointer resolved at dlopen (Linux
# allows this by default). dynamic_lookup is the Darwin equivalent.
LDFLAGS-libnd-shop-Darwin += -undefined dynamic_lookup
FOLDER := nd
add-prefix := ../axil-nd ../nd-region ../nd-session ../nd-world ../nd-view ../nd-obj ../nd-speech ../nd-mcp

-include ./../mk/include.mk

