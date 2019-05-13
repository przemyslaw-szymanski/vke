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
            VKE_ADD_DDI_OBJECT( DDIBuffer );

            public:

                CBuffer( CBufferManager* pMgr );
                ~CBuffer();

                Result  Init( const SBufferDesc& Desc );

                static hash_t CalcHash( const SBufferDesc& Desc );

                uint32_t            GetSize() const { return m_chunkSize; }

                const SBufferDesc&  GetDesc() const { return m_Desc; }

                const SResourceBindingInfo& GetBindingInfo() const { return m_ResourceBindingInfo; }
                //SResourceBindingInfo& GetBindingInfo() { return m_ResourceBindingInfo; }

                uint32_t            GetChunkSize() const { return m_chunkSize; }

                uint32_t            SetNextChunk();
                uint32_t            SetChunk( uint32_t idx );

            protected:

                void            _SetState(const BUFFER_STATE& state);
                
                void _Destroy();

            protected:

                SBufferDesc             m_Desc;
                CBufferManager*         m_pMgr;
                handle_t                m_hMemory;
                SResourceBindingInfo    m_ResourceBindingInfo;
                uint32_t                m_chunkSize = 0; // if doubleBuffering is used tis is size of one buffer
                uint32_t                m_currentChunk = 0;
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