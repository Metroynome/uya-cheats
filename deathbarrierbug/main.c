#include <tamtypes.h>

#include <libuya/string.h>
#include <libuya/player.h>
#include <libuya/utils.h>
#include <libuya/game.h>
#include <libuya/pad.h>
#include <libuya/stdio.h>
#include <libuya/uya.h>
#include <libuya/interop.h>


void patchDeathBarrierBug()
{
    
}

int main()
{
    if (isInGame())
    {
        patchDeathBarrierBug();
    }
    return 0;
}
