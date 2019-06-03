#pragma once

#include "Core/VKECommon.h"

namespace VKE
{
    namespace Input
    {
        struct Keys
        {
            enum KEY
            {
                UNKNOWN,
                CAPITAL_Q,
                CAPITAL_W,
                CAPITAL_E,
                CAPITAL_R,
                CAPITAL_T,
                CAPITAL_Y,
                CAPITAL_U,
                CAPITAL_I,
                CAPITAL_O,
                CAPITAL_P,
                CAPITAL_A,
                CAPITAL_S,
                CAPITAL_D,
                CAPITAL_F,
                CAPITAL_G,
                CAPITAL_H,
                CAPITAL_J,
                CAPITAL_K,
                CAPITAL_L,
                CAPITAL_Z,
                CAPITAL_X,
                CAPITAL_C,
                CAPITAL_V,
                CAPITAL_B,
                CAPITAL_N,
                CAPITAL_M,
                Q,
                W,
                E,
                R,
                T,
                Y,
                U,
                I,
                O,
                P,
                A,
                S,
                D,
                F,
                G,
                H,
                J,
                K,
                L,
                Z,
                X,
                C,
                V,
                B,
                N,
                M,
                _MAX_COUNT
            };
        };
        using KEY = Keys::KEY;

        using MousePosition = ExtentU16;

    } // Input
} // VKE