#pragma once

#include "Common.h"

namespace VKE
{
    namespace Input
    {
        namespace EventListeners
        {
            class IInput
            {
                public:

                    virtual ~IInput() {}

                    virtual void OnKeyDown( const Input::KEY& ) {}
                    virtual void OnKeyUp( const Input::KEY& ) {}
                    virtual void OnMouseMove( const MousePosition& Position ) {}
            };
        } // EventListeners
    } // Input
} // VKE