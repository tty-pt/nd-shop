## 1.0.1

- **macOS: link with `-undefined dynamic_lookup`.** macOS `ld` rejects
  undefined symbols in a shared library, but `WARN` needs `qsyslog` — an
  engine-provided function pointer resolved at `dlopen` time (Linux allows
  this by default). `-undefined dynamic_lookup` is the Darwin equivalent, set
  as `LDFLAGS-libnd-shop-Darwin` so no other platform is affected.

## [1.0.0]

- **nd-shop is now an installable library rather than a build artifact of the
  engine.** It builds and installs one file: `lib/libnd-shop.so`, following the
  same layout as `axil-tty` and `axil-auth`, and the same layout `nd-core` was
  converted to first. Previously `make` produced a `shop.so` named by the
  engine's `mods.load` and installed nothing. It installs no header because it
  exports no API: `do_shop`/`do_buy`/`do_sell` are command entry points the
  engine reaches by name, and the one thing it does export for others —
  `shop_icon_decorate` — is registered, not called. There is no
  `lib/nd-shop.so` symlink: `mods.load` names this module `libnd-shop`, the
  installed filename, and `module_load_path()` only appends `.so`, so the load
  name must equal the installed name. A symlink would in any case have been
  dropped from the OpenBSD package, whose packing list is built from
  `find usr -type f`.

- **The link line is libxylem alone.** `LDLIBS := -lxylem`; the engine is not
  linked. `core_icon_decorate()` is reached through the `XY_DECL` in
  `<ttypt/nd-core.h>`, which dispatches via `xy_call()` at run time, so
  including that header creates no link-time dependency on `nd-core`. `NEEDED`
  is `libxylem.so` and `libc.so.6`.

- **The game's service API is included as `<nd/xy.h>`**, from the engine's
  `$(PREFIX)/include/nd/` — the same include root as `<ttypt/xy.h>`, so this
  library needs no private `-I` for the game headers at all. It previously
  included `"papi/nd-xy.h"`, which no longer exists in any checkout: the engine
  moved the module-facing tree from `papi/` to `nd/` and `papi/` now holds only
  `nd.h`. That include could not compile, so this release also fixes the build.

- **nd-shop has a build-time dependency on an installed `nd-core`.** It is the
  only module of the ported set that includes `<ttypt/nd-core.h>`, so building
  it needs that header on the include path — install `nd-core`, or keep a
  checkout beside this repo. Previously the include was
  `"../axil-nd-core/core.h"`, a relative path into a sibling checkout, so the
  dependency was invisible in the build and broke for anyone who did not happen
  to have the sibling at that exact relative path.

- **Dropped the `nd-mod.mk` dependency.** This module resolves the game's
  headers itself, the way every other house library does, and `nd-mod.mk` is
  the SIC-era engine module build contract. It has now been deleted: this was
  the last kind of module that still included it.

- **`mods.load` names the installed filename.** A line with no in-tree module
  is passed to `xy_load()` verbatim so the dynamic linker resolves it. As
  before, the name carries no `.so` suffix — `xy_load()` appends it itself.
  Load order is unchanged and still significant: shop registers an icon
  decorator with `nd-core`, and `nd-core` walks its decorator table in
  registration order, so a shop loaded first would have nowhere to register.
