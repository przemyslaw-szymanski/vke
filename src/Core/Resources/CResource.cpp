#include "Core/Resources/CResource.h"

namespace VKE
{
    namespace Resources
    {
        CResource::CResource() :
            CResource(nullptr)
        {}

        CResource::CResource(CManager* pMgr) :
            CResource(nullptr, NULL_HANDLE)
        {}

        CResource::CResource(CManager* pMgr, handle_t hResource) :
            m_pMgr(pMgr)
            , m_handle(hResource)
            , m_isValid(false)
        {}

        CResource::~CResource()
        {

        }

        void CResource::Invalidate()
        {
            m_isValid = false;
        }

        void Update()
        {

        }

    } // Resources
} // VKE