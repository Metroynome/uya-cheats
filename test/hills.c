#include <libuya/stdio.h>
#include <libuya/spawnpoint.h>
#include <libuya/game.h>
#include <libuya/map.h>
#include "koth.h"


extern Cuboid bakisiIsles[];
extern Cuboid hovenGorge[];
extern Cuboid outpostX12[];
extern Cuboid korgonOutpost[];
extern Cuboid metropolis[];
extern Cuboid blackwaterCity[];
extern Cuboid commandCenter[];
extern Cuboid blackwaterDocks[];
extern Cuboid aquatosSewers[];
extern Cuboid marcadia[];

/* Define cuboid counts for each map */
const int MAP_CUBOID_COUNTS[10] = {
    2,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1
};

// array based on MAP_ID - 40, if hill moby (0x3000) doesn't exist.
Cuboid* vanillaMapCuboids[10] = {
    bakisiIsles,
    hovenGorge,
    outpostX12,
    korgonOutpost,
    metropolis,
    blackwaterCity,
    commandCenter,
    blackwaterDocks,
    aquatosSewers,
    marcadia
};

Cuboid bakisiIsles[] = {
    {
        .matrix = {
            {8.724, 0, 0, 0},
            {0, 8.724, 0, 0},
            {0, 0, 1, 0}
        },
        .pos = {373.6029, 414.3022, 203.8417, 0},
        .imatrix = 0,
        .rot = {-2.371, 0, -104.243, 0}
    },
    {
        .matrix = {
            {20, 0, 0, 0},
            {0, 30, 0, 0},
            {0, 0, 1, 0}
        },
        .pos = {519.58356, 398.7586, 201.38, 1},
        .imatrix = 0,
        .rot = {0, 0, 0, 0}
    },
};

Cuboid hovenGorge[] = {
    {
        .matrix = {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 1, 0}
        },
        .pos = {0, 0, 0, 0},
        .imatrix = 0,
        .rot = {0, 0, 0, 0}
    },
};

Cuboid outpostX12[] = {
    {
        .matrix = {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 1, 0}
        },
        .pos = {0, 0, 0, 0},
        .imatrix = 0,
        .rot = {0, 0, 0, 0}
    },
};

Cuboid korgonOutpost[] = {
    {
        .matrix = {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 1, 0}
        },
        .pos = {0, 0, 0, 0},
        .imatrix = 0,
        .rot = {0, 0, 0, 0}
    },
};

Cuboid metropolis[] = {
    {
        .matrix = {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 1, 0}
        },
        .pos = {0, 0, 0, 0},
        .imatrix = 0,
        .rot = {0, 0, 0, 0}
    },
};

Cuboid blackwaterCity[] = {
    {
        .matrix = {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 1, 0}
        },
        .pos = {0, 0, 0, 0},
        .imatrix = 0,
        .rot = {0, 0, 0, 0}
    },
};

Cuboid commandCenter[] = {
    {
        .matrix = {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 1, 0}
        },
        .pos = {0, 0, 0, 0},
        .imatrix = 0,
        .rot = {0, 0, 0, 0}
    },
};

Cuboid blackwaterDocks[] = {
    {
        .matrix = {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 1, 0}
        },
        .pos = {0, 0, 0, 0},
        .imatrix = 0,
        .rot = {0, 0, 0, 0}
    },
};

Cuboid aquatosSewers[] = {
    {
        .matrix = {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 1, 0}
        },
        .pos = {0, 0, 0, 0},
        .imatrix = 0,
        .rot = {0, 0, 0, 0}
    },
};

Cuboid marcadia[] = {
    {
        .matrix = {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 1, 0}
        },
        .pos = {0, 0, 0, 0},
        .imatrix = 0,
        .rot = {0, 0, 0, 0}
    },
};

void getVanillaHills(hillPvar_t *this)
{
    int mapId = GAME_MAP_ID - 40;  // Fixed based on your comment
    int numCuboids = MAP_CUBOID_COUNTS[mapId];
    Cuboid* cuboids = vanillaMapCuboids[mapId];

    printf("\n=================");
    printf("\npvar %08x", this);
    printf("\ncuboidList %08x", cuboids);

    // Initialize all to NULL first
    memset(&this->vanillaHills, -1, sizeof(int) * 32);

    // Fill in the valid ones
    int i;
    for (i = 0; i < numCuboids; i++) {
        this->vanillaHills[i] = &cuboids[i];
        printf("\nhill %i : %08x", i, &this->vanillaHills[i]);
    }
    printf("\n=================");
}
