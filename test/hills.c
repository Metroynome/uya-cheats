#import <libuya/spawnpoint.h>

/* Define cuboid counts for each map */
const int MAP_CUBOID_COUNTS[10] = {
    1,
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

// void usage(void)
// {
//     /* Access map 5's cuboids */
//     int mapId = 5;
//     int numCuboids = MAP_CUBOID_COUNTS[mapId];
//     Cuboid* cuboids = vanillaMapCuboids[mapId];

//     /* Iterate */
//     for (i = 0; i < numCuboids; i++) {
//         Cuboid* c = &cuboids[i];
//         /* Use c->pos, c->rot, etc. */
//     }
// }
