#include "Core/Platform/CPlatform.h"

#if VKE_LINUX

namespace VKE::Platform
{
    namespace Internal
    {
        // TODO(blturkot): Reconsider this. Remove map generation and make it static just like Windows is.
        // 
        // X keycodes are evdev (linux/input-event-codes.h) codes + 8. Mapping by physical key, independent of layout.
        static constexpr uint32_t X_KEYCODE_OFFSET = 8;

        struct SKeyMapEntry
        {
            uint16_t   evdevCode;
            Input::KEY key;
        };

        static constexpr SKeyMapEntry aEvdevKeys[] = {
            { 1, Input::Keys::ESC },
            { 2, Input::Keys::NUM_1 },
            { 3, Input::Keys::NUM_2 },
            { 4, Input::Keys::NUM_3 },
            { 5, Input::Keys::NUM_4 },
            { 6, Input::Keys::NUM_5 },
            { 7, Input::Keys::NUM_6 },
            { 8, Input::Keys::NUM_7 },
            { 9, Input::Keys::NUM_8 },
            { 10, Input::Keys::NUM_9 },
            { 11, Input::Keys::NUM_0 },
            { 12, Input::Keys::SUBTRACT },
            { 13, Input::Keys::ADD },
            { 14, Input::Keys::BACK },
            { 15, Input::Keys::TAB },
            { 16, Input::Keys::Q },
            { 17, Input::Keys::W },
            { 18, Input::Keys::E },
            { 19, Input::Keys::R },
            { 20, Input::Keys::T },
            { 21, Input::Keys::Y },
            { 22, Input::Keys::U },
            { 23, Input::Keys::I },
            { 24, Input::Keys::O },
            { 25, Input::Keys::P },
            { 28, Input::Keys::ENTER },
            { 29, Input::Keys::LEFT_CONTROL },
            { 30, Input::Keys::A },
            { 31, Input::Keys::S },
            { 32, Input::Keys::D },
            { 33, Input::Keys::F },
            { 34, Input::Keys::G },
            { 35, Input::Keys::H },
            { 36, Input::Keys::J },
            { 37, Input::Keys::K },
            { 38, Input::Keys::L },
            { 42, Input::Keys::LEFT_SHIFT },
            { 44, Input::Keys::Z },
            { 45, Input::Keys::X },
            { 46, Input::Keys::C },
            { 47, Input::Keys::V },
            { 48, Input::Keys::B },
            { 49, Input::Keys::N },
            { 50, Input::Keys::M },
            { 54, Input::Keys::RIGHT_SHIFT },
            { 55, Input::Keys::NUMPAD_MULTIPLY },
            { 56, Input::Keys::LEFT_ALT },
            { 57, Input::Keys::SPACE },
            { 58, Input::Keys::CAPSLOCK },
            { 59, Input::Keys::F1 },
            { 60, Input::Keys::F2 },
            { 61, Input::Keys::F3 },
            { 62, Input::Keys::F4 },
            { 63, Input::Keys::F5 },
            { 64, Input::Keys::F6 },
            { 65, Input::Keys::F7 },
            { 66, Input::Keys::F8 },
            { 67, Input::Keys::F9 },
            { 68, Input::Keys::F10 },
            { 69, Input::Keys::NUMLOCK },
            { 70, Input::Keys::SCROLL },
            { 71, Input::Keys::NUMPAD_7 },
            { 72, Input::Keys::NUMPAD_8 },
            { 73, Input::Keys::NUMPAD_9 },
            { 74, Input::Keys::NUMPAD_MINUS },
            { 75, Input::Keys::NUMPAD_4 },
            { 76, Input::Keys::NUMPAD_5 },
            { 77, Input::Keys::NUMPAD_6 },
            { 78, Input::Keys::NUMPAD_PLUS },
            { 79, Input::Keys::NUMPAD_1 },
            { 80, Input::Keys::NUMPAD_2 },
            { 81, Input::Keys::NUMPAD_3 },
            { 82, Input::Keys::NUMPAD_0 },
            { 83, Input::Keys::DECIMAL },
            { 87, Input::Keys::F11 },
            { 88, Input::Keys::F12 },
            { 96, Input::Keys::ENTER },
            { 97, Input::Keys::RIGHT_CONTROL },
            { 98, Input::Keys::NUMPAD_SLASH },
            { 99, Input::Keys::PRINT_SCREEN },
            { 100, Input::Keys::RIGHT_ALT },
            { 102, Input::Keys::HOME },
            { 103, Input::Keys::UP },
            { 104, Input::Keys::PAGE_UP },
            { 105, Input::Keys::LEFT },
            { 106, Input::Keys::RIGHT },
            { 107, Input::Keys::END },
            { 108, Input::Keys::DOWN },
            { 109, Input::Keys::PAGE_DOWN },
            { 110, Input::Keys::INSERT },
            { 111, Input::Keys::DEL },
            { 113, Input::Keys::VOLUME_MUTE },
            { 114, Input::Keys::VOLUME_DOWN },
            { 115, Input::Keys::VOLUME_UP },
            { 119, Input::Keys::PAUSE },
            { 125, Input::Keys::LEFT_WINDOWS },
            { 126, Input::Keys::RIGHT_WINDOW },
            { 142, Input::Keys::SLEEP },
        };

        static constexpr std::array< Input::KEY, KeyMap::MAP_SIZE > BuildKeyMap()
        {
            std::array< Input::KEY, KeyMap::MAP_SIZE > aMap{};
            for( const auto& Entry: aEvdevKeys )
            {
                aMap[ ( Entry.evdevCode + X_KEYCODE_OFFSET ) & 0xFF ] = Entry.key;
            }
            return aMap;
        }

        static constexpr std::array< Input::KEY, KeyMap::MAP_SIZE > aKeyMap = BuildKeyMap();
    } // namespace Internal

    const Input::KEY KeyMap::s_aKeyMap[ KeyMap::MAP_SIZE ] = {
#define VKE_KEY_8( _i )                                                                                                \
    Internal::aKeyMap[ _i + 0 ], Internal::aKeyMap[ _i + 1 ], Internal::aKeyMap[ _i + 2 ],                             \
        Internal::aKeyMap[ _i + 3 ], Internal::aKeyMap[ _i + 4 ], Internal::aKeyMap[ _i + 5 ],                         \
        Internal::aKeyMap[ _i + 6 ], Internal::aKeyMap[ _i + 7 ]
#define VKE_KEY_64( _i )                                                                                               \
    VKE_KEY_8( _i ), VKE_KEY_8( _i + 8 ), VKE_KEY_8( _i + 16 ), VKE_KEY_8( _i + 24 ), VKE_KEY_8( _i + 32 ),            \
        VKE_KEY_8( _i + 40 ), VKE_KEY_8( _i + 48 ), VKE_KEY_8( _i + 56 )
        VKE_KEY_64( 0 ), VKE_KEY_64( 64 ), VKE_KEY_64( 128 ), VKE_KEY_64( 192 )
#undef VKE_KEY_64
#undef VKE_KEY_8
    };
} // namespace VKE::Platform

#endif // VKE_LINUX
