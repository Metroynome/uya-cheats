#include <tamtypes.h>
#include <libuya/stdio.h>
#include <libuya/string.h>
#include <libuya/utils.h>
#include <libuya/game.h>
#include <libuya/interop.h>
#include <libuya/graphics.h>
#include <libuya/ui.h>
#include <libuya/hud.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

typedef struct {
	u32 callsite;
	const u32 (*table)[2];
    int count;
} ColorHookMap;

static ColorHookMap gColorHooks[4];
static int gColorHookCount = 0;

const u32 widget3d_2d_colors[][2] = {
	{0x331465b7, 0x60070b54}, // background
	{0x802299de, 0x80070b54}, // line details
};

const u32 widgetRectangle_colors[][2] = {
	{0x00000000, 0x00000000}, // screen overlay 1
	{0x60000000, 0x80000000},
	{0x80000000, 0x80000000},
	{0x80808080, 0x80808080}, // screen overlay 2
	{0x331465b7, 0x33606060}, // main background
	{0xafafafaf, 0xaf070b54},
};

const u32 widgetText_colors[][2] = {
	{0x8066ccff, 0x80ffffff},
};

const u32 widgetTextarea_colors[][2] = {
    {0, 0},
};



static inline u32 get_ra(void)
{
	u32 ra;
	__asm__ volatile("move %0, $ra" : "=r"(ra));
	return ra;
}

bool uiColor_dispatch(HANDLE_ID handle, u32 color)
{
	u32 ra = get_ra();

    int i, j;
	for (i = 0; i < gColorHookCount; ++i) {
		if (ra == gColorHooks[i].callsite + 8) {
			const u32 (*table)[2] = gColorHooks[i].table;
			int count = gColorHooks[i].count;

			for (j = 0; j < count; ++j)
				if (color == table[j][0])
					return hudSetColor(handle, table[j][1]);

			break;
		}
	}

	return hudSetColor(handle, color);
}

void uiColor(void)
{
	struct Def {
		u32 *va;
		u32 offset;
		const u32 (*table)[2];
		int count;
	};

	static struct Def defs[4];

	int i;
	u32 addr;

	/* initialize once (C89-safe style, no fancy init blocks inside function logic) */
	defs[0].va = &vaCreateWidget3d_2D;
	defs[0].offset = 0x74;
	defs[0].table = widget3d_2d_colors;
	defs[0].count = ARRAY_SIZE(widget3d_2d_colors);

	defs[1].va = &vaCreateWidgetRectangle;
	defs[1].offset = 0x6c;
	defs[1].table = widgetRectangle_colors;
	defs[1].count = ARRAY_SIZE(widgetRectangle_colors);

	defs[2].va = &vaCreateWidgetText;
	defs[2].offset = 0x64;
	defs[2].table = widgetText_colors;
	defs[2].count = ARRAY_SIZE(widgetText_colors);

	defs[3].va = &vaCreateWidgetTextArea;
	defs[3].offset = 0x74;
	defs[3].table = widgetTextarea_colors;
	defs[3].count = ARRAY_SIZE(widgetTextarea_colors);

	gColorHookCount = 0;

	for (i = 0; i < 4; ++i) {
		addr = GetAddressImmediate(defs[i].va);
		if (!addr)
			continue;

		addr += defs[i].offset;

		HOOK_JAL(addr, &uiColor_dispatch);

		{
			ColorHookMap *hook = &gColorHooks[gColorHookCount++];
			hook->callsite = addr;
			hook->table = defs[i].table;
			hook->count = defs[i].count;
		}
	}
}

void runApril(void)
{
    printf("rawr\n");
    uiColor();
}