#pragma once
#if VKE_VULKAN_RENDER_SYSTEM
#include "Core/CObject.h"
#include "RenderSystem/CDDI.h"
#include "RenderSystem/Resources/CResource.h"

namespace VKE
{
    namespace RenderSystem
    {

        class VKE_API CBuffer
        {
            friend class CBufferManager;
            friend class CStagingBufferManager;

            VKE_ADD_OBJECT_MEMBERS;
            VKE_ADD_DDI_OBJECT( DDIBuffer );
            VKE_DECL_BASE_OBJECT( BufferHandle );
            VKE_DECL_BASE_RESOURCE();

            struct SRegion
            {
                uint32_t    offset;
                uint32_t    size;
                uint32_t    elemSize; // includes size requested and required alignment
            };

            using RegionArray = Utils::TCDynamicArray< SRegion >;

            public:

                CBuffer( CBufferManager* pMgr );
                ~CBuffer();

                Result  Init( const SBufferDesc& Desc );

                static hash_t CalcHash( const SBufferDesc& Desc );

                uint32_t            GetSize() const { return m_Desc.size; }

                const SBufferDesc&  GetDesc() const { return m_Desc; }

                const SResourceBindingInfo& GetBindingInfo() const { return m_ResourceBindingInfo; }

                /// <summary>
                /// Calculates 0 based absolute offset for element in region.
                /// </summary>
                /// <param name="region"></param>
                /// <param name="elemIdx"></param>
                /// <returns></returns>
                uint32_t            CalcAbsoluteOffset( const uint16_t& region, const uint32_t& elemIdx ) const;
                /// <summary>
                /// Calculates 0 based element offset relative to a given region.
                /// </summary>
                /// <param name="region"></param>
                /// <param name="elemIdx"></param>
                /// <returns></returns>
                uint32_t            CalcRelativeOffset( const uint16_t& region, const uint32_t& elemIdx ) const;

                uint32_t            GetRegionElementSize( const uint16_t& region ) const { return m_vRegions[region].elemSize; }
                uint32_t            GetRegionSize( const uint16_t& region ) const { return m_vRegions[region].size; }

                uint32_t            GetRegionCount() const { return m_vRegions.GetCount(); }

                handle_t            GetMemory() const { return m_hMemory; }

            protected:

                void            _SetState(const BUFFER_STATE& state);

                void _Destroy();

            protected:

                SBufferDesc             m_Desc;
                CBufferManager*         m_pMgr;
                handle_t                m_hMemory;
                SResourceBindingInfo    m_ResourceBindingInfo;
                RegionArray             m_vRegions;
                uint16_t                m_alignment = 1;
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
        using IndexBufferPtr = BufferPtr;
        using IndexBufferRefPtr = BufferRefPtr;

        /*class VKE_API CIndexBuffer
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
        using IndexBufferRefPtr = Utils::TCObjectSmartPtr< CIndexBuffer >;*/

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDER_SYSTEM