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

bool uiColor_widget3d_2d_setColor(HANDLE_ID handle, u32 color)
{
	int i;
	const int widgetsColorSize = sizeof(widget3d_2d_colors) / sizeof(widget3d_2d_colors[0]);

	for (i = 0; i < widgetsColorSize; ++i) {
		if (color == widget3d_2d_colors[i][0]) {
			return hudSetColor(handle, widget3d_2d_colors[i][1]);
		}
	}

	return hudSetColor(handle, color);
}

bool uiColor_widgetRectangle_setColor(HANDLE_ID handle, u32 color)
{
	int i;
	const int widgetsColorSize = sizeof(widgetRectangle_colors) / sizeof(widgetRectangle_colors[0]);

	for (i = 0; i < widgetsColorSize; ++i) {
		if (color == widgetRectangle_colors[i][0]) {
			return hudSetColor(handle, widgetRectangle_colors[i][1]);
		}
	}

	return hudSetColor(handle, color);
}

bool uiColor_widgetText_setColor(HANDLE_ID handle, u32 color)
{
	int i;
	const int widgetsColorSize = sizeof(widgetText_colors) / sizeof(widgetText_colors[0]);

	for (i = 0; i < widgetsColorSize; ++i) {
		if (color == widgetText_colors[i][0]) {
			return hudSetColor(handle, widgetText_colors[i][1]);
		}
	}

	return hudSetColor(handle, color);
}

bool uiColor_widgetTextarea_setColor(HANDLE_ID handle, u32 color)
{
	int i;
	const int widgetsColorSize = sizeof(widgetTextarea_colors) / sizeof(widgetTextarea_colors[0]);

	for (i = 0; i < widgetsColorSize; ++i) {
		if (color == widgetTextarea_colors[i][0]) {
			return hudSetColor(handle, widgetTextarea_colors[i][1]);
		}
	}

	return hudSetColor(handle, color);
}

inline void hookWidget(u32 base, u32 offset, void *func)
{
	if (base)
		HOOK_JAL(base + offset, func);
}

void uiColor(void)
{
	hookWidget(GetAddressImmediate(&vaCreateWidget3d_2D), 0x74, &uiColor_widget3d_2d_setColor);
	hookWidget(GetAddressImmediate(&vaCreateWidgetRectangle), 0x6c, &uiColor_widgetRectangle_setColor);
	hookWidget(GetAddressImmediate(&vaCreateWidgetText), 0x64, &uiColor_widgetText_setColor);
	hookWidget(GetAddressImmediate(&vaCreateWidgetTextArea), 0x74, &uiColor_widgetTextarea_setColor);
}

void runApril(void)
{
    uiColor();
}