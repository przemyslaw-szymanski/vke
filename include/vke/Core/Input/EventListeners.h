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

                    virtual void OnKeyDown( const Input::SKeyboardState&, const Input::KEY& ) {}
                    virtual void OnKeyUp( const Input::SKeyboardState&, const Input::KEY& ) {}
                    virtual void OnMouseMove( const Input::SMouseState& ) {}
                    virtual void OnMouseButtonDown( const Input::SMouseState& ) {}
                    virtual void OnMouseButtonUp( const Input::SMouseState& ) {}
            };
        } // EventListeners
    } // Input
} // VKE