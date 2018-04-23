#ifndef __VKE_TCSINGLETON_H__
#define __VKE_TCSINGLETON_H__

#include "Core/VKETypes.h"

namespace VKE
{
    namespace Utils
    {
        template<typename _T_>
        class TCSingleton
        {
            public:

                TCSingleton()
                {
                    assert(m_pSingleton == nullptr);
                    m_pSingleton = (_T_*)this;
                }

                virtual ~TCSingleton()
                {
                    m_pSingleton = nullptr;
                }

                static _T_* GetSingletonPtr() { return m_pSingleton; }
                static _T_& GetSingleton() { return *m_pSingleton; }

            private:

                static _T_* m_pSingleton;
        };

        template<typename _T_> _T_* TCSingleton< _T_>::m_pSingleton = nullptr;

    } // Utils
} // VKE

#endif // __VKE_TCSINGLETON_H__