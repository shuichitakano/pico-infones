/*
 * author : Shuichi TAKANO
 * since  : Fri Jul 30 2021 04:42:27
 */
#ifndef _510036F3_0134_6411_4376_A918ACA8AC4C
#define _510036F3_0134_6411_4376_A918ACA8AC4C

#include <stdint.h>

namespace io
{
    struct GamePadState
    {
        uint8_t axis[3]{0x80, 0x80, 0x80};
        uint8_t buttons{};
    };

    const GamePadState &getCurrentGamePadState(int i);
}

#endif /* _510036F3_0134_6411_4376_A918ACA8AC4C */
