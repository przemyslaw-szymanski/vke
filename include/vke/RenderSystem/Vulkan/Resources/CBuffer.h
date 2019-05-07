#pragma once
#if VKE_VULKAN_RENDERER
#include "Core/CObject.h"
#include "RenderSystem/CDDI.h"
#include "RenderSystem/Resources/CResource.h"

namespace VKE
{
    namespace RenderSystem
    {

        class VKE_API CBuffer : public VKE::Resources::CResource
        {
            friend class CBufferManager;
            friend class CStagingBufferManager;
            
            VKE_ADD_OBJECT_MEMBERS;

            public:

                CBuffer( CBufferManager* pMgr );
                ~CBuffer();

                Result  Init( const SBufferDesc& Desc );

                static hash_t CalcHash( const SBufferDesc& Desc );

                const DDIBuffer&    GetDDIObject() const { return m_BindInfo.hDDIBuffer; }
                uint32_t            GetSize() const { return m_chunkSize; }

                const BUFFER_STATE& GetState() const { return m_currentState; }

                const SBufferDesc&  GetDesc() const { return m_Desc; }

            protected:

                void            _SetState(const BUFFER_STATE& state);
                
                void _Destroy();

            protected:

                SBufferDesc         m_Desc;
                CBufferManager*     m_pMgr;
                SBindMemoryInfo     m_BindInfo;
                uint32_t            m_chunkSize = 0; // if doubleBuffering is used tis is size of one buffer
                handle_t            m_hMemory = NULL_HANDLE;
                BUFFER_STATE        m_oldState = BufferStates::UNKNOWN;
                BUFFER_STATE        m_currentState = BufferStates::UNKNOWN;
        };
        using BufferPtr = Utils::TCWeakPtr< CBuffer >;
        using BufferRefPtr = Utils::TCObjectSmartPtr< CBuffer >;


        /*class VKE_API CVertexBuffer
        {
            public:

                CVertexBuffer( CBufferManager* pMgr );
                ~CVertexBuffer();

                Result Init( const SVertexBufferDesc& Desc );
                void Destroy();

                static hash_t CalcHash( const SVertexBufferDesc& Desc );

            protected:

                void _Destroy();

            protected:

                CBuffer     m_Buffer;
        };
        using VertexBufferPtr = Utils::TCWeakPtr< CVertexBuffer >;
        using VertexBufferRefPtr = Utils::TCObjectSmartPtr< CVertexBuffer >;*/
        using VertexBufferPtr = BufferPtr;
        using VertexBufferRefPtr = BufferRefPtr;

        class VKE_API CIndexBuffer
        {
            public:

                CIndexBuffer( CBufferManager* pMgr );
                ~CIndexBuffer();

                Result Init( const SIndexBufferDesc& Desc );
                void Destroy();

                const DDIBuffer& GetDDIObject() const { return m_Buffer.GetDDIObject(); }

                static hash_t CalcHash( const SIndexBufferDesc& Desc );

                INDEX_TYPE GetIndexType() const { return m_indexType; }

            protected:

                void _Destroy();

            protected:

                CBuffer     m_Buffer;
                INDEX_TYPE  m_indexType;
        };
        using IndexBufferPtr = Utils::TCWeakPtr< CIndexBuffer >;
        using IndexBufferRefPtr = Utils::TCObjectSmartPtr< CIndexBuffer >;

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER