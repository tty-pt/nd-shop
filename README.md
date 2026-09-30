# axil-nd-shop

`nd-shop` for [axil-nd](../axil-nd), ported from SIC to libxylem.

Adds the `shop` / `buy` / `sell` commands, and decorates the icons of shop
objects. It is the only module of the ported set that depends on another:
`nd-core` owns `on_icon`, and shop registers a decorator with it rather than
co-implementing the hook.

## Install

```sh
make install
```

Installs one file:

```
lib/libnd-shop.so
```

There is deliberately no `lib/nd-shop.so` symlink. `xy_load()` appends `.so`
itself and does not retry with a `lib` prefix, so the engine's `mods.load` names
this module `libnd-shop` and `dlopen`s `libnd-shop.so`. A soname symlink would
also have been silently dropped from the OpenBSD package: `tty-pt/ci` builds the
packing list from `find usr -type f`, which never lists a symlink, so the package
would have shipped the library under one name and asked the loader for another.

Also packaged for deb, apk, rpm, brew and openbsd from a `v*` tag.

It installs no header: `do_shop`/`do_buy`/`do_sell` are command entry points the
engine reaches by name, and the one thing it offers other modules —
`shop_icon_decorate` — is *registered* with `nd-core`, not called.

**Load order matters.** shop must come after `nd-core` in the engine's
`mods.load`. `nd-core` walks its decorator table in registration order, so a shop
loaded first would have nowhere to register. Its own `xy_install` also calls
`xy_load("libnd-core")` to make the callee resident, which is a no-op refcount
bump when the engine already loaded it, and required when it did not.

## Build from source

```sh
make
```

Needs [libxylem](https://github.com/tty-pt/libxylem) (headers already in
`$(PREFIX)/include`), the engine's game API `<nd/xy.h>` from either an `axil-nd`
checkout beside this repo or an installed `axil-nd`, **and an installed
`nd-core`**:

```sh
git clone https://github.com/tty-pt/nd-shop && cd nd-shop
git clone https://github.com/tty-pt/axil-nd ../axil-nd
make
```

This is the one module with a build-time dependency on another library, because
it is the only one that includes `<nd/core.h>`. There is no link-time
dependency: `core_icon_decorate()` is reached through the `XY_DECL` in that
header, which dispatches via `xy_call()` at run time, so `NEEDED` is
`libxylem.so` and `libc.so.6` and nothing else.

That header dependency used to be invisible. The include was
`"../axil-nd-core/core.h"` — a relative path into a sibling checkout — so it
worked only for anyone who happened to have that sibling at that exact relative
path, and broke silently for everyone else. `<nd/core.h>` resolves from
`-I$(PREFIX)/include`, or from the checkout beside this repo, which is what makes
the dependency explicit.

## Using it

`xy_install` registers the three commands and then installs the icon decorator:

```c
static struct icon
shop_icon_decorate(struct icon i, unsigned ref, unsigned type,
	unsigned player_ref)
{
	if (type == TYPE_OBJ && ...) {
		i.ch = '$';
		i.pi.fg = YELLOW;
		i.actions |= ACT_LOOK;
		WARN("nd-shop: decorated ref=%u ch='%c' actions=0x%x\n", ...);
	}
	return i;
}
```

A decorator receives the icon the owner built and returns the amended one. It
must not re-derive the icon it is decorating, or it silently drops whatever the
owner set. See `<nd/core.h>`.

## Testing

There is no `test.sh` here. Behaviour is asserted by the engine's own suite,
which builds every module in its `mods.load`, boots, and greps stderr:

```sh
cd ../axil-nd
make && ./test.sh
```

The suite asserts both `nd-shop: xy_install, commands shop/buy/sell` and
`nd-core: core_icon_decorate #1` — the latter is the cross-module half, proving a
decorator registered in *this* module reached the installed `nd-core`.

One part of the chain is still unasserted: the amend-path marker
(`nd-shop: decorated ref=... ch='$' ...`). Nothing in the engine or the art ever
sets `EF_SHOP` — `src/view.c` only reads it — so no vendor exists on a stock
boot and the line cannot fire. Asserting it needs a fixture world containing a
vendor.

## Notes from the port

* **The include changed spelling.** It was `"papi/nd-xy.h"`, a file that no
  longer exists in any checkout: the engine moved its module-facing tree from
  `papi/` to `nd/`, and `papi/` now holds only `nd.h`. The module could not
  compile until this was fixed.
* `xy_load()` of the callee was added. The original relied on the engine having
  loaded `nd-core` already, which was true of the in-tree layout and is not
  guaranteed now that both are independently installable packages.
* This TU does **not** `XY_IMPL` `on_icon`. That is the whole point of the
  decorator route: `nd-core` is the sole owner of the hook and XY cannot observe
  a co-implemented chain, so two modules both claiming `on_icon` gives a build
  error at best and a silently wrong dispatch at worst.

## License

BSD 2-Clause, carried over from `tty-pt/nd-shop`. See `LICENSE`.
