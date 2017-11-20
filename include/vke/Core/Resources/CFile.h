#pragma once
#include "Common.h"
#include "Core/CObject.h"
#include "Core/Utils/TCSmartPtr.h"
namespace VKE
{
    namespace Resources
    {
        class CFile : public Core::CObject
        {

        };
    } // Resources
    using FilePtr = Utils::TCWeakPtr< Resources::CFile >;
    using FileSmartPtr = Utils::TCObjectSmartPtr< Resources::CFile >;
} // VKE