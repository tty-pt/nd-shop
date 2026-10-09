/* main.c — shop, ported to libxylem.
 *
 * Original: tty-pt/nd-basics @ shop, 174 lines. Three commands (shop, buy,
 * sell) and one on_icon.
 *
 * on_icon is a DECORATOR in the old engine -- its signature took the running
 * icon and returned it, amended only for a shop entity:
 *
 *	struct icon on_icon(struct icon i, unsigned ref, unsigned type)
 *
 * and the old sic_call() handed it whatever the previous module had built. XY
 * cannot reproduce that: a co-implemented on_icon runs, but the LAST return
 * wins and xy.last() cannot read the previous one mid-dispatch (measured:
 * rc=-1 ran=0 -- MODS.md §7).
 *
 * So this module does not co-implement on_icon. It follows the ~/site/mods
 * dependency style -- one owner per hook, consumers call it (163 hooks there,
 * none co-implemented) -- and registers an amend function with nd-core, which
 * owns the icon and threads it through its decorators in registration order.
 * See <ttypt/nd-core.h>.
 *
 * Ported unchanged otherwise. The one API-shape change is the cursor: the old
 * nd_cur_t was a pointer passed by address, the new one is an unsigned handle
 * (include/uapi/io.h:107-114), so `nd_fin(&c)` became `nd_fin(c)`.
 */
#include <ttypt/xy-mod.h>

#include <nd/xy.h>

#include <nd/core.h>

unsigned act_shop;

static inline unsigned
vendor_find(unsigned where_ref)
{
	unsigned c = nd_iter(HD_CONTENTS, &where_ref);
	unsigned tmp_ref;

	while (nd_next(&where_ref, &tmp_ref, c)) {
		OBJ tmp;
		nd_get(HD_OBJ, &tmp, &tmp_ref);
		if (tmp.type != TYPE_ENTITY)
			continue;
		if (ent_get(tmp_ref).flags & EF_SHOP) {
			nd_fin(c);
			return tmp_ref;
		}
	}

	return NOTHING;
}

void
do_shop(int fd, int argc __attribute__((unused)), char *argv[] __attribute__((unused)))
{
	OBJ player, npc;
	unsigned player_ref = fd_player(fd);
	unsigned npc_ref, tmp_ref;

	nd_get(HD_OBJ, &player, &player_ref);
	npc_ref = vendor_find(player.location);

	if (npc_ref == NOTHING) {
		nd_printf(player_ref,
			"No one here wants to buy or sell anything.\n");
		return;
	}

	nd_get(HD_OBJ, &npc, &npc_ref);
	nd_printf(player_ref, "%s shows you what's for sale.\n", npc.name);
	look_at(player_ref, npc_ref);

	unsigned c = nd_iter(HD_OBJ, &npc_ref);
	while (nd_next(&npc_ref, &tmp_ref, c)) {
		OBJ tmp;
		nd_get(HD_OBJ, &tmp, &tmp_ref);
		if (tmp.flags & OF_INF)
			nd_printf(player_ref, "%-13s %5dP (Inf)",
					tmp.name, tmp.value);
		else
			nd_printf(player_ref, "%-13s %5dP",
					tmp.name, tmp.value);
	}
	nd_fin(c);
}

/* buy <name> -- takes the item out of the VENDOR, which is why the match is
 * ematch_at(player_ref, npc_ref, ...) and not ematch_mine: ematch_mine looks
 * in the player's own inventory, which is do_sell's job below. */
void
do_buy(int fd, int argc __attribute__((unused)), char *argv[])
{
	unsigned player_ref = fd_player(fd);
	OBJ player;
	nd_get(HD_OBJ, &player, &player_ref);
	char *name = argv[1];
	unsigned npc_ref = vendor_find(player.location);

	if (npc_ref == NOTHING) {
		nd_printf(player_ref,
			"No one here wants to sell you anything.\n");
		return;
	}

	OBJ npc;
	nd_get(HD_OBJ, &npc, &npc_ref);
	unsigned item_ref = ematch_at(player_ref, npc_ref, name);

	if (item_ref == NOTHING) {
		nd_printf(player_ref, "%s does not sell %s.\n",
				npc.name, name);
		return;
	}

	OBJ item;
	nd_get(HD_OBJ, &item, &item_ref);
	int cost = item.value;
	int ihave = player.value;

	if (ihave < cost) {
		nd_printf(player_ref, "You don't have enough pennies.\n");
		return;
	}

	player.value -= cost;
	npc.value += cost;
	nd_put(HD_OBJ, &npc_ref, &npc);
	nd_put(HD_OBJ, &player_ref, &player);

	/* An infinite-stock template is copied rather than moved, so the shop
	 * keeps its shelf. */
	if (item.flags & OF_INF) {
		OBJ nu;
		unsigned nu_ref = object_copy(&nu, item_ref);
		nu.flags &= ~OF_INF;
		nd_put(HD_OBJ, &nu_ref, &nu);
		object_move(nu_ref, player_ref);
	} else {
		object_move(item_ref, player_ref);
	}

	nd_printf(player_ref, "You bought %s for %dP.\n", item.name, cost);
}

/* sell <name> -- the mirror image: matches in the PLAYER's inventory, and the
 * vendor has to be able to afford it. */
void
do_sell(int fd, int argc __attribute__((unused)), char *argv[])
{
	unsigned player_ref = fd_player(fd);
	OBJ player;
	nd_get(HD_OBJ, &player, &player_ref);
	unsigned npc_ref = vendor_find(player.location);

	if (npc_ref == NOTHING) {
		nd_printf(player_ref,
			"No one here wants to buy you anything.\n");
		return;
	}

	unsigned item_ref = ematch_mine(player_ref, argv[1]);

	if (item_ref == NOTHING) {
		nd_printf(player_ref, "You don't have that item.\n");
		return;
	}

	OBJ npc, item;

	nd_get(HD_OBJ, &npc, &npc_ref);
	nd_get(HD_OBJ, &item, &item_ref);
	int cost = item.value;
	int npchas = npc.value;

	if (cost > npchas) {
		nd_printf(player_ref, "%s can't afford to buy %s from you.\n",
				npc.name, item.name);
		return;
	}

	object_move(item_ref, npc_ref);

	player.value += cost;
	npc.value -= cost;
	nd_put(HD_OBJ, &player_ref, &player);
	nd_put(HD_OBJ, &npc_ref, &npc);

	nd_printf(player_ref, "You sold %s for %dP.\n", item.name, cost);
}

/* The decorator. Translates the old

 *	struct icon on_icon(struct icon i, unsigned ref, unsigned type)

 * and is otherwise its body verbatim. `i` is the icon nd-core built, handed
 * back for amendment -- which is the one thing the co-implementor route could
 * not do, and why this module is a decorator rather than a second on_icon. */
static struct icon
shop_icon_decorate(struct icon i, unsigned ref, unsigned type,
	unsigned player_ref __attribute__((unused)))
{
	OBJ obj;

	if (!(type == TYPE_ENTITY && (ent_get(ref).flags & EF_SHOP)))
		return i;

	nd_get(HD_OBJ, &obj, &ref);

	i.actions |= act_shop;
	i.pi.fg = GREEN;
	i.ch = '$';

	/* Marker for the engine's test suite. The suite's existing icon
	 * assertion greps nd-core's OWN WARN, which is printed before the
	 * decorators run and so cannot see anything they do -- it proves
	 * nd-core ran, not what the client received. That gap let an earlier
	 * revision of this decorator pass the suite while quietly replacing
	 * every room's glyph with '?'. This line is on the amend path only, so
	 * seeing it means a real shop entity was decorated. */
	WARN("nd-shop: decorated ref=%u ch='%c' actions=0x%x fg=%d\n",
		ref, i.ch, (unsigned) i.actions, i.pi.fg);

	return i;
}

XY_MODULE_API void
xy_install(void)
{
	/* The callee, made resident before we ask it for anything. xy_load is
	 * refcounted and idempotent (libxylem mod_load_try_reuse_existing), so
	 * this is safe whether or not nd-core is also named in mods.load. It is
	 * the same filename mods.load uses, so both spellings collapse to one
	 * entry (module_load_path() realpath()s). */
	if (xy_load("libnd-core") != XY_OK) {
		WARN("nd-shop: nd-core not resident; icons will not be "
		     "decorated\n");
		return;
	}

	act_shop = action_register("shop", "💰");

	nd_register("shop", do_shop, 0);
	nd_register("buy", do_buy, 0);
	nd_register("sell", do_sell, 0);

	/* Decorate, do not co-implement. */
	core_icon_decorate(shop_icon_decorate);

	WARN("nd-shop: xy_install, commands shop/buy/sell, act_shop=%u, "
	     "icon decorator registered\n", act_shop);
}
