#include "RenderSystem/CDDI.h"

#if VKE_RENDER_SYSTEM_D3D12

#include "Core/Managers/CFileManager.h"
#include "Core/Platform/CWindow.h"

#include "RenderSystem/CContextBase.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/CGraphicsContext.h"
#include "RenderSystem/CRenderPass.h"
#include "RenderSystem/Resources/CBuffer.h"
#include "RenderSystem/Resources/CTexture.h"

#include "RenderSystem/D3D12/dxgiFormats.h"

namespace VKE::RenderSystem
{

    // -----------------------------------------------------------------------------------------------------------------
    // Macros to help D3D12 DDI development.
#define TRACK_CALL_ONCE( msg )                                                                                         \
    static bool s_called = false;                                                                                      \
    if( s_called )                                                                                                     \
    {                                                                                                                  \
        VKE_LOG_ERR( "D3D12 Render System: " + std::string( msg ) + " can only be called once!" );                     \
    }                                                                                                                  \
    s_called = true;

#define UNIMPLEMENTED_D3D12_METHOD() VKE_ASSERT2( false, "D3D12 Render System: Unimplemented method" )

    // -----------------------------------------------------------------------------------------------------------------
    // Initialization of static members.
    NativeAPI::D3D12Factory* NativeAPI::SImplementation::spFactory = NativeAPI::Null;

    bool NativeAPI::SImplementation::sDebugLayerEnabled                 = false;
    bool NativeAPI::SImplementation::SDeviceFeatures::sTearingSupported = false;

    CDDI::AdapterArray CDDI::svAdapters;

    typedef Utils::TCDynamicArray< D3D12_RESOURCE_BARRIER > DDIBarrierArray;

    // -----------------------------------------------------------------------------------------------------------------
    // Implementation functions.
    // These functions are specific to D3D12 implementation that requires long lasting members, which are members of
    // SImplementation class.
    namespace NativeAPI
    {
        SImplementation::SDescriptorHeapInfo* SImplementation::CreateDescriptorHeap( const NativeAPI::Device&   pDevice,
                                                                                     D3D12_DESCRIPTOR_HEAP_TYPE Type,
                                                                                     D3D12_DESCRIPTOR_HEAP_FLAGS Flags )
        {
            D3D12_DESCRIPTOR_HEAP_DESC Desc;
            Desc.Type           = Type;
            Desc.NumDescriptors = SDescriptorHeapInfo::scMaxDescriptorsInHeap;
            Desc.Flags          = Flags;
            Desc.NodeMask       = 0;

            SDescriptorHeapInfo HeapInfo;
            HeapInfo.pDescriptorHeap = nullptr;

            uint32_t Index;

            if( FAILED( pDevice->CreateDescriptorHeap( &Desc, IID_PPV_ARGS( &HeapInfo.pDescriptorHeap ) ) ) )
            {
                VKE_ASSERT2( false, "Failed to create descriptor heap" );
                return nullptr;
            }

            {
                HeapInfo.SlotPool.Create( Desc.NumDescriptors );
                HeapInfo.DescriptorSize = pDevice->GetDescriptorHandleIncrementSize( Type );

                Index = m_vDescriptorHeapPool.PushBack( HeapInfo );
            }

            return &m_vDescriptorHeapPool[ Index ];
        }

        SImplementation::SDescriptorHeapInfo* SImplementation::GetDescriptorHeap( const NativeAPI::Device&    pDevice,
                                                                                  D3D12_DESCRIPTOR_HEAP_TYPE  Type,
                                                                                  D3D12_DESCRIPTOR_HEAP_FLAGS Flags )
        {
            SDescriptorHeapInfo* pDescriptorHeap = nullptr;
            for( auto& HeapInfo: m_vDescriptorHeapPool )
            {
                if( HeapInfo.Matches( Type, Flags ) && !HeapInfo.IsFull() )
                {
                    pDescriptorHeap = &HeapInfo;
                    break;
                }
            }

            if( pDescriptorHeap == nullptr )
            {
                pDescriptorHeap = CreateDescriptorHeap( pDevice, Type, Flags );
            }

            return pDescriptorHeap;
        }

        uint32_t GetLastCompletedBreadcrumb( const D3D12_AUTO_BREADCRUMB_NODE1* pNode )
        {
            uint32_t lastCompleted = 0;

            for( uint32_t i = 0; i < pNode->BreadcrumbCount; i++ )
            {
                if( pNode->pLastBreadcrumbValue && ( *pNode->pLastBreadcrumbValue & ( 1u << i ) ) )
                {
                    lastCompleted = i;
                }
            }

            return lastCompleted;
        }

        const char* GetBreadcrumbOpName( D3D12_AUTO_BREADCRUMB_OP op )
        {
            static const char* ascMap[] = {
                "SetMarker",                            // D3D12_AUTO_BREADCRUMB_OP_SETMARKER
                "BeginEvent",                           // D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT
                "EndEvent",                             // D3D12_AUTO_BREADCRUMB_OP_ENDEVENT
                "DrawInstanced",                        // D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED
                "DrawIndexedInstanced",                 // D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED
                "ExecuteIndirect",                      // D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT
                "Dispatch",                             // D3D12_AUTO_BREADCRUMB_OP_DISPATCH
                "CopyBufferRegion",                     // D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION
                "CopyTextureRegion",                    // D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION
                "CopyResource",                         // D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE
                "CopyTiles",                            // D3D12_AUTO_BREADCRUMB_OP_COPYTILES
                "ResolveSubresource",                   // D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE
                "ClearRenderTargetView",                // D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW
                "ClearUnorderedAccessView",             // D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW
                "ClearDepthStencilView",                // D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW
                "ResourceBarrier",                      // D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER
                "ExecuteBundle",                        // D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE
                "Present",                              // D3D12_AUTO_BREADCRUMB_OP_PRESENT
                "ResolveQueryData",                     // D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA
                "BeginSubmission",                      // D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION
                "EndSubmission",                        // D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION
                "DecodeFrame",                          // D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME
                "ProcessFrames",                        // D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES
                "AtomicCopyBufferUINT",                 // D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT
                "AtomicCopyBufferUINT64",               // D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT64
                "ResolveSubresourceRegion",             // D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCEREGION
                "WriteBufferImmediate",                 // D3D12_AUTO_BREADCRUMB_OP_WRITEBUFFERIMMEDIATE
                "DecodeFrame1",                         // D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME1
                "SetProtectedResourceSession",          // D3D12_AUTO_BREADCRUMB_OP_SETPROTECTEDRESOURCESESSION
                "DecodeFrame2",                         // D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME2
                "ProcessFrames1",                       // D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES1
                "BuildRaytracingAccelerationStructure", // D3D12_AUTO_BREADCRUMB_OP_BUILDRAYTRACINGACCELERATIONSTRUCTURE
                "EmitRaytracingAccelerationStructurePostBuildInfo", // D3D12_AUTO_BREADCRUMB_OP_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO
                "CopyRaytracingAccelerationStructure", // D3D12_AUTO_BREADCRUMB_OP_COPYRAYTRACINGACCELERATIONSTRUCTURE
                "DispatchRays",                        // D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS
                "InitializeMetaCommand",               // D3D12_AUTO_BREADCRUMB_OP_INITIALIZEMETACOMMAND
                "ExecuteMetaCommand",                  // D3D12_AUTO_BREADCRUMB_OP_EXECUTEMETACOMMAND
                "EstimateMotion",                      // D3D12_AUTO_BREADCRUMB_OP_ESTIMATEMOTION
                "ResolveMotionVectorHeap",             // D3D12_AUTO_BREADCRUMB_OP_RESOLVEMOTIONVECTORHEAP
                "SetPipelineState1",                   // D3D12_AUTO_BREADCRUMB_OP_SETPIPELINESTATE1
                "InitializeExtensionCommand",          // D3D12_AUTO_BREADCRUMB_OP_INITIALIZEEXTENSIONCOMMAND
                "ExecuteExtensionCommand",             // D3D12_AUTO_BREADCRUMB_OP_EXECUTEEXTENSIONCOMMAND
                "DispatchMesh",                        // D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH
                "EncodeFrame",                         // D3D12_AUTO_BREADCRUMB_OP_ENCODEFRAME
                "ResolveEncoderOutputMetadata",        // D3D12_AUTO_BREADCRUMB_OP_RESOLVEENCODEROUTPUTMETADATA
                "Barrier",                             // D3D12_AUTO_BREADCRUMB_OP_BARRIER
                "BeginCommandList",                    // D3D12_AUTO_BREADCRUMB_OP_BEGIN_COMMAND_LIST
                "DispatchGraph",                       // D3D12_AUTO_BREADCRUMB_OP_DISPATCHGRAPH
                "SetProgram",                          // D3D12_AUTO_BREADCRUMB_OP_SETPROGRAM
                "EncodeFrame1",                        // D3D12_AUTO_BREADCRUMB_OP_ENCODEFRAME1
                "ResolveEncoderOutputMetadata1",       // D3D12_AUTO_BREADCRUMB_OP_RESOLVEENCODEROUTPUTMETADATA1
                "ResolveInputParamLayout",             // D3D12_AUTO_BREADCRUMB_OP_RESOLVEINPUTPARAMLAYOUT
                "ProcessFrames2",                      // D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES2
                "SetWorkGraphMaximumGPUInputRecords", // D3D12_AUTO_BREADCRUMB_OP_SET_WORK_GRAPH_MAXIMUM_GPU_INPUT_RECORDS
            };

            if( static_cast< uint32_t >( op ) < _countof( ascMap ) )
            {
                return ascMap[ op ];
            }
            else
            {
                return "<unknown>";
            }
        }

        void HandleDeviceRemoval( NativeAPI::Device pDevice )
        {
            VKE_ASSERT2( pDevice != NativeAPI::Null, "HandleDeviceRemoval: pDevice is NULL" );

            if( !NativeAPI::SImplementation::sDebugLayerEnabled )
            {
                VKE_LOG(
                    "HandleDeviceRemoval: DRED is not enabled without NativeAPI::SImplementation::sDebugLayerEnabled" );
                return;
            }

            HRESULT reason = pDevice->GetDeviceRemovedReason();

            if( reason == S_OK )
            {
                VKE_LOG( "HandleDeviceRemoval: called but no TDR happened" );
                return;
            }

            VKE_LOG( "HandleDeviceRemoval: TDR with reason: " << std::hex << reason );

            ID3D12DeviceRemovedExtendedData1* pDRED = NativeAPI::Null;
            if( FAILED( pDevice->QueryInterface( IID_PPV_ARGS( &pDRED ) ) ) )
            {
                VKE_LOG( "HandleDeviceRemoval: Failed to query DRED" );
                return;
            }

            D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 Breadcrumbs = {};
            if( SUCCEEDED( pDRED->GetAutoBreadcrumbsOutput1( &Breadcrumbs ) ) )
            {
                VKE_LOG( "HandleDeviceRemoval: === DRED Auto-Breadcrumbs ===" );
                const D3D12_AUTO_BREADCRUMB_NODE1* pNode = NativeAPI::Null;

                for( pNode = Breadcrumbs.pHeadAutoBreadcrumbNode; pNode != nullptr; pNode = pNode->pNext )
                {
                    VKE_LOG( "HandleDeviceRemoval: Command List: "
                             << ( pNode->pCommandListDebugNameA ? pNode->pCommandListDebugNameA : "<unnamed>" ) );
                    VKE_LOG( "HandleDeviceRemoval:  Command Queue: "
                             << ( pNode->pCommandQueueDebugNameA ? pNode->pCommandQueueDebugNameA : "<unnamed>" ) );

                    uint32_t lastCompleted = GetLastCompletedBreadcrumb( pNode );
                    VKE_LOG( "HandleDeviceRemoval:  Last completed operation: " << lastCompleted << " / "
                                                                                << pNode->BreadcrumbCount );

                    for( uint32_t i = 0; i < pNode->BreadcrumbCount; i++ )
                    {
                        bool completed = pNode->pLastBreadcrumbValue && ( *pNode->pLastBreadcrumbValue & ( 1u << i ) );

                        VKE_LOG( "HandleDeviceRemoval:    [" << ( completed ? "X" : " " ) << "] Op " << i << ": "
                                                             << GetBreadcrumbOpName( pNode->pCommandHistory[ i ] ) );
                    }
                }
            }
        }

    } // namespace NativeAPI

    // -----------------------------------------------------------------------------------------------------------------
    // Map functions.
    // These are specific functions that has easy mapping Engine types to Native types. If there is more complex
    // functions, like switch/case, if/else, loops they should go to Convert namespace.
    namespace Map
    {
        D3D12_COMMAND_LIST_TYPE GetCommandListType( QUEUE_TYPE EngineType )
        {
            static const D3D12_COMMAND_LIST_TYPE ascNativeMap[] = {
                D3D12_COMMAND_LIST_TYPE_DIRECT,  // GENERAL
                D3D12_COMMAND_LIST_TYPE_COMPUTE, // COMPUTE
                D3D12_COMMAND_LIST_TYPE_COPY,    // TRANSFER
                D3D12_COMMAND_LIST_TYPE_NONE,    // SPARSE - not supported in DX12
                D3D12_COMMAND_LIST_TYPE_NONE,    // PRESENT - not supported in DX12
            };

            // Update ascNativeMap when changing QUEUE_TYPE enum
            static_assert( QUEUE_TYPE::_MAX_COUNT == _countof( ascNativeMap ) );

            return ascNativeMap[ static_cast< size_t >( EngineType ) ];
        }

        D3D12_DESCRIPTOR_HEAP_TYPE GetDescriptorHeapType( DESCRIPTOR_SET_TYPE EngineType )
        {
            static const D3D12_DESCRIPTOR_HEAP_TYPE ascNativeMap[] = {
                D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,     // SAMPLER
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, // TEXTURE
                D3D12_DESCRIPTOR_HEAP_TYPE_RTV,         // STORAGE_TEXTURE
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, // READ_ONLY_TEXEL_BUFFER  : basically read only UAV
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, // READ_WRITE_TEXEL_BUFFER : basically R/W UAV
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, // CONSTANT_BUFFER
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, // BUFFER
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, // DYNAMIC_CONSTANT_BUFFER
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, // DYNAMIC_BUFFER
                D3D12_DESCRIPTOR_HEAP_TYPE_RTV,         // RENDER_TARGET
                D3D12_DESCRIPTOR_HEAP_TYPE_DSV,         // DEPTH_STENCIL
            };

            // Update ascNativeMap when changing DESCRIPTOR_SET_TYPE enum
            static_assert( DESCRIPTOR_SET_TYPE::_MAX_COUNT == _countof( ascNativeMap ) );

            return ascNativeMap[ static_cast< size_t >( EngineType ) ];
        }

        DXGI_COLOR_SPACE_TYPE GetDXGIColorSpace( COLOR_SPACE EngineColorSpace )
        {
            static const DXGI_COLOR_SPACE_TYPE ascNativeMap[] = {
                DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709, // SRGB
            };

            // Update ascNativeMap when changing COLOR_SPACE enum
            static_assert( COLOR_SPACE::_MAX_COUNT == _countof( ascNativeMap ) );

            return ascNativeMap[ static_cast< size_t >( EngineColorSpace ) ];
        }

        D3D12_SRV_DIMENSION GetSRVDimension( TEXTURE_VIEW_TYPE EngineType )
        {
            static const D3D12_SRV_DIMENSION ascNativeMap[] = {
                D3D12_SRV_DIMENSION_TEXTURE1D,        // VIEW_1D
                D3D12_SRV_DIMENSION_TEXTURE2D,        // VIEW_2D
                D3D12_SRV_DIMENSION_TEXTURE3D,        // VIEW_3D
                D3D12_SRV_DIMENSION_TEXTURECUBE,      // VIEW_CUBE
                D3D12_SRV_DIMENSION_TEXTURE1DARRAY,   // VIEW_1D_ARRAY
                D3D12_SRV_DIMENSION_TEXTURE2DARRAY,   // VIEW_2D_ARRAY
                D3D12_SRV_DIMENSION_TEXTURECUBEARRAY, // VIEW_CUBE_ARRAY
            };

            // Update ascNativeMap when changing TEXTURE_VIEW_TYPE enum
            static_assert( TEXTURE_VIEW_TYPE::_MAX_COUNT == _countof( ascNativeMap ) );

            return ascNativeMap[ static_cast< size_t >( EngineType ) ];
        }

        D3D12_RTV_DIMENSION GetRTVDimension( TEXTURE_VIEW_TYPE EngineType )
        {
            static const D3D12_RTV_DIMENSION ascNativeMap[] = {
                D3D12_RTV_DIMENSION_TEXTURE1D,      // VIEW_1D
                D3D12_RTV_DIMENSION_TEXTURE2D,      // VIEW_2D
                D3D12_RTV_DIMENSION_TEXTURE3D,      // VIEW_3D
                D3D12_RTV_DIMENSION_UNKNOWN,        // VIEW_CUBE
                D3D12_RTV_DIMENSION_TEXTURE1DARRAY, // VIEW_1D_ARRAY
                D3D12_RTV_DIMENSION_TEXTURE2DARRAY, // VIEW_2D_ARRAY
                D3D12_RTV_DIMENSION_UNKNOWN,        // VIEW_CUBE_ARRAY
            };

            // Update ascNativeMap when changing TEXTURE_VIEW_TYPE enum
            static_assert( TEXTURE_VIEW_TYPE::_MAX_COUNT == _countof( ascNativeMap ) );

            return ascNativeMap[ static_cast< size_t >( EngineType ) ];
        }

        D3D12_UAV_DIMENSION GetUAVDimension( TEXTURE_VIEW_TYPE EngineType )
        {
            static const D3D12_UAV_DIMENSION ascNativeMap[] = {
                D3D12_UAV_DIMENSION_TEXTURE1D,      // VIEW_1D
                D3D12_UAV_DIMENSION_TEXTURE2D,      // VIEW_2D
                D3D12_UAV_DIMENSION_TEXTURE3D,      // VIEW_3D
                D3D12_UAV_DIMENSION_UNKNOWN,        // VIEW_CUBE
                D3D12_UAV_DIMENSION_TEXTURE1DARRAY, // VIEW_1D_ARRAY
                D3D12_UAV_DIMENSION_TEXTURE2DARRAY, // VIEW_2D_ARRAY
                D3D12_UAV_DIMENSION_UNKNOWN,        // VIEW_CUBE_ARRAY
            };

            // Update ascNativeMap when changing TEXTURE_VIEW_TYPE enum
            static_assert( TEXTURE_VIEW_TYPE::_MAX_COUNT == _countof( ascNativeMap ) );

            return ascNativeMap[ static_cast< size_t >( EngineType ) ];
        }

        D3D12_DSV_DIMENSION GetDSVDimension( TEXTURE_VIEW_TYPE EngineType )
        {
            static const D3D12_DSV_DIMENSION ascNativeMap[] = {
                D3D12_DSV_DIMENSION_TEXTURE1D,      // VIEW_1D
                D3D12_DSV_DIMENSION_TEXTURE2D,      // VIEW_2D
                D3D12_DSV_DIMENSION_UNKNOWN,        // VIEW_3D
                D3D12_DSV_DIMENSION_UNKNOWN,        // VIEW_CUBE
                D3D12_DSV_DIMENSION_TEXTURE1DARRAY, // VIEW_1D_ARRAY
                D3D12_DSV_DIMENSION_TEXTURE2DARRAY, // VIEW_2D_ARRAY
                D3D12_DSV_DIMENSION_UNKNOWN,        // VIEW_CUBE_ARRAY
            };

            // Update ascNativeMap when changing TEXTURE_VIEW_TYPE enum
            static_assert( TEXTURE_VIEW_TYPE::_MAX_COUNT == _countof( ascNativeMap ) );

            return ascNativeMap[ static_cast< size_t >( EngineType ) ];
        }

        D3D12_RESOURCE_DIMENSION GetResourceDimension( TEXTURE_TYPE EngineType )
        {
            // TODO(blturkot): Add support for TEXTURE_TYPE::TEX_2D_ARRAY and TEX_CUBE
            static const D3D12_RESOURCE_DIMENSION ascNativeMap[] = {
                D3D12_RESOURCE_DIMENSION_TEXTURE1D, // TEX_1D
                D3D12_RESOURCE_DIMENSION_TEXTURE2D, // TEX_2D
                D3D12_RESOURCE_DIMENSION_TEXTURE3D, // TEX_3D
                D3D12_RESOURCE_DIMENSION_UNKNOWN,
            };

            // Update ascNativeMap when changing TEXTURE_TYPE enum
            static_assert( TEXTURE_TYPE::_MAX_COUNT == _countof( ascNativeMap ) );

            return ascNativeMap[ static_cast< size_t >( EngineType ) ];
        }

        D3D12_DESCRIPTOR_HEAP_TYPE GetDescriptorHeapType( D3D12_DESCRIPTOR_RANGE_TYPE NativeRange )
        {
            static const D3D12_DESCRIPTOR_HEAP_TYPE ascNativeMap[] = {
                D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,     // D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, // D3D12_DESCRIPTOR_RANGE_TYPE_SRV
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, // D3D12_DESCRIPTOR_RANGE_TYPE_UAV
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, // D3D12_DESCRIPTOR_RANGE_TYPE_CBV
            };

            return ascNativeMap[ static_cast< size_t >( NativeRange ) ];
        }

        D3D12_RESOURCE_STATES GetResourceState( TEXTURE_STATE EngineState )
        {
            static const D3D12_RESOURCE_STATES ascNativeMap[] = {
                D3D12_RESOURCE_STATE_COMMON,              // UNDEFINED,
                D3D12_RESOURCE_STATE_COMMON,              // GENERAL,
                D3D12_RESOURCE_STATE_RENDER_TARGET,       // COLOR_RENDER_TARGET,
                D3D12_RESOURCE_STATE_DEPTH_WRITE,         // DEPTH_RENDER_TARGET,
                D3D12_RESOURCE_STATE_DEPTH_WRITE,         // STENCIL_RENDER_TARGET,
                D3D12_RESOURCE_STATE_DEPTH_WRITE,         // DEPTH_STENCIL_RENDER_TARGET,
                D3D12_RESOURCE_STATE_DEPTH_READ,          // DEPTH_BUFFER,
                D3D12_RESOURCE_STATE_DEPTH_READ,          // STENCIL_BUFFER,
                D3D12_RESOURCE_STATE_DEPTH_READ,          // DEPTH_STENCIL_BUFFER,
                D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, // SHADER_READ,
                D3D12_RESOURCE_STATE_COPY_SOURCE,         // TRANSFER_SRC,
                D3D12_RESOURCE_STATE_COPY_DEST,           // TRANSFER_DST,
                D3D12_RESOURCE_STATE_PRESENT,             // PRESENT,
                D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, // COLOR_RENDER_TARGET_READ,
                D3D12_RESOURCE_STATE_DEPTH_READ,          // DEPTH_RENDER_TARGET_READ,
                D3D12_RESOURCE_STATE_DEPTH_READ,          // STENCIL_RENDER_TARGET_READ,
                D3D12_RESOURCE_STATE_DEPTH_READ,          // DEPTH_STENCIL_RENDER_TARGET_READ,
            };

            // Update ascNativeMap when changing TEXTURE_STATE enum
            static_assert( TEXTURE_STATE::_MAX_COUNT == _countof( ascNativeMap ) );

            return ascNativeMap[ EngineState ];
        }

        D3D12_DESCRIPTOR_RANGE_TYPE GetDescriptorRangeType( BINDING_TYPE EngineType )
        {
            static const D3D12_DESCRIPTOR_RANGE_TYPE ascNativeMap[] = {
                D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, // SAMPLER
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,     // TEXTURE
                D3D12_DESCRIPTOR_RANGE_TYPE_UAV,     // STORAGE_TEXTURE
                D3D12_DESCRIPTOR_RANGE_TYPE_UAV,     // READ_ONLY_TEXEL_BUFFER  : basically read only UAV
                D3D12_DESCRIPTOR_RANGE_TYPE_UAV,     // READ_WRITE_TEXEL_BUFFER : basically R/W UAV
                D3D12_DESCRIPTOR_RANGE_TYPE_CBV,     // CONSTANT_BUFFER
                D3D12_DESCRIPTOR_RANGE_TYPE_UAV,     // BUFFER
                D3D12_DESCRIPTOR_RANGE_TYPE_CBV,     // DYNAMIC_CONSTANT_BUFFER
                D3D12_DESCRIPTOR_RANGE_TYPE_UAV,     // DYNAMIC_BUFFER
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,     // RENDER_TARGET
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,     // DEPTH_STENCIL
            };

            // Update ascNativeMap when changing TEXTURE_STATE enum
            static_assert( BINDING_TYPE::_MAX_COUNT == _countof( ascNativeMap ) );

            return ascNativeMap[ EngineType ];
        }

        D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE
        GetRenderPassBeginningAccessType( RENDER_TARGET_RENDER_PASS_OP EngineOp )
        {
            static const D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE ascNativeMap[] = {
                D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS, // UNDEFINED,
                D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD,   // COLOR, // load = dont't care, store = don't care
                D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR,     // COLOR_CLEAR, // load = clear, store = dont't care
                D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD,   // COLOR_STORE, // load = don't care, store = store
                D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR,     // COLOR_CLEAR_STORE, // load = clear, store = store
                D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD,   // DEPTH_STENCIL,
                D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR,     // DEPTH_STENCIL_CLEAR,
                D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD,   // DEPTH_STENCIL_STORE,
                D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR,     // DEPTH_STENCIL_CLEAR_STORE,
            };

            // Update ascNativeMap when changing TEXTURE_STATE enum
            static_assert( RENDER_TARGET_RENDER_PASS_OP::_MAX_COUNT == _countof( ascNativeMap ) );

            return ascNativeMap[ EngineOp ];
        }

        D3D12_RENDER_PASS_ENDING_ACCESS_TYPE GetRenderPassEndingAccessType( RENDER_TARGET_RENDER_PASS_OP EngineOp )
        {
            static const D3D12_RENDER_PASS_ENDING_ACCESS_TYPE
                ascNativeMap[ RenderTargetRenderPassOperations::_MAX_COUNT ] = {
                    D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS, // UNDEFINED,
                    D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD,   // COLOR, // load = dont't care, store = don't care
                    D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD,   // COLOR_CLEAR, // load = clear, store = dont't care
                    D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE,  // COLOR_STORE, // load = don't care, store = store
                    D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE,  // COLOR_CLEAR_STORE, // load = clear, store = store
                    D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD,   // DEPTH_STENCIL,
                    D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD,   // DEPTH_STENCIL_CLEAR,
                    D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE,  // DEPTH_STENCIL_STORE,
                    D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE,  // DEPTH_STENCIL_CLEAR_STORE,
                };

            // Update ascNativeMap when changing TEXTURE_STATE enum
            static_assert( RENDER_TARGET_RENDER_PASS_OP::_MAX_COUNT == _countof( ascNativeMap ) );

            return ascNativeMap[ EngineOp ];
        }

    }; // namespace Map

    // -----------------------------------------------------------------------------------------------------------------
    // Convert functions.
    // These are specific functions that has complex mapping from Engine types to Native types.
    namespace Convert
    {
        D3D12_SHADER_VISIBILITY GetShaderVisibility( uint16_t EngineType )
        {
            switch( EngineType )
            {
                case PipelineStages::TYPE::VERTEX:
                    return D3D12_SHADER_VISIBILITY_VERTEX;

                case PipelineStages::TYPE::TS_DOMAIN:
                    return D3D12_SHADER_VISIBILITY_DOMAIN;

                case PipelineStages::TYPE::TS_HULL:
                    return D3D12_SHADER_VISIBILITY_HULL;

                case PipelineStages::TYPE::GEOMETRY:
                    return D3D12_SHADER_VISIBILITY_GEOMETRY;

                case PipelineStages::TYPE::PIXEL:
                    return D3D12_SHADER_VISIBILITY_PIXEL;

                case PipelineStages::TYPE::MS_TASK:
                    return D3D12_SHADER_VISIBILITY_AMPLIFICATION;

                case PipelineStages::TYPE::MS_MESH:
                    return D3D12_SHADER_VISIBILITY_MESH;

                default:
                    return D3D12_SHADER_VISIBILITY_ALL;
            }
        }

        DXGI_FORMAT GetDXGIFormat( FORMAT EngineFormat )
        {
            // When changing FORMAT enum, also update g_aFormats.
            static_assert( FORMAT::_MAX_COUNT == _countof( VKE::RenderSystem::D3D12::g_aFormats ) );

            uint32_t formatIndex = static_cast< size_t >( EngineFormat );
            return VKE::RenderSystem::D3D12::g_aFormats[ formatIndex ];
        }

        D3D12_RESOURCE_STATES GetResourceState( TEXTURE_STATE EngineState, MEMORY_ACCESS_TYPE EngineMask )
        {
            D3D12_RESOURCE_STATES OutState = Map::GetResourceState( EngineState );

            // TODO(blturkot): Consider Mask for more accurate resource state.
            // OutState |= Convert::GetResourceState( Mask );

            return OutState;
        }

        D3D12_RESOURCE_STATES GetResourceState( MEMORY_ACCESS_TYPE EngineMask )
        {
            D3D12_RESOURCE_STATES State = D3D12_RESOURCE_STATE_COMMON;

            if( EngineMask & ( MemoryAccessTypes::CPU_MEMORY_READ | MemoryAccessTypes::CPU_MEMORY_WRITE |
                               MemoryAccessTypes::GPU_MEMORY_READ | MemoryAccessTypes::GPU_MEMORY_WRITE ) )
            {
                // DX12 requires CPU access to be in COMMON state.
                return State;
            }

            if( EngineMask & MemoryAccessTypes::INDIRECT_BUFFER_READ )
            {
                State |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
            }

            if( EngineMask & MemoryAccessTypes::INDEX_READ )
            {
                State |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
            }

            if( EngineMask & ( MemoryAccessTypes::VERTEX_ATTRIBUTE_READ | MemoryAccessTypes::VS_UNIFORM_READ |
                               MemoryAccessTypes::PS_UNIFORM_READ | MemoryAccessTypes::GS_UNIFORM_READ |
                               MemoryAccessTypes::TS_UNIFORM_READ | MemoryAccessTypes::CS_UNIFORM_READ |
                               MemoryAccessTypes::MS_UNIFORM_READ | MemoryAccessTypes::RT_UNIFORM_READ ) )
            {
                State |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
            }

            if( EngineMask & MemoryAccessTypes::INPUT_ATTACHMENT_READ )
            {
                State |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            }

            // Is this actually UAV?
            if( EngineMask & ( MemoryAccessTypes::VS_SHADER_READ | MemoryAccessTypes::GS_SHADER_READ |
                               MemoryAccessTypes::TS_SHADER_READ | MemoryAccessTypes::CS_SHADER_READ |
                               MemoryAccessTypes::MS_SHADER_READ | MemoryAccessTypes::RS_SHADER_READ ) )
            {
                State |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            }
            // And this?
            if( EngineMask & MemoryAccessTypes::PS_SHADER_READ )
            {
                State |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            }

            if( EngineMask & ( MemoryAccessTypes::VS_SHADER_WRITE | MemoryAccessTypes::GS_SHADER_WRITE |
                               MemoryAccessTypes::TS_SHADER_WRITE | MemoryAccessTypes::CS_SHADER_WRITE |
                               MemoryAccessTypes::MS_SHADER_WRITE | MemoryAccessTypes::RS_SHADER_WRITE |
                               MemoryAccessTypes::PS_SHADER_WRITE ) )
            {
                State |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            }

            if( EngineMask &
                ( MemoryAccessTypes::COLOR_RENDER_TARGET_READ | MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ ) )
            {
                State |= D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
            }

            if( EngineMask & ( MemoryAccessTypes::COLOR_RENDER_TARGET_WRITE |
                               MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_WRITE ) )
            {
                State |= D3D12_RESOURCE_STATE_RENDER_TARGET;
            }

            if( EngineMask & ( MemoryAccessTypes::DATA_TRANSFER_READ ) )
            {
                State |= D3D12_RESOURCE_STATE_COPY_SOURCE;
            }

            if( EngineMask & ( MemoryAccessTypes::DATA_TRANSFER_WRITE ) )
            {
                State |= D3D12_RESOURCE_STATE_COPY_DEST;
            }

            return State;
        }

#if 0
        // This is for enhanced barriers. Need to check capabilities and add another path for barriers.
        vke_force_inline D3D12_BARRIER_SUBRESOURCE_RANGE GetSubresourceIndex( NativeAPI::Texture              hTexture,
                                                                              const STextureSubresourceRange& Range )
        {
            D3D12_BARRIER_SUBRESOURCE_RANGE ddiRange;
            ddiRange.IndexOrFirstMipLevel = Range.beginMipmapLevel;
            ddiRange.NumMipLevels         = Range.mipmapLevelCount;
            ddiRange.FirstArraySlice      = Range.beginArrayLayer;
            ddiRange.NumArraySlices       = Range.layerCount;
            ddiRange.FirstPlane           = 0;
            ddiRange.NumPlanes            = 1;

            return ddiRange;
        }
#endif
        vke_force_inline UINT GetSubresourceIndex( UINT MipLevel, UINT MipCount, UINT ArraySlice, UINT ArraySliceCount,
                                                   UINT Plane )
        {
            return MipLevel + ( ArraySlice * MipCount ) + ( Plane * MipCount * ArraySliceCount );
        }

        DXGI_SAMPLE_DESC GetSampleDesc( SAMPLE_COUNT EngineCount )
        {
            DXGI_SAMPLE_DESC Desc;
            Desc.Count   = ( 1 << EngineCount );
            Desc.Quality = 0;
            return Desc;
        }

        D3D12_RESOURCE_FLAGS GetResourceFlags( TEXTURE_USAGE EngineUsage )
        {
            D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE;

            if( ( EngineUsage & TextureUsages::DEPTH_STENCIL_RENDER_TARGET ) )
            {
                Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

                if( ( EngineUsage & TextureUsages::SAMPLED ) == 0 )
                {
                    Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
                }
            }
            else
            {
                // In DX12 D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL cannot be set with either:
                // - D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
                // - D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
                // - D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS
                if( ( EngineUsage & TextureUsages::STORAGE ) )
                {
                    Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
                }

                if( ( EngineUsage & TextureUsages::COLOR_RENDER_TARGET ) )
                {
                    Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
                }
            }
            return Flags;
        }

        D3D12_RESOURCE_FLAGS GetResourceFlags( BUFFER_USAGE EngineUsage )
        {
            D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE;

            if( ( EngineUsage & BufferUsages::TEXEL_BUFFER ) )
            {
                Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            }

            return Flags;
        }

        D3D12_HEAP_DESC GetMemoryHeapDesc( MEMORY_USAGE EngineUsage, bool HeapTier2 )
        {
            static const MEMORY_USAGE AccessMaskBits = MemoryUsages::CPU_ACCESS | MemoryUsages::GPU_ACCESS |
                                                       MemoryUsages::CPU_CACHED | MemoryUsages::CPU_NO_FLUSH;

            static const MEMORY_USAGE vPreconfiguredHeaps[] = {
                // D3D12_HEAP_TYPE_DEFAULT
                MemoryUsages::GPU_ACCESS | !MemoryUsages::CPU_ACCESS,
                // D3D12_HEAP_TYPE_UPLOAD
                MemoryUsages::GPU_ACCESS | MemoryUsages::CPU_ACCESS | MemoryUsages::CPU_NO_FLUSH,
                // D3D12_HEAP_TYPE_READBACK
                MemoryUsages::GPU_ACCESS | MemoryUsages::CPU_ACCESS | MemoryUsages::CPU_CACHED,
            };

            bool AllowBuffers  = EngineUsage & MemoryUsages::BUFFER;
            bool AllowTextures = EngineUsage & MemoryUsages::TEXTURE;
            bool CPUAccess     = EngineUsage & MemoryUsages::CPU_ACCESS;
            bool GPUAccess     = EngineUsage & MemoryUsages::GPU_ACCESS;
            bool IsWriteback   = EngineUsage & MemoryUsages::CPU_CACHED;
            bool IsUpload      = EngineUsage & MemoryUsages::CPU_NO_FLUSH;

            D3D12_HEAP_DESC Desc;
            Desc.SizeInBytes = 0;                                          // To be filled later
            Desc.Alignment   = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT; // Default alignment

            // Reset node mask
            Desc.Properties.CreationNodeMask = 0;
            Desc.Properties.VisibleNodeMask  = 0;

            if( GPUAccess && !CPUAccess )
            {
                // This heap type experiences the most bandwidth for the GPU, but cannot provide CPU access. The GPU can
                // read and write to the memory from this pool, and resource transition barriers may be changed. The
                // majority of heaps and resources are expected to be located here, and are typically populated through
                // resources in upload heaps.
                Desc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
            }
            else if( GPUAccess && CPUAccess )
            {
                if( EngineUsage & MemoryUsages::CPU_NO_FLUSH )
                {
                    // This heap type has CPU access optimized for uploading to the GPU, but does not experience the
                    // maximum amount of bandwidth for the GPU. This heap type is best for CPU-write-once, GPU-read-once
                    // data; but GPU-read-once is stricter than necessary. GPU-read-once-or-from-cache is an acceptable
                    // use-case for the data; but such usages are hard to judge due to differing GPU cache designs and
                    // sizes. If in doubt, stick to the GPU-read-once definition or profile the difference on many GPUs
                    // between copying the data to a _DEFAULT heap vs. reading the data from an _UPLOAD heap.

                    // Resources in this heap must be created with D3D12_RESOURCE_STATE_GENERIC_READ and cannot be
                    // changed away from this. The CPU address for such heaps is commonly not efficient for CPU reads.
                    Desc.Properties.Type = D3D12_HEAP_TYPE_UPLOAD;
                }
                else if( EngineUsage & MemoryUsages::CPU_CACHED )
                {
                    // Specifies a heap used for reading back. This heap type has CPU access optimized for reading data
                    // back from the GPU, but does not experience the maximum amount of bandwidth for the GPU. This heap
                    // type is best for GPU-write-once, CPU-readable data. The CPU cache behavior is write-back, which
                    // is conducive for multiple sub-cache-line CPU reads.

                    // Resources in this heap must be created with D3D12_RESOURCE_STATE_COPY_DEST,
                    // and cannot be changed away from this.
                    Desc.Properties.Type = D3D12_HEAP_TYPE_READBACK;
                }
                else
                {
                    // The application may specify the memory pool and CPU cache properties directly, which can be
                    // useful for UMA optimizations, multi-engine, multi-adapter, or other special cases. To do so, the
                    // application is expected to understand the adapter architecture to make the right choice.
                    Desc.Properties.Type = D3D12_HEAP_TYPE_CUSTOM;
                }
            }
            else
            {
                Desc.Properties.Type = D3D12_HEAP_TYPE_CUSTOM;
            }

            // Reset remaining fields from descriptor. Predefined heaps already have correct properties and API requires
            // to set UNKNOWN for them. Otherwise it will report runtime error.
            Desc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            Desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

            // Custom heaps requires overriding.
            if( Desc.Properties.Type == D3D12_HEAP_TYPE_CUSTOM )
            {
                if( !CPUAccess )
                {
                    // No CPU access, acts like DEFAULT heap.
                    Desc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;
                    Desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_L1;
                }
                else if( CPUAccess && IsWriteback )
                {
                    Desc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
                    Desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
                }
                else if( CPUAccess && IsUpload )
                {
                    Desc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
                    Desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
                }
            }

            // TODO(szymansk): I've added this flag assuming engine will handle not zeroed resources. Or should we allow
            // API to zero?
            // desc.Flags = D3D12_HEAP_FLAG_CREATE_NOT_ZEROED;
            Desc.Flags = D3D12_HEAP_FLAG_NONE;

            if( ( AllowBuffers && AllowTextures ) || HeapTier2 )
            {
                // TODO(blturkot): This allows all buffers and textures but it is only required when HW supports Tier2
                // heaps.
                Desc.Flags |= D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
            }
            else
            {
                if( !AllowBuffers )
                {
                    Desc.Flags |= D3D12_HEAP_FLAG_DENY_BUFFERS;
                }

                if( !AllowTextures )
                {
                    Desc.Flags |= D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES;
                }
            }

            return Desc;
        }

        void GetResourceDesc( const STextureDesc&                                EngineDesc,
                              const NativeAPI::SImplementation::SDeviceFeatures& NativeFeatures,
                              NativeAPI::D3D12ResourceDesc*                      pOut )
        {
            NativeAPI::D3D12ResourceDesc& OutDesc = *pOut;

            OutDesc.Dimension = Map::GetResourceDimension( EngineDesc.type );
            OutDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
            OutDesc.Width     = static_cast< UINT64 >( EngineDesc.Size.width );
            OutDesc.Height    = static_cast< UINT >( EngineDesc.Size.height );

            if( EngineDesc.type == TEXTURE_TYPE::TEXTURE_3D )
            {
                OutDesc.DepthOrArraySize = static_cast< UINT16 >( EngineDesc.sliceCount );
            }
            else
            {
                OutDesc.DepthOrArraySize = static_cast< UINT16 >( EngineDesc.arrayElementCount );
            }

            OutDesc.MipLevels  = static_cast< UINT16 >( EngineDesc.mipmapCount );
            OutDesc.Format     = Convert::GetDXGIFormat( EngineDesc.format );
            OutDesc.SampleDesc = Convert::GetSampleDesc( EngineDesc.multisampling );
            OutDesc.Layout     = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            OutDesc.Flags      = Convert::GetResourceFlags( EngineDesc.usage );

            if( NativeFeatures.TightAlignmentSupported )
            {
                OutDesc.Alignment  = 0;
                OutDesc.Flags     |= D3D12_RESOURCE_FLAG_USE_TIGHT_ALIGNMENT;
            }
        }

        NativeAPI::D3D12ResourceDesc
        GetResourceDesc( const SBufferDesc&                                 EngineDesc,
                         const NativeAPI::SImplementation::SDeviceFeatures& NativeFeatures )
        {
            NativeAPI::D3D12ResourceDesc OutDesc;

            OutDesc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
            OutDesc.Alignment        = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
            OutDesc.Width            = static_cast< UINT64 >( EngineDesc.CalcSize() );
            OutDesc.Height           = 1;
            OutDesc.DepthOrArraySize = 1;
            OutDesc.MipLevels        = 1;
            OutDesc.Format           = DXGI_FORMAT_UNKNOWN;

            OutDesc.SampleDesc.Count   = 1;
            OutDesc.SampleDesc.Quality = 0;

            OutDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            OutDesc.Flags  = Convert::GetResourceFlags( EngineDesc.usage );

            if( NativeFeatures.TightAlignmentSupported )
            {
                OutDesc.Alignment  = 0;
                OutDesc.Flags     |= D3D12_RESOURCE_FLAG_USE_TIGHT_ALIGNMENT;
            }

            return OutDesc;
        }

        UINT32 GetPixColor( const VKE::RenderSystem::SColor& EngineColor )
        {
            // Spec URL:
            // https://devblogs.microsoft.com/pix/winpixeventruntime/
            // raw DWORD noting that the format is ARGB and the alpha channel value must be 0xff
            return PIX_COLOR( static_cast< UINT8 >( std::lround( std::clamp( EngineColor.r, 0.0f, 1.0f ) * 0xFF ) ),
                              static_cast< UINT8 >( std::lround( std::clamp( EngineColor.g, 0.0f, 1.0f ) * 0xFF ) ),
                              static_cast< UINT8 >( std::lround( std::clamp( EngineColor.b, 0.0f, 1.0f ) * 0xFF ) ) );
        }

        D3D12_CLEAR_VALUE GetClearValue( const SRenderTargetInfo& EngineInfo )
        {
            D3D12_CLEAR_VALUE ClearValue;
            ClearValue.Format = Convert::GetDXGIFormat( EngineInfo.format );

            ClearValue.Color[ 0 ] = EngineInfo.ClearColor.Color.r;
            ClearValue.Color[ 1 ] = EngineInfo.ClearColor.Color.g;
            ClearValue.Color[ 2 ] = EngineInfo.ClearColor.Color.b;
            ClearValue.Color[ 3 ] = EngineInfo.ClearColor.Color.a;

            return ClearValue;
        }

        D3D12_DSV_FLAGS GetDepthStencilViewFlags()
        {
            return D3D12_DSV_FLAG_NONE;
        }

        D3D12_CLEAR_FLAGS GetClearDepthStencilViewFlags( TEXTURE_FORMAT EngineFormat )
        {
            D3D12_CLEAR_FLAGS flags = (D3D12_CLEAR_FLAGS)0;

            if( VKE::RenderSystem::IsDepthFormat( EngineFormat ) )
            {
                flags |= D3D12_CLEAR_FLAG_DEPTH;
            }

            if( VKE::RenderSystem::IsStencilFormat( EngineFormat ) )
            {
                flags |= D3D12_CLEAR_FLAG_STENCIL;
            }

            return flags;
        }

        void GetRect( const Rect2DI32& EngineRect, D3D12_RECT* pNativeRect )
        {
            pNativeRect->left   = EngineRect.Position.x;
            pNativeRect->top    = EngineRect.Position.y;
            pNativeRect->right  = pNativeRect->left + EngineRect.Size.width;
            pNativeRect->bottom = pNativeRect->top + EngineRect.Size.height;
        }

    }; // namespace Convert

    // -----------------------------------------------------------------------------------------------------------------
    // Helper functions.
    // These are not directly to translate but have common code across multiple CDDI functions. This is just to prevent
    // duplicated code and work on function that has to be easy to refactor.
    namespace Helper
    {

        D3D_FEATURE_LEVEL GetMaxFeatureLevel( IDXGIAdapter1* pAdapter )
        {
            static const D3D_FEATURE_LEVEL vFeatureLevels[] = {
                D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0,
                D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
            };

            D3D_FEATURE_LEVEL MaxLevel = vFeatureLevels[ _countof( vFeatureLevels ) - 1 ];

            ID3D12Device* pDevice = NativeAPI::Null;
            HRESULT       Result  = S_OK;

            for( auto Level: vFeatureLevels )
            {
                Result = D3D12CreateDevice( pAdapter, Level, IID_PPV_ARGS( &pDevice ) );

                if( SUCCEEDED( Result ) )
                {
                    MaxLevel = Level;
                    pDevice->Release();
                    break;
                }
            }

            return MaxLevel;
        }

        UINT GetNodeMask()
        {
            // TODO(blturkot): Implement node mask when engine enable multi adapter rendering.
            return 0;
        }

        Result QueryAdapterProperties( const NativeAPI::Adapter& hAdapter, SDeviceProperties* pOut )
        {
            Memory::Zero( &pOut->Features );
            Memory::Zero( &pOut->Limits );
            Memory::Zero( &pOut->Properties );

            // Query memory properties
            DXGI_QUERY_VIDEO_MEMORY_INFO VideoMemoryInfo = {};

            HRESULT hr;
            if( FAILED( hr = hAdapter->QueryVideoMemoryInfo( 0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &VideoMemoryInfo ) ) )
            {
                VKE_LOG_ERR( "CDDI::QueryDeviceInfo: QueryVideoMemoryInfo failed with error code " +
                             std::to_string( hr ) );
            }

            pOut->Properties.Memory.localBudget = VideoMemoryInfo.Budget;

            if( FAILED(
                    hr = hAdapter->QueryVideoMemoryInfo( 0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &VideoMemoryInfo ) ) )
            {
                VKE_LOG_ERR( "CDDI::QueryDeviceInfo: QueryVideoMemoryInfo failed with error code " +
                             std::to_string( hr ) );
            }

            pOut->Properties.Memory.hostBudget = VideoMemoryInfo.Budget;

            return Result::OK;
        }

        bool ValidateBarrier( const D3D12_RESOURCE_BARRIER& Barrier, MEMORY_ACCESS_TYPE SrcAccessType,
                              MEMORY_ACCESS_TYPE DstAccessType )
        {
            bool IsValid = true;

            if( Barrier.Transition.StateBefore == Barrier.Transition.StateAfter )
            {
                VKE_LOG_WARN( "CDDI::ValidateBarrier: Translation resulted in no transition." );
                IsValid = false;
            }

            if( IsValid == false )
            {
                std::stringstream src;
                std::stringstream dst;

                src << "SrcAccessType: ";
                dst << "DstAccessType: ";

                for( uint32_t i = 1; i < MemoryAccessTypes::_MAX_COUNT; i++ )
                {
                    if( SrcAccessType & ( 1ull << i ) )
                    {
                        src << i << " ";
                    }
                    if( DstAccessType & ( 1ull << i ) )
                    {
                        dst << i << " ";
                    }
                }
                VKE_LOG_WARN( src.str() );
                VKE_LOG_WARN( dst.str() );
            }

            return IsValid;
        }

        void CreateLegacySubresourceBarriers( const STextureBarrierInfo& Info, DDIBarrierArray& OutArray )
        {
            if( Info.currentState == Info.newState && Info.srcMemoryAccess == Info.dstMemoryAccess )
            {
                // TODO(szymansk): This assert should never be hit, engine must prevent transitioning same state.
                VKE_LOG_WARN( "CDDI::Barrier: Source and destination memory access masks are the same, DX12 doesn't "
                              "allow that." );
                return;
            }

            // Used when Texture was a custom struct.
            // const NativeAPI::D3D12ResourceDesc& desc = Info.hDDITexture->Desc;
            NativeAPI::D3D12ResourceDesc desc = Info.hDDITexture->GetDesc();

            UINT textureMipLevels = ( desc.MipLevels > 0 ) ? desc.MipLevels : 1;
            UINT textureArraySize = ( desc.DepthOrArraySize > 0 ) ? desc.DepthOrArraySize : 1;

            if( desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D )
            {
                textureArraySize = 1;
            }

            auto& range          = Info.SubresourceRange;
            bool  isFullResource = ( textureMipLevels == range.mipmapLevelCount ) &&
                                  ( textureArraySize == range.layerCount ) && ( range.beginArrayLayer == 0 ) &&
                                  ( range.beginMipmapLevel == 0 );

            if( isFullResource )
            {
                D3D12_RESOURCE_BARRIER barrier;
                barrier.Type  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

                auto& transition       = barrier.Transition;
                transition.pResource   = Info.hDDITexture;
                transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                transition.StateBefore = Convert::GetResourceState( Info.currentState, Info.srcMemoryAccess );
                transition.StateAfter  = Convert::GetResourceState( Info.newState, Info.dstMemoryAccess );

                if( ValidateBarrier( barrier, Info.srcMemoryAccess, Info.dstMemoryAccess ) )
                {
                    OutArray.PushBack( barrier );
                }
            }
            else
            {
                for( uint32_t layer = 0; layer < Info.SubresourceRange.layerCount; layer++ )
                {
                    for( uint32_t mip = 0; mip < Info.SubresourceRange.mipmapLevelCount; mip++ )
                    {
                        D3D12_RESOURCE_BARRIER barrier;
                        barrier.Type  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

                        auto& transition     = barrier.Transition;
                        transition.pResource = Info.hDDITexture;
                        transition.Subresource =
                            Convert::GetSubresourceIndex( Info.SubresourceRange.beginMipmapLevel + mip,
                                                          textureMipLevels,
                                                          Info.SubresourceRange.beginArrayLayer + layer,
                                                          textureArraySize,
                                                          1 );

                        transition.StateBefore = Convert::GetResourceState( Info.currentState, Info.srcMemoryAccess );
                        transition.StateAfter  = Convert::GetResourceState( Info.newState, Info.dstMemoryAccess );

                        if( ValidateBarrier( barrier, Info.srcMemoryAccess, Info.dstMemoryAccess ) )
                        {
                            OutArray.PushBack( barrier );
                        }
                    }
                }
            }
        }

        void CreateLegacySubresourceBarriers( const SBufferBarrierInfo& Info, DDIBarrierArray& OutArray )
        {
            if( Info.srcMemoryAccess == Info.dstMemoryAccess )
            {
                // TODO(szymansk): This assert should never be hit, engine must prevent transitioning same state.
                VKE_LOG_WARN( "CDDI::Barrier: Source and destination memory access masks are the same, DX12 doesn't "
                              "allow that." );
                return;
            }

            D3D12_RESOURCE_BARRIER barrier;
            barrier.Type  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

            auto& transition       = barrier.Transition;
            transition.pResource   = Info.hDDIBuffer;
            transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            transition.StateBefore = Convert::GetResourceState( Info.srcMemoryAccess );
            transition.StateAfter  = Convert::GetResourceState( Info.dstMemoryAccess );

            if( ValidateBarrier( barrier, Info.srcMemoryAccess, Info.dstMemoryAccess ) )
            {
                OutArray.PushBack( barrier );
            }
        }

        template< typename ViewDimensionType >
        void SetCommonMipParams( ViewDimensionType& Type, const STextureSubresourceRange& SubresourceRange )
        {
            Type.MostDetailedMip     = SubresourceRange.beginMipmapLevel;
            Type.MipLevels           = SubresourceRange.mipmapLevelCount;
            Type.ResourceMinLODClamp = 0.0f;
        }

        void CreateShaderResourceView( const STextureViewDesc& TextureViewDesc, D3D12_SHADER_RESOURCE_VIEW_DESC* pOut )
        {
            pOut->Format                  = Convert::GetDXGIFormat( TextureViewDesc.format );
            pOut->ViewDimension           = Map::GetSRVDimension( TextureViewDesc.type );
            pOut->Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

            switch( TextureViewDesc.type )
            {
                case TEXTURE_VIEW_TYPE::VIEW_1D:
                    SetCommonMipParams( pOut->Texture1D, TextureViewDesc.SubresourceRange );
                    break;

                case TEXTURE_VIEW_TYPE::VIEW_2D:
                    SetCommonMipParams( pOut->Texture2D, TextureViewDesc.SubresourceRange );
                    pOut->Texture2D.PlaneSlice = 0;
                    break;

                case TEXTURE_VIEW_TYPE::VIEW_3D:
                    SetCommonMipParams( pOut->Texture3D, TextureViewDesc.SubresourceRange );
                    break;

                case TEXTURE_VIEW_TYPE::VIEW_CUBE:
                    SetCommonMipParams( pOut->TextureCube, TextureViewDesc.SubresourceRange );
                    break;

                case TEXTURE_VIEW_TYPE::VIEW_1D_ARRAY:
                    SetCommonMipParams( pOut->Texture1DArray, TextureViewDesc.SubresourceRange );
                    pOut->Texture1DArray.FirstArraySlice = TextureViewDesc.SubresourceRange.beginArrayLayer;
                    pOut->Texture1DArray.ArraySize       = TextureViewDesc.SubresourceRange.layerCount;
                    break;

                case TEXTURE_VIEW_TYPE::VIEW_2D_ARRAY:
                    SetCommonMipParams( pOut->Texture2DArray, TextureViewDesc.SubresourceRange );
                    pOut->Texture2DArray.FirstArraySlice = TextureViewDesc.SubresourceRange.beginArrayLayer;
                    pOut->Texture2DArray.ArraySize       = TextureViewDesc.SubresourceRange.layerCount;
                    pOut->Texture2DArray.PlaneSlice      = 0;
                    break;

                case TEXTURE_VIEW_TYPE::VIEW_CUBE_ARRAY:
                    SetCommonMipParams( pOut->TextureCubeArray, TextureViewDesc.SubresourceRange );
                    pOut->TextureCubeArray.First2DArrayFace = TextureViewDesc.SubresourceRange.beginArrayLayer;
                    pOut->TextureCubeArray.NumCubes         = TextureViewDesc.SubresourceRange.layerCount;
                    break;

                default:
                    VKE_LOG_ERR( "D3D12::CreateShaderResourceView: unhandled TEXTURE_VIEW_TYPE: "
                                 << static_cast< uint32_t >( TextureViewDesc.type ) );
                    break;
            }
        }

        void CreateRenderTargetView( const STextureViewDesc& TextureViewDesc, D3D12_RENDER_TARGET_VIEW_DESC* pOut )
        {
            pOut->Format        = Convert::GetDXGIFormat( TextureViewDesc.format );
            pOut->ViewDimension = Map::GetRTVDimension( TextureViewDesc.type );

            switch( TextureViewDesc.type )
            {
                case TEXTURE_VIEW_TYPE::VIEW_1D:
                    pOut->Texture1D.MipSlice = TextureViewDesc.SubresourceRange.beginMipmapLevel;
                    break;

                case TEXTURE_VIEW_TYPE::VIEW_2D:
                    pOut->Texture2D.MipSlice   = TextureViewDesc.SubresourceRange.beginMipmapLevel;
                    pOut->Texture2D.PlaneSlice = 0;
                    break;

                case TEXTURE_VIEW_TYPE::VIEW_3D:
                    pOut->Texture3D.MipSlice    = TextureViewDesc.SubresourceRange.beginMipmapLevel;
                    pOut->Texture3D.FirstWSlice = 0;
                    pOut->Texture3D.WSize       = TextureViewDesc.SubresourceRange.layerCount;
                    break;

                case TEXTURE_VIEW_TYPE::VIEW_CUBE:
                    VKE_LOG_ERR( "D3D12::CreateRenderTargetView: TEXTURE_VIEW_TYPE::VIEW_CUBE is not supported" );
                    break;

                case TEXTURE_VIEW_TYPE::VIEW_1D_ARRAY:
                    pOut->Texture1DArray.MipSlice        = TextureViewDesc.SubresourceRange.beginMipmapLevel;
                    pOut->Texture1DArray.FirstArraySlice = TextureViewDesc.SubresourceRange.beginArrayLayer;
                    pOut->Texture1DArray.ArraySize       = TextureViewDesc.SubresourceRange.layerCount;
                    break;

                case TEXTURE_VIEW_TYPE::VIEW_2D_ARRAY:
                    pOut->Texture2DArray.MipSlice        = TextureViewDesc.SubresourceRange.beginMipmapLevel;
                    pOut->Texture2DArray.FirstArraySlice = TextureViewDesc.SubresourceRange.beginArrayLayer;
                    pOut->Texture2DArray.ArraySize       = TextureViewDesc.SubresourceRange.layerCount;
                    pOut->Texture2DArray.PlaneSlice      = 0;
                    break;

                case TEXTURE_VIEW_TYPE::VIEW_CUBE_ARRAY:
                    VKE_LOG_ERR( "D3D12::CreateRenderTargetView: TEXTURE_VIEW_TYPE::VIEW_CUBE_ARRAY is not supported" );
                    break;

                default:
                    VKE_LOG_ERR( "D3D12::CreateRenderTargetView: unhandled TEXTURE_VIEW_TYPE: "
                                 << static_cast< uint32_t >( TextureViewDesc.type ) );
                    break;
            }
        }

        void CreateUnorderedAccessView( const STextureViewDesc&           TextureViewDesc,
                                        D3D12_UNORDERED_ACCESS_VIEW_DESC* pOut )
        {
            pOut->Format        = Convert::GetDXGIFormat( TextureViewDesc.format );
            pOut->ViewDimension = Map::GetUAVDimension( TextureViewDesc.type );

            switch( TextureViewDesc.type )
            {
                case TEXTURE_VIEW_TYPE::VIEW_1D:
                    pOut->Texture1D.MipSlice = TextureViewDesc.SubresourceRange.beginMipmapLevel;
                    break;

                case TEXTURE_VIEW_TYPE::VIEW_2D:
                    pOut->Texture2D.MipSlice   = TextureViewDesc.SubresourceRange.beginMipmapLevel;
                    pOut->Texture2D.PlaneSlice = 0;
                    break;

                case TEXTURE_VIEW_TYPE::VIEW_3D:
                    pOut->Texture3D.MipSlice    = TextureViewDesc.SubresourceRange.beginMipmapLevel;
                    pOut->Texture3D.FirstWSlice = 0;
                    pOut->Texture3D.WSize       = TextureViewDesc.SubresourceRange.layerCount;
                    break;

                case TEXTURE_VIEW_TYPE::VIEW_CUBE:
                    VKE_LOG_ERR( "D3D12::CreateUnorderedAccessView: TEXTURE_VIEW_TYPE::VIEW_CUBE is not supported" );
                    break;

                case TEXTURE_VIEW_TYPE::VIEW_1D_ARRAY:
                    pOut->Texture1DArray.MipSlice        = TextureViewDesc.SubresourceRange.beginMipmapLevel;
                    pOut->Texture1DArray.FirstArraySlice = TextureViewDesc.SubresourceRange.beginArrayLayer;
                    pOut->Texture1DArray.ArraySize       = TextureViewDesc.SubresourceRange.layerCount;
                    break;

                case TEXTURE_VIEW_TYPE::VIEW_2D_ARRAY:
                    pOut->Texture2DArray.MipSlice        = TextureViewDesc.SubresourceRange.beginMipmapLevel;
                    pOut->Texture2DArray.FirstArraySlice = TextureViewDesc.SubresourceRange.beginArrayLayer;
                    pOut->Texture2DArray.ArraySize       = TextureViewDesc.SubresourceRange.layerCount;
                    pOut->Texture2DArray.PlaneSlice      = 0;
                    break;

                case TEXTURE_VIEW_TYPE::VIEW_CUBE_ARRAY:
                    VKE_LOG_ERR(
                        "D3D12::CreateUnorderedAccessView: TEXTURE_VIEW_TYPE::VIEW_CUBE_ARRAY is not supported" );
                    break;

                default:
                    VKE_LOG_ERR( "D3D12::CreateUnorderedAccessView: unhandled TEXTURE_VIEW_TYPE: "
                                 << static_cast< uint32_t >( TextureViewDesc.type ) );
                    break;
            }
        }

        void CreateDepthStencilViewDesc( const STextureViewDesc& TextureViewDesc, D3D12_DEPTH_STENCIL_VIEW_DESC* pOut )
        {
            pOut->Format        = Convert::GetDXGIFormat( TextureViewDesc.format );
            pOut->ViewDimension = Map::GetDSVDimension( TextureViewDesc.type );
            pOut->Flags         = Convert::GetDepthStencilViewFlags();

            switch( TextureViewDesc.type )
            {
                case TEXTURE_VIEW_TYPE::VIEW_1D:
                case TEXTURE_VIEW_TYPE::VIEW_2D:
                    pOut->Texture1D.MipSlice = TextureViewDesc.SubresourceRange.beginMipmapLevel;
                    break;

                case TEXTURE_VIEW_TYPE::VIEW_1D_ARRAY:
                case TEXTURE_VIEW_TYPE::VIEW_2D_ARRAY:
                    pOut->Texture1DArray.MipSlice        = TextureViewDesc.SubresourceRange.beginMipmapLevel;
                    pOut->Texture1DArray.FirstArraySlice = TextureViewDesc.SubresourceRange.beginArrayLayer;
                    pOut->Texture1DArray.ArraySize       = TextureViewDesc.SubresourceRange.layerCount;
                    break;

                default:
                    VKE_LOG_ERR( "D3D12::CreateDepthStencilViewDesc: unhandled TEXTURE_VIEW_TYPE: "
                                 << static_cast< uint32_t >( TextureViewDesc.type ) );
                    break;
            }
        }

        void ExpectResourceState( NativeAPI::D3D12Resource* pResource, D3D12_RESOURCE_STATES expectedState,
                                  D3D12_RESOURCE_STATES                                     currentState,
                                  NativeAPI::CustomTypes::SRenderPass::SRenderPassBarriers* pOutBarriers )
        {
            if( expectedState == currentState )
            {
                return;
            }

            D3D12_RESOURCE_BARRIER& Barrier = pOutBarriers->Reserve();

            Barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            Barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            Barrier.Transition.pResource   = pResource;
            Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            Barrier.Transition.StateBefore = currentState;
            Barrier.Transition.StateAfter  = expectedState;
        }

    }; // namespace Helper

    // -----------------------------------------------------------------------------------------------------------------
    // Static methods.
    // CDDI class implementation.

    Result CDDI::QueryAdapters( AdapterInfoArray* pOut )
    {
        static const size_t MAX_ADAPTERS = 5;
        auto                pFactory     = NativeAPI::SImplementation::spFactory;

        if( pFactory == NativeAPI::Null )
        {
            VKE_LOG_ERR( "CDDI::QueryAdapters: DXGI Factory is null" );
            return VKE_FAIL;
        }

        UINT    Index  = 0;
        HRESULT Result = S_OK;

        // Limit adapters by high performance preference
        while( Index < MAX_ADAPTERS )
        {
            IDXGIAdapter1*     pAdapter1 = NativeAPI::Null;
            NativeAPI::Adapter pAdapter  = NativeAPI::Null;

            Result = pFactory->EnumAdapterByGpuPreference(
                Index++, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS( &pAdapter1 ) );

            if( Result == DXGI_ERROR_NOT_FOUND )
            {
                break;
            }

            if( FAILED( pAdapter1->QueryInterface( IID_PPV_ARGS( &pAdapter ) ) ) )
            {
                VKE_LOG_ERR( "CDDI::QueryAdapters: Query NativeAPI::Adapter failed" );
            }

            DXGI_ADAPTER_DESC3 AdapterDesc;
            if( FAILED( pAdapter->GetDesc3( &AdapterDesc ) ) )
            {
                VKE_LOG_ERR( "CDDI::QueryAdapters: Fail getting descriptor" );
            }

            VKE::RenderSystem::SAdapterInfo AdapterInfo = {};

            AdapterInfo.deviceID = static_cast< uint32_t >( AdapterDesc.DeviceId ); // from: UINT
            AdapterInfo.vendorID = static_cast< uint32_t >( AdapterDesc.VendorId ); // from: UINT
            AdapterInfo.apiVersion =
                static_cast< uint32_t >( Helper::GetMaxFeatureLevel( pAdapter ) ); // from: D3D_FEATURE_LEVEL

            AdapterInfo.hDDIAdapter = reinterpret_cast< handle_t >( pAdapter );

            LARGE_INTEGER DriverVersion = {};
            if( SUCCEEDED( pAdapter->CheckInterfaceSupport( __uuidof( IDXGIDevice ), &DriverVersion ) ) )
            {
                // Intel UHD eg: 30.0.101.1273
                // Nvidia eg: 31.0.15.3742
                WORD VersionMajor = DriverVersion.QuadPart >> 48;
                WORD VersionMinor = ( DriverVersion.QuadPart >> 32 ) & 0xFFFF;
                WORD VersionPatch = ( DriverVersion.QuadPart >> 16 ) & 0xFFFF;
                WORD VersionBuild = DriverVersion.QuadPart & 0xFFFF;

                char Buffer[ 128 ];
                sprintf_s( &Buffer[ 0 ], 128, "%u.%u.%u.%u", VersionMajor, VersionMinor, VersionPatch, VersionBuild );
                VKE_LOG( Buffer );

                AdapterInfo.driverVersion = ( VersionMajor << 16 ) | VersionPatch;
            }

            if( (UINT)AdapterDesc.Flags & (UINT)DXGI_ADAPTER_FLAG_SOFTWARE )
            {
                AdapterInfo.type = VKE::RenderSystem::ADAPTER_TYPE::VIRTUAL;
            }
            else if( AdapterDesc.DedicatedVideoMemory == 0 )
            {
                AdapterInfo.type = VKE::RenderSystem::ADAPTER_TYPE::INTEGRATED;
            }
            else
            {
                AdapterInfo.type = VKE::RenderSystem::ADAPTER_TYPE::DISCRETE;
            }

            // Discrepancy between Vulkan and DX12 - Vk reports char[] while DX12 wchar[] in unicode.
            // To store info.name, conversion is needed.
            size_t MaxSize = std::min( _countof( AdapterInfo.name ), _countof( AdapterDesc.Description ) );
            size_t InfoSize;
            wcstombs_s( &InfoSize, AdapterInfo.name, AdapterDesc.Description, MaxSize );

            pOut->PushBack( AdapterInfo );
        }

        pOut->Resize( Index - 1 );

        return Result::OK;
    }

    Result CDDI::Load( const SDDILoadInfo& Info, SDriverInfo* pOut )
    {
        if( Info.enableDebugMode )
        {
            ID3D12Debug* pDebug;
            if( FAILED( D3D12GetDebugInterface( IID_PPV_ARGS( &pDebug ) ) ) )
            {
                VKE_LOG_ERR( "CDDI::Load: Error while getting debug interface." );
            }
            else
            {
                pDebug->EnableDebugLayer();
                NativeAPI::SImplementation::sDebugLayerEnabled = true;
            }
        }

        // DX12 doesn't actually have a driver to load, just create the DXGI Factory via DXGI.
        Result Res = VKE_OK;

        UINT Flags = Info.enableDebugMode ? DXGI_CREATE_FACTORY_DEBUG : 0;

        if( FAILED( CreateDXGIFactory2( Flags, IID_PPV_ARGS( &NativeAPI::SImplementation::spFactory ) ) ) )
        {
            VKE_LOG_ERR( "CDDI::Load: Failed to create DXGI Factory" );
            return VKE_FAIL;
        }

        BOOL AllowTearing = FALSE;
        if( FAILED( NativeAPI::SImplementation::spFactory->CheckFeatureSupport(
                DXGI_FEATURE_PRESENT_ALLOW_TEARING, &AllowTearing, sizeof( AllowTearing ) ) ) )
        {
            VKE_LOG_ERR( "CDDI::QueryAdapters: Check tearing support failed" );
            return VKE_FAIL;
        }

        NativeAPI::SImplementation::SDeviceFeatures::sTearingSupported = ( AllowTearing == TRUE );

        return Res;
    }

    // Object methods

    void BitTest()
    {
        Utils::TCBitPool< uint8_t > Pool;
        Pool.Create( 100 );
        Utils::TCDynamicArray< ExtentU32 > vAllocated;

        for( uint32_t i = 0; i < 20; ++i )
        {
            uint32_t s         = rand() % 15;
            uint32_t firstSlot = Pool.AllocateSlots( s );
            Pool.Print( std::format( "+({},{})", firstSlot, s ) );

            bool doFree = vAllocated.GetCount() > 4 && ( ( rand() % 12 ) % 3 == 0 );

            if( doFree )
            {
                s       = rand() % vAllocated.GetCount();
                auto al = vAllocated[ s ];
                Pool.FreeSlots( al.begin, al.end );
                vAllocated.RemoveFast( s );
                Pool.Print( std::format( "-({},{})", al.begin, al.end ) );
            }
            vAllocated.PushBack( { firstSlot, s } );
        }
    }

    Result CDDI::CreateDevice( const SCreateDeviceDesc& Info, CDeviceContext* pCtx )
    {

        m_hAdapter = reinterpret_cast< NativeAPI::Adapter >( pCtx->m_Desc.pAdapterInfo->hDDIAdapter );
        m_pCtx     = pCtx;

        VKE_ASSERT2( m_hAdapter != NativeAPI::Null, "CDDI::CreateDevice: Adapter is null" );
        VKE_RETURN_IF_FAILED( Helper::QueryAdapterProperties( m_hAdapter, &m_DeviceProperties ) );

        // TODO(blturkot): Compare pCtx->m_Desc.pAdapterInfo->apiVersion with Info.Settings.Features

        if( NativeAPI::SImplementation::sDebugLayerEnabled )
        {
            // Enable TDR extended info (DRED)
            ID3D12DeviceRemovedExtendedDataSettings1* pDREDSettings = nullptr;
            if( SUCCEEDED( D3D12GetDebugInterface( IID_PPV_ARGS( &pDREDSettings ) ) ) )
            {
                pDREDSettings->SetAutoBreadcrumbsEnablement( D3D12_DRED_ENABLEMENT_FORCED_ON );
                pDREDSettings->SetPageFaultEnablement( D3D12_DRED_ENABLEMENT_FORCED_ON );
                pDREDSettings->SetBreadcrumbContextEnablement( D3D12_DRED_ENABLEMENT_FORCED_ON );

                pDREDSettings->Release();
                VKE_LOG( "CDDI::CreateDevice: DRED enabled with auto-breadcrumbs and page fault reporting" );
            }
        }

        HRESULT Result = D3D12CreateDevice( m_hAdapter,
                                            static_cast< D3D_FEATURE_LEVEL >( pCtx->m_Desc.pAdapterInfo->apiVersion ),
                                            IID_PPV_ARGS( &m_hDevice ) );
        if( FAILED( Result ) )
        {
            VKE_LOG_ERR( "CDDI::CreateDevice: D3D12CreateDevice failed" );
            return VKE_FAIL;
        }

        if( NativeAPI::SImplementation::sDebugLayerEnabled )
        {
            ID3D12InfoQueue* pInfoQueue = NativeAPI::Null;
            if( FAILED( m_hDevice->QueryInterface( &pInfoQueue ) ) )
            {
                VKE_LOG_ERR( "CDDI::CreateDevice: QueryInterface for ID3D12InfoQueue failed" );
            }
            else
            {
                pInfoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE );
                pInfoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_ERROR, TRUE );
                pInfoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_WARNING, TRUE );
                pInfoQueue->Release();
            }
        }

        CD3DX12FeatureSupport FeatureSupport;
        FeatureSupport.Init( m_hDevice );

        auto& Features = m_Implementation.Features;

        Features.TightAlignmentSupported =
            ( FeatureSupport.TightAlignmentSupportTier() != D3D12_TIGHT_ALIGNMENT_TIER_NOT_SUPPORTED );

        Features.BindlessResourceAccessSupported =
            ( FeatureSupport.ResourceBindingTier() >= D3D12_RESOURCE_BINDING_TIER_3 );

        Features.ResourceHeapTier = static_cast< uint8_t >( FeatureSupport.ResourceHeapTier() );

        Features.RayTracingSupported = ( FeatureSupport.RaytracingTier() != D3D12_RAYTRACING_TIER_NOT_SUPPORTED );
        Features.MeshShaderSupported = ( FeatureSupport.MeshShaderTier() != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED );

        Features.EnhancedBarriersSupported = FeatureSupport.EnhancedBarriersSupported();
        Features.UploadHeapSupported       = FeatureSupport.GPUUploadHeapSupported();

        for( UINT i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; i++ )
        {
            m_Implementation.Properties.Memory.DescriptorHeapSizes[ i ] =
                m_hDevice->GetDescriptorHandleIncrementSize( static_cast< D3D12_DESCRIPTOR_HEAP_TYPE >( i ) );
        }

        if( Info.Settings.Features.bindlessResourceAccess == FeatureEnableModes::ENABLE &&
            !m_Implementation.Features.BindlessResourceAccessSupported )
        {
            VKE_LOG_WARN( "CDDI::CreateDevice: Bindless Resource Access not fully supported on this device" );
        }

        if( Info.Settings.Features.meshShaders == FeatureEnableModes::ENABLE &&
            !m_Implementation.Features.MeshShaderSupported )
        {
            VKE_LOG_ERR( "CDDI::CreateDevice: Mesh Shaders not supported on this device" );
            return VKE_FAIL;
        }

        if( Info.Settings.Features.raytracing == FeatureEnableModes::ENABLE &&
            !m_Implementation.Features.RayTracingSupported )
        {
            VKE_LOG_ERR( "CDDI::CreateDevice: Raytracing not supported on this device" );
            return VKE_FAIL;
        }

        if( m_Implementation.Features.ResourceHeapTier < 2 )
        {
            VKE_LOG_WARN( "CDDI::CreateDevice: Hardware does not support Tier2 resource heaps." );
        }

        return Result::OK;
    }

    void CDDI::DestroyDevice()
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    NativeAPI::Queue CreateCommandQueue( NativeAPI::Device pDevice, D3D12_COMMAND_LIST_TYPE Type,
                                         bool Required = false )
    {
        D3D12_COMMAND_QUEUE_DESC Desc = {};
        Desc.Type                     = Type;
        Desc.Priority                 = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        Desc.Flags                    = D3D12_COMMAND_QUEUE_FLAG_NONE;
        Desc.NodeMask                 = Helper::GetNodeMask();

        NativeAPI::Queue pQueue = NativeAPI::Null;
        if( FAILED( pDevice->CreateCommandQueue( &Desc, IID_PPV_ARGS( &pQueue ) ) ) && Required )
        {
            VKE_LOG_ERR( "CDDI::CreateCommandQueue: Failed to create command queue" );
        }

        return pQueue;
    }

    void CDDI::QueryDeviceInfo( SDeviceInfo* pOut )
    {
        TRACK_CALL_ONCE( "CDDI::QueryDeviceInfo" );

        auto& Limits = pOut->Limits;

        auto& Alignment                = Limits.Alignment;
        Alignment.constantBufferOffset = 256;
        Alignment.bufferCopyOffset     = 0;
        Alignment.bufferCopyRowPitch   = 256;
        Alignment.memoryMap =
            0; // DX12 doesn't require alignments for memory mapping but resources needs to be aligned
               // appropriately, eg.:  64KB for default buffers/textures, 4KB for upload/readback buffers/textures.
        Alignment.texelBufferOffset   = 0;
        Alignment.storageBufferOffset = 0;

        auto& Binding                        = Limits.Binding;
        Binding.maxConstantBufferRange       = D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16; // 16 bytes per element
        Binding.maxPushConstantsSize         = 0; // DX12 doesn't have push constants
        Binding.Stage.maxConstantBufferCount = D3D12_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;
        Binding.Stage.maxSamplerCount        = D3D12_COMMONSHADER_SAMPLER_SLOT_COUNT;
        Binding.Stage.maxStorageBufferCount  = 0; // DX12 doesn't have storage buffers
        Binding.Stage.maxStorageTextureCount = 0; // DX12 doesn't have storage textures
        Binding.Stage.maxResourceCount       = D3D12_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;
        Binding.Stage.maxTextureCount        = D3D12_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;

        auto& Memory                            = Limits.Memory;
        Memory.maxAllocationCount               = UINT32_MAX; // No documented limit
        Memory.minMapAlignment                  = 0;          // No documented limit
        Memory.minTexelBufferOffsetAlignment    = 0;          // No documented limit
        Memory.minConstantBufferOffsetAlignment = 256;        // Must be 256-byte aligned
        Memory.minStorageBufferOffsetAlignment  = 0;          // No documented limit

        auto& l1Budget = m_Implementation.Properties.Memory.localBudget;
        auto& l0Budget = m_Implementation.Properties.Memory.hostBudget;

        // Right now just assume budgets for heaps for each level.
        auto& Heaps                            = m_Implementation.Properties.Memory.HeapProperties;
        Heaps[ MemoryHeapTypes::CPU ]          = { D3D12_HEAP_TYPE_DEFAULT, D3D12_MEMORY_POOL_L0, l0Budget };
        Heaps[ MemoryHeapTypes::GPU ]          = { D3D12_HEAP_TYPE_DEFAULT, D3D12_MEMORY_POOL_L1, l1Budget };
        Heaps[ MemoryHeapTypes::UPLOAD ]       = { D3D12_HEAP_TYPE_UPLOAD, D3D12_MEMORY_POOL_UNKNOWN, l1Budget };
        Heaps[ MemoryHeapTypes::CPU_CACHED ]   = { D3D12_HEAP_TYPE_READBACK, D3D12_MEMORY_POOL_L0, l0Budget };
        Heaps[ MemoryHeapTypes::CPU_COHERENT ] = { D3D12_HEAP_TYPE_READBACK, D3D12_MEMORY_POOL_L0, l0Budget };
        Heaps[ MemoryHeapTypes::OTHER ]        = { D3D12_HEAP_TYPE_CUSTOM, D3D12_MEMORY_POOL_UNKNOWN, l1Budget };

        auto& RenderPass                     = Limits.RenderPass;
        RenderPass.maxColorRenderTargetCount = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;

        auto& Query           = Limits.Query;
        Query.timestampPeriod = 0.0; // This should in nanoseconds to match other API.
                                     // DX12 needs to query the timestamp frequency from the command queue.

        VKE_ASSERT2( m_hDevice != NativeAPI::Null, "Device cannot be Null when calling QueryDeviceIndo" );

        // TODO(any): Think about queue priorities and multiple queues per family.
        for( int i = 0; i < QUEUE_TYPE::_MAX_COUNT; i++ )
        {
            NativeAPI::Queue pQueue = NativeAPI::Null;

            SQueueFamilyInfo QueueInfo = {};

            QueueInfo.index = i;
            QueueInfo.type  = static_cast< QUEUE_TYPE >( i );

            D3D12_COMMAND_LIST_TYPE Type = Map::GetCommandListType( QueueInfo.type );

            if( Type != D3D12_COMMAND_LIST_TYPE_NONE )
            {
                pQueue = CreateCommandQueue( m_hDevice, Type, Type == D3D12_COMMAND_LIST_TYPE_DIRECT );
            }

            if( pQueue != NativeAPI::Null )
            {
                QueueInfo.vQueues.PushBack( pQueue );
                QueueInfo.vPriorities.PushBack( 1.0f );
            }
            else
            {
                QueueInfo.vQueues.Resize( 0 );
                QueueInfo.vPriorities.Resize( 0 );
            }

            m_DeviceProperties.vQueueFamilies.PushBack( QueueInfo );
        }
    }

    NativeAPI::D3D12Resource* CreateResource( NativeAPI::Device                   pDevice,
                                              const NativeAPI::D3D12ResourceDesc& ResourceDesc,
                                              const SBindMemoryInfo&              MemInfo )
    {
        NativeAPI::D3D12Resource* pResource = NativeAPI::Null;

        if( FAILED( pDevice->CreatePlacedResource( MemInfo.hDDIMemory,
                                                   MemInfo.offset,
                                                   &ResourceDesc,
                                                   D3D12_RESOURCE_STATE_COMMON,
                                                   nullptr,
                                                   IID_PPV_ARGS( &pResource ) ) ) )
        {
            VKE_LOG_ERR( "CDDI::CreateResource: Create resource failure." );
        }
        else
        {
            if( ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER )
            {
                VKE_LOG( std::format( "CDDI::CreateResource: Placed resource created at GPU VA: {}",
                                      pResource->GetGPUVirtualAddress() ) );
            }
            else
            {
                VKE_LOG(
                    std::format( "CDDI::CreateResource: Placed resource created at heap offset: {}", MemInfo.offset ) );
            }
        }

        return pResource;
    }

    NativeAPI::Buffer CDDI::CreateBuffer( const SBufferDesc& Desc, const SBindMemoryInfo& MemInfo )
    {
        VKE_ASSERT2( m_hDevice != NativeAPI::Null, "CDDI::CreateBuffer: m_hDevice can't be null" );

        NativeAPI::D3D12ResourceDesc ResourceDesc = Convert::GetResourceDesc( Desc, m_Implementation.Features );
        NativeAPI::Buffer            pBuffer      = CreateResource( m_hDevice, ResourceDesc, MemInfo );

        if( pBuffer != NativeAPI::Null )
        {
            SetObjectDebugName( (const uint64_t)pBuffer, ApiObjectTypes::BUFFER, "Unnamed buffer" );
        }

        return pBuffer;
    }

    void CDDI::DestroyBuffer( NativeAPI::Buffer* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    NativeAPI::BufferView CDDI::CreateBufferView( const SBufferViewDesc& Desc, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return NativeAPI::Null;
    }

    void CDDI::DestroyBufferView( NativeAPI::BufferView* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    Result CDDI::GetTextureFormatProperties( const STextureDesc& Desc, STextureFormatProperties* pOut )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return Result::OK;
    }

    NativeAPI::Texture CDDI::CreateTexture( const STextureDesc& Desc, const SBindMemoryInfo& MemInfo )
    {
        VKE_ASSERT2( m_hDevice != NativeAPI::Null, "CDDI::CreateTexture: m_hDevice can't be null" );

        NativeAPI::D3D12ResourceDesc ResourceDesc;
        Convert::GetResourceDesc( Desc, m_Implementation.Features, &ResourceDesc );
        NativeAPI::Texture pTexture = CreateResource( m_hDevice, ResourceDesc, MemInfo );

        if( pTexture != NativeAPI::Null )
        {
            SetObjectDebugName( (const uint64_t)pTexture, ApiObjectTypes::TEXTURE, Desc.Name.GetData() );
        }

        return pTexture;
    }

    void CDDI::DestroyTexture( NativeAPI::Texture* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    namespace Map
    {
        D3D12_UAV_DIMENSION DimmensionToUAVDimmension( D3D12_RESOURCE_DIMENSION dim )
        {
            const D3D12_UAV_DIMENSION cValues[] = {
                D3D12_UAV_DIMENSION_UNKNOWN,   // unknown
                D3D12_UAV_DIMENSION_BUFFER,    // buffer
                D3D12_UAV_DIMENSION_TEXTURE1D, // texture 1d
                D3D12_UAV_DIMENSION_TEXTURE2D, // texture 2d
                D3D12_UAV_DIMENSION_TEXTURE3D, // texture 3d
            };
            return cValues[ dim ];
        }

        D3D12_UAV_DIMENSION DimmensionToUAVDimmensionArray( D3D12_RESOURCE_DIMENSION dim )
        {
            const D3D12_UAV_DIMENSION cValues[] = {
                D3D12_UAV_DIMENSION_UNKNOWN,        // unknown
                D3D12_UAV_DIMENSION_BUFFER,         // buffer
                D3D12_UAV_DIMENSION_TEXTURE1DARRAY, // texture 1d
                D3D12_UAV_DIMENSION_TEXTURE2DARRAY, // texture 2d
                D3D12_UAV_DIMENSION_TEXTURE3D,      // texture 3d
            };
            return cValues[ dim ];
        }

        D3D12_SRV_DIMENSION DimmensionToSRVDimmension( D3D12_RESOURCE_DIMENSION dim )
        {
            const D3D12_SRV_DIMENSION cValues[] = {
                D3D12_SRV_DIMENSION_UNKNOWN,   // unknown
                D3D12_SRV_DIMENSION_BUFFER,    // buffer
                D3D12_SRV_DIMENSION_TEXTURE1D, // texture 1d
                D3D12_SRV_DIMENSION_TEXTURE2D, // texture 2d
                D3D12_SRV_DIMENSION_TEXTURE3D, // texture 3d
            };
            return cValues[ dim ];
        }

        D3D12_SRV_DIMENSION DimmensionToSRVDimmensionArray( D3D12_RESOURCE_DIMENSION dim )
        {
            const D3D12_SRV_DIMENSION cValues[] = {
                D3D12_SRV_DIMENSION_UNKNOWN,        // unknown
                D3D12_SRV_DIMENSION_BUFFER,         // buffer
                D3D12_SRV_DIMENSION_TEXTURE1DARRAY, // texture 1d
                D3D12_SRV_DIMENSION_TEXTURE2DARRAY, // texture 2d
                D3D12_SRV_DIMENSION_TEXTURE3D,      // texture 3d
            };
            return cValues[ dim ];
        }

        D3D12_DESCRIPTOR_HEAP_TYPE DescriptorPoolTypeToDescriptorHeapType( RenderSystem::DESCRIPTOR_POOL_TYPE type )
        {
            /*
            struct DescriptorPoolTypes
            {
                enum TYPE
                {
                    TEXTURE_BUFFER_CBUFFER,
                    SAMPLER,
                    RENDER_TARGET,
                    DEPTH_STENCIL,
                    _MAX_COUNT
                };
            };
            */
            static const D3D12_DESCRIPTOR_HEAP_TYPE scValues[ D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES ] = {
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
                D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
                D3D12_DESCRIPTOR_HEAP_TYPE_DSV
            };
            return scValues[ type ];
        }

        D3D12_DESCRIPTOR_HEAP_TYPE DescriptorBindingTypeToHeapType( RenderSystem::BINDING_TYPE type )
        {
            /*struct BindingTypes
            {
                enum TYPE : uint8_t
                {
                    SAMPLER,             // only sampler
                    TEXTURE,             // only texture without sampler
                    STORAGE_TEXTURE,
                    READ_ONLY_TEXEL_BUFFER,
                    READ_WRITE_TEXEL_BUFFER,
                    CONSTANT_BUFFER,
                    BUFFER,
                    DYNAMIC_CONSTANT_BUFFER,
                    DYNAMIC_BUFFER,
                    RENDER_TARGET,
                    DEPTH_STENCIL,
                    _MAX_COUNT,
                    UNKNOWN = _MAX_COUNT
                };
            };*/
            static const D3D12_DESCRIPTOR_HEAP_TYPE ascValues[ RenderSystem::BindingTypes::_MAX_COUNT ] = {
                D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,     // sampler
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, // texture
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, // storage tex
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, // ro tex buff
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, // rw tex buff
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, // cbuff
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, // buffer
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, // dyn cbuffer
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, // dyn buff
                D3D12_DESCRIPTOR_HEAP_TYPE_RTV,         // render target
                D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES    // depth stencil
            };
            return ascValues[ type ];
        }
    } // namespace Map

    NativeAPI::TextureView CDDI::CreateTextureView( const STextureViewDesc& TextureViewDesc, const void* pAllocator )
    {
        VKE_ASSERT2( m_hDevice != NativeAPI::Null, "m_hDevice can't be null" );

        NativeAPI::TextureView pTextureView = NativeAPI::Null;
        if( VKE_FAILED( Memory::CreateObject( &HeapAllocator, &pTextureView ) ) )
        {
            VKE_LOG_ERR( "Out of memory" );
            return pTextureView;
        }

        pTextureView->pResource                   = m_pCtx->GetTexture( TextureViewDesc.hTexture )->GetDDIObject();
        NativeAPI::D3D12ResourceDesc ResourceDesc = pTextureView->pResource->GetDesc();

        if( ( ResourceDesc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE ) == 0 )
        {
            Helper::CreateShaderResourceView( TextureViewDesc, &pTextureView->ShaderResourceViewDesc );
            pTextureView->Enable( NativeAPI::ResourceViewTypes::SRV );
        }

        if( ( ResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET ) )
        {
            Helper::CreateRenderTargetView( TextureViewDesc, &pTextureView->RenderTargetViewDesc );
            pTextureView->Enable( NativeAPI::ResourceViewTypes::RTV );
        }

        if( ( ResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS ) )
        {
            Helper::CreateUnorderedAccessView( TextureViewDesc, &pTextureView->UnorderedAccessViewDesc );
            pTextureView->Enable( NativeAPI::ResourceViewTypes::UAV );
        }

        if( ( ResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL ) )
        {
            Helper::CreateDepthStencilViewDesc( TextureViewDesc, &pTextureView->DepthStencilViewDesc );
            pTextureView->Enable( NativeAPI::ResourceViewTypes::DSV );
        }

        return pTextureView;
    }

    void CDDI::DestroyTextureView( NativeAPI::TextureView* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    NativeAPI::Framebuffer CDDI::CreateFramebuffer( const SFramebufferDesc& Desc, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return NativeAPI::Null;
    }

    void CDDI::DestroyFramebuffer( NativeAPI::Framebuffer* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    NativeAPI::CPUFence CDDI::CreateFence( const SFenceDesc& Desc, const void* pAllocator ) const
    {
        VKE_ASSERT2( m_hDevice != NativeAPI::Null, "m_hDevice can't be null" );

        NativeAPI::CPUFence pFence = NativeAPI::Null;
        Memory::CreateObject( &HeapAllocator, &pFence );

        D3D12_FENCE_FLAGS Flags = D3D12_FENCE_FLAG_NONE;

        if( FAILED( m_hDevice->CreateFence( Desc.startValue, Flags, IID_PPV_ARGS( &pFence->pObject ) ) ) )
        {
            VKE_LOG_ERR( "CDDI::CreateFence: Failed to create fence" );
        }

        pFence->Value = Desc.startValue;

        return pFence;
    }

    NativeAPI::Fence CDDI::CreateFence2( const SFenceDesc& Desc ) const
    {
        NativeAPI::Fence pFence = NativeAPI::Null;
        Memory::CreateObject( &HeapAllocator, &pFence );

        D3D12_FENCE_FLAGS Flags = D3D12_FENCE_FLAG_NONE;

        if( FAILED( m_hDevice->CreateFence( Desc.startValue, Flags, IID_PPV_ARGS( &pFence->pObject ) ) ) )
        {
            VKE_LOG_ERR( "CDDI::CreateFence: Failed to create fence" );
        }

        pFence->Value = Desc.startValue;

        return pFence;
    }

    void CDDI::DestroyFence( NativeAPI::CPUFence* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::DestroyFence( NativeAPI::Fence* pInOut )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    NativeAPI::GPUFence CDDI::CreateSemaphore( const SSemaphoreDesc& Desc, const void* pAllocator ) const
    {
        VKE_ASSERT2( m_hDevice != NativeAPI::Null, "m_hDevice can't be null" );

        NativeAPI::GPUFence pFence;
        Memory::CreateObject( &HeapAllocator, &pFence );

        D3D12_FENCE_FLAGS Flags = D3D12_FENCE_FLAG_NONE;

        if( FAILED( m_hDevice->CreateFence( Desc.startValue, Flags, IID_PPV_ARGS( &pFence->pObject ) ) ) )
        {
            VKE_LOG_ERR( "CDDI::CreateFence: Failed to create fence" );
        }

        pFence->Value = Desc.startValue;
        return pFence;
    }

    void CDDI::DestroySemaphore( NativeAPI::GPUFence* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    NativeAPI::RenderPass CDDI::CreateRenderPass( const SRenderPassDesc& EngineRenderPassDesc, const void* pAllocator )
    {
        NativeAPI::RenderPass pNativeRenderPass = NativeAPI::Null;
        if( VKE_FAILED( Memory::CreateObject( &HeapAllocator, &pNativeRenderPass ) ) )
        {
            VKE_LOG_ERR(
                "CDDI::CreateRenderPass: Unable to allocate memory for RenderPass: " << EngineRenderPassDesc.Name );
            return NativeAPI::Null;
        }

        bool createDefaultSubpass = EngineRenderPassDesc.vSubpasses.IsEmpty();

        NativeAPI::CustomTypes::SRenderPass::SSubpass* pDefaultSubpass = NativeAPI::Null;
        if( createDefaultSubpass && VKE_FAILED( Memory::CreateObject( &HeapAllocator, &pDefaultSubpass ) ) )
        {
            VKE_LOG_ERR( "CDDI::CreateRenderPass: Unable to allocate memory for subpass" );
            return NativeAPI::Null;
        }

        // TODO(szymansk): Currently we're having global descriptor heaps in SImplementation class. Right now they are
        // not under control of the engine. The ideal situation would be to:
        // 1. Have a collection of render targets in engine
        // 2. Engine calls something like: NativeAPI::RenderTarget CDDI::CreateRenderTarget( RenderTargetPool ),
        // CDDI
        // returns it's own handle:
        // - DX12: D3D12_CPU_DESCRIPTOR_HANDLE
        // - Vulkan: Texture pointer?
        // 3. SRenderPassDesc will include RendeTarget

        auto pDescriptorHeapRTV = m_Implementation.GetDescriptorHeap(
            m_hDevice, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE );

        auto pDescriptorHeapDSV = m_Implementation.GetDescriptorHeap(
            m_hDevice, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE );

        // Desc.vRenderTargetDescs seems to be never used by engine at the time, so CDDI skips it for now.
        // TODO(szymansk): Remove it from engine?
        // ---
        // Reserve slots in descriptor heap and get offset for the first descriptor to be used in render pass for
        // later binding.
        D3D12_CPU_DESCRIPTOR_HANDLE hNativeCPUDescriptor =
            pDescriptorHeapRTV->Allocate( EngineRenderPassDesc.vRenderTargets.GetCount() );

        for( uint32_t index = 0; index < EngineRenderPassDesc.vRenderTargets.GetCount(); index++ )
        {
            const auto& EngineRenderTarget = EngineRenderPassDesc.vRenderTargets[ index ];
            const auto& NativeResourceView = *EngineRenderTarget.hNativeView;

            D3D12_RESOURCE_STATES NativeTrackedResourceState = Map::GetResourceState( EngineRenderTarget.beginState );

            bool isRenderTargetView = false;
            bool isDepthStencilView = false;
            bool isClearOp          = false;
            bool isStoreOp          = false;

            switch( EngineRenderTarget.usage )
            {
                case RENDER_TARGET_RENDER_PASS_OP::UNDEFINED:
                    // No access?
                    break;

                case RENDER_TARGET_RENDER_PASS_OP::COLOR:
                    isRenderTargetView = true;
                    break;

                case RENDER_TARGET_RENDER_PASS_OP::COLOR_CLEAR:
                    isRenderTargetView = true;
                    isClearOp          = true;
                    break;

                case RENDER_TARGET_RENDER_PASS_OP::COLOR_STORE:
                    isRenderTargetView = true;
                    isStoreOp          = true;
                    break;

                case RENDER_TARGET_RENDER_PASS_OP::COLOR_CLEAR_STORE:
                    isRenderTargetView = true;
                    isClearOp          = true;
                    isStoreOp          = true;
                    break;

                case RENDER_TARGET_RENDER_PASS_OP::DEPTH_STENCIL:
                    isDepthStencilView = true;
                    break;

                case RENDER_TARGET_RENDER_PASS_OP::DEPTH_STENCIL_CLEAR:
                    isDepthStencilView = true;
                    isClearOp          = true;
                    break;

                case RENDER_TARGET_RENDER_PASS_OP::DEPTH_STENCIL_STORE:
                    isDepthStencilView = true;
                    isStoreOp          = true;
                    break;

                case RENDER_TARGET_RENDER_PASS_OP::DEPTH_STENCIL_CLEAR_STORE:
                    isDepthStencilView = true;
                    isClearOp          = true;
                    isStoreOp          = true;
                    break;

                default:
                    VKE_LOG_ERR( "CDDI::CreateRenderPass: Unhandled usage" );
                    return NativeAPI::Null;
            }

            if( isRenderTargetView )
            {
                if( !NativeResourceView.IsEnabled( NativeAPI::ResourceViewTypes::RTV ) )
                {
                    VKE_LOG_ERR( "CDDI::CreateRenderPass: COLOR render targets must have resource valid for "
                                 "RENDER_TARGET_VIEW" );
                    return NativeAPI::Null;
                }

                m_hDevice->CreateRenderTargetView(
                    NativeResourceView.pResource, &NativeResourceView.RenderTargetViewDesc, hNativeCPUDescriptor );
                hNativeCPUDescriptor.ptr += pDescriptorHeapRTV->DescriptorSize;

                auto& NativeRenderTargetView = pNativeRenderPass->RenderTargetViews.Reserve();
                NativeRenderTargetView.ptr   = hNativeCPUDescriptor.ptr;

                if( isClearOp )
                {
                    auto& ClearArgs = pNativeRenderPass->Clear.Reserve();
                    ClearArgs.Type  = NativeAPI::CustomTypes::SRenderPass::SClearArgs::RENDER_TARGET;

                    auto& ClearArgsRTV             = ClearArgs.RenderTargetView;
                    ClearArgsRTV.hRenderTargetView = hNativeCPUDescriptor;

                    Memory::Copy( &ClearArgsRTV.aColorRGBA,
                                  sizeof( ClearArgsRTV.aColorRGBA ),
                                  EngineRenderTarget.ClearValue.Color.floats,
                                  sizeof( EngineRenderTarget.ClearValue.Color.floats ) );

                    ClearArgsRTV.Rect.left   = EngineRenderPassDesc.PositionOffset.x;
                    ClearArgsRTV.Rect.top    = EngineRenderPassDesc.PositionOffset.y;
                    ClearArgsRTV.Rect.right  = EngineRenderPassDesc.Size.x;
                    ClearArgsRTV.Rect.bottom = EngineRenderPassDesc.Size.y;

                    // ClearRenderTargetView() requires resource state D3D12_RESOURCE_STATE_RENDER_TARGET.
                    Helper::ExpectResourceState( NativeResourceView.pResource,
                                                 D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                 NativeTrackedResourceState,
                                                 &pNativeRenderPass->Clear.Barriers );

                    // After clear resource state is left in D3D12_RESOURCE_STATE_RENDER_TARGET.
                    NativeTrackedResourceState = D3D12_RESOURCE_STATE_RENDER_TARGET;
                }

                if( createDefaultSubpass )
                {
                    auto& NativeDefaultSubpassRenderTarget          = pDefaultSubpass->RenderTargetViews.Reserve();
                    NativeDefaultSubpassRenderTarget.hCPUDescriptor = hNativeCPUDescriptor;
                    NativeDefaultSubpassRenderTarget.resourceState  = NativeTrackedResourceState;
                }

                // Barriers for BeginRenderPass() after Clear() is done.
                Helper::ExpectResourceState( NativeResourceView.pResource,
                                             Map::GetResourceState( EngineRenderTarget.beginState ),
                                             NativeTrackedResourceState,
                                             &pDefaultSubpass->BeginBarriers );

                // TODO(blturkot): Handle STORE/DEFAULT usage
            }
            else if( isDepthStencilView )
            {
                if( !NativeResourceView.IsEnabled( NativeAPI::ResourceViewTypes::DSV ) )
                {
                    VKE_LOG_ERR( "CDDI::CreateRenderPass: DEPTH_STENCIL render targets must have resource valid "
                                 "for DEPTH_STENCIL_VIEW" );
                    return NativeAPI::Null;
                }

                pNativeRenderPass->DepthStencilView.hCPUDescriptor = pDescriptorHeapDSV->Allocate( 1 );
                m_hDevice->CreateDepthStencilView( NativeResourceView.pResource,
                                                   &NativeResourceView.DepthStencilViewDesc,
                                                   pNativeRenderPass->DepthStencilView.hCPUDescriptor );

                if( isClearOp )
                {
                    auto& ClearArgs = pNativeRenderPass->Clear.Reserve();
                    ClearArgs.Type  = NativeAPI::CustomTypes::SRenderPass::SClearArgs::DEPTH_STENCIL_VIEW;

                    auto& ClearArgsDSV             = ClearArgs.DepthStencilView;
                    ClearArgsDSV.hDepthStencilView = pNativeRenderPass->DepthStencilView.hCPUDescriptor;
                    ClearArgsDSV.ClearFlags = Convert::GetClearDepthStencilViewFlags( EngineRenderTarget.format );
                    ClearArgsDSV.depth      = EngineRenderTarget.ClearValue.DepthStencil.depth;
                    ClearArgsDSV.stencil = static_cast< UINT8 >( EngineRenderTarget.ClearValue.DepthStencil.stencil );

                    ClearArgsDSV.Rect.left   = EngineRenderPassDesc.PositionOffset.x;
                    ClearArgsDSV.Rect.top    = EngineRenderPassDesc.PositionOffset.y;
                    ClearArgsDSV.Rect.right  = EngineRenderPassDesc.Size.x;
                    ClearArgsDSV.Rect.bottom = EngineRenderPassDesc.Size.y;
                }

                // ClearDepthStencilView() requires resource state D3D12_RESOURCE_STATE_DEPTH_WRITE.
                Helper::ExpectResourceState( NativeResourceView.pResource,
                                             D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                             NativeTrackedResourceState,
                                             &pNativeRenderPass->Clear.Barriers );

                // After clear resource state is left in D3D12_RESOURCE_STATE_RENDER_TARGET.
                NativeTrackedResourceState = D3D12_RESOURCE_STATE_RENDER_TARGET;
                auto EngineRequestedState  = Map::GetResourceState( EngineRenderTarget.beginState );

                // Barriers for BeginRenderPass() after Clear() is done.
                Helper::ExpectResourceState( NativeResourceView.pResource,
                                             EngineRequestedState,
                                             NativeTrackedResourceState,
                                             &pDefaultSubpass->BeginBarriers );
                NativeTrackedResourceState = EngineRequestedState;

                // TODO(blturkot): Decde if STORE/DEFAULT usage
            }
            else
            {
                VKE_LOG_ERR( "CDDI::CreateRenderPass: Render target must have RTV or DSV set." );
                return NativeAPI::Null;
            }

            // Append baked barriers on EndRenderPass(). This may be valid for RenderPasses with default subpass.
            // TODO(blturkot): EndRenderPass() should track resource state from last used subpass.
            Helper::ExpectResourceState( NativeResourceView.pResource,
                                         Map::GetResourceState( EngineRenderTarget.endState ),
                                         NativeTrackedResourceState,
                                         &pNativeRenderPass->EndBarriers );
        }

        for( const auto& Subpass: EngineRenderPassDesc.vSubpasses )
        {
            VKE_LOG( "Subpass" << Subpass.GetDebugName() );
            UNIMPLEMENTED_D3D12_METHOD();
        }

        pNativeRenderPass->pName = EngineRenderPassDesc.GetDebugName();
        return pNativeRenderPass;
    }

    void CDDI::DestroyRenderPass( NativeAPI::RenderPass* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    NativeAPI::CommandBufferPool CDDI::CreateCommandBufferPool( const SCommandBufferPoolDesc& Desc,
                                                                const void*                   pAllocator )
    {
        VKE_ASSERT2( m_hDevice != NativeAPI::Null, "CDDI::CreateCommandBufferPool: m_hDevice can't be null" );

        NativeAPI::CommandBufferPool pCommandBufferPool = NativeAPI::Null;
        Memory::CreateObject( &HeapAllocator, &pCommandBufferPool );

        D3D12_COMMAND_LIST_TYPE type = Map::GetCommandListType( Desc.pContext->m_pQueue->GetType() );
        if( type == D3D12_COMMAND_LIST_TYPE_NONE )
        {
            VKE_LOG_ERR( "CDDI::CreateCommandBufferPool: Unsupported command list type" );
            return NativeAPI::Null;
        }

        // if( FAILED( m_hDevice->CreateCommandAllocator( type, IID_PPV_ARGS( &pCommandAllocator->pAllocator ) ) ) )
        //{
        //     VKE_LOG_ERR( "CDDI::CreateCommandBufferPool: Failed to create command allocator" );
        // }

        pCommandBufferPool->EngineType = Desc.pContext->m_pQueue->GetType();
        pCommandBufferPool->NativeType = type;

        return pCommandBufferPool;
    }

    void CDDI::DestroyCommandBufferPool( NativeAPI::CommandBufferPool* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    NativeAPI::DescriptorPool CDDI::CreateDescriptorPool( const SDescriptorPoolDesc& EngineDesc,
                                                          const void*                pAllocator )
    {
        if( EngineDesc.IsValid() == false )
        {
            VKE_LOG_ERR(
                "Invalid DescriptorPool desc. Descriptor pool must have only one type (DESCRIPTOR_POOL_TYPE). Make "
                "sure that textures/buffers, sampler, render targets, depth stencils are not mixed together." );

            return NativeAPI::Null;
        }

        VKE_ASSERT2( m_hDevice != NativeAPI::Null, "CDDI::CreateDescriptorPool: m_hDevice can't be null" );

        NativeAPI::DescriptorPool pPool = NativeAPI::Null;
        if( VKE_FAILED( Memory::CreateObject( &HeapAllocator, &pPool ) ) )
        {
            VKE_LOG_ERR( "CDDI::CreateDescriptorPool: Unable to allocate memory" );
            return NativeAPI::Null;
        }

        auto     poolType                 = BindingTypeToPoolType( EngineDesc.vPoolSizes[ 0 ].type );
        uint32_t totalDescriptorSlotCount = 0;

        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        // D3D12_DESCRIPTOR_HEAP_TYPE heapType = {};

        for( auto& poolSize: EngineDesc.vPoolSizes )
        {
            totalDescriptorSlotCount += poolSize.count;
        }

        if( VKE_FAILED(
                pPool->DescriptorSetMemMgr.Create( totalDescriptorSlotCount, sizeof( NativeAPI::DescriptorSet ), 1 ) ) )
        {
            Memory::DestroyObject( &HeapAllocator, &pPool );
            VKE_LOG_ERR( "Critical: Not enouth memory to create descriptor pool. Required number of descriptor slots: "
                         << totalDescriptorSlotCount );
            return pPool;
        }

        {
            auto nativePoolType   = Map::DescriptorPoolTypeToDescriptorHeapType( poolType );
            pPool->descriptorSize = m_hDevice->GetDescriptorHandleIncrementSize( nativePoolType );
            if( pPool->descriptorSize )
            {
                heapDesc.Type           = nativePoolType;
                heapDesc.NumDescriptors = totalDescriptorSlotCount;
                heapDesc.NodeMask       = Helper::GetNodeMask();
                heapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

                if( heapDesc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ||
                    heapDesc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER )
                {
                    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
                }

                if( VKE_FAILED( pPool->SlotMgr.Create( heapDesc.NumDescriptors ) ) )
                {
                    DestroyDescriptorPool( &pPool, pAllocator );
                    VKE_LOG_ERR( "Unable to create descriptor heap slot pool. Out of memory." );
                    return pPool;
                }

                if( FAILED( m_hDevice->CreateDescriptorHeap( &heapDesc, IID_PPV_ARGS( &pPool->pHeap ) ) ) )
                {
                    VKE_LOG_ERR( "CDDI::CreateDescriptorPool: Failed to create descriptor heap" );
                }

                pPool->type = nativePoolType;
            }
        }

        return pPool;
    }

    void CDDI::DestroyDescriptorPool( NativeAPI::DescriptorPool* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    NativeAPI::DescriptorSetLayout CDDI::CreateDescriptorSetLayout( const SDescriptorSetLayoutDesc& Desc,
                                                                    const void*                     pAllocator )
    {
        VKE_ASSERT2( m_hDevice != NativeAPI::Null, "CDDI::CreateDescriptorSetLayout: m_hDevice can't be null" );

        if( !Desc.IsValid() )
        {
            return NativeAPI::Null;
        }
        if( Desc.vBindings.IsEmpty() )
        {
            VKE_LOG_ERRF( "Unable to create DescriptorSetLayout: '{}' because number of resource bindings is 0.",
                          Desc.GetDebugName() );
            return NativeAPI::Null;
        }

        NativeAPI::DescriptorSetLayout pNativeDescriptorSetLayout = NativeAPI::Null;
        if( VKE_FAILED( Memory::CreateObject( &HeapAllocator, &pNativeDescriptorSetLayout ) ) )
        {
            VKE_LOG_ERR( "CDDI::CreateDescriptorSetLayout: Unable to allocate memory" );
        }

        NativeAPI::D3D12RootParameter& rootParameter = pNativeDescriptorSetLayout->RootParameter;
        pNativeDescriptorSetLayout->type             = Map::DescriptorBindingTypeToHeapType( Desc.vBindings[ 0 ].type );
        pNativeDescriptorSetLayout->numSlots         = 0;

        // Assume shader visibility all, see notes below.
        rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameter.ParameterType    = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

        for( auto& binding: Desc.vBindings )
        {
            // We can prevent shader visibility set to all by create each range for a specific visibility. For
            // example range with visibility PIXEL, VERTEX. But it still won't match Vulkan as theres no possibility
            // for combining like PIXEL | VERTEX. D3D12_SHADER_VISIBILITY visibility = Convert::getShaderVisibility(
            // binding.stages );
            NativeAPI::D3D12DescriptorRange range;

            range.RangeType          = Map::GetDescriptorRangeType( binding.type );
            range.NumDescriptors     = binding.count;
            range.BaseShaderRegister = binding.idx;
            range.RegisterSpace      = 0;

            // This flag allows us to update descriptors in descriptor table on command list execution.
            // Same as in vulkan.
            range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

            range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
            pNativeDescriptorSetLayout->vDescriptorRanges.PushBack( range );
            pNativeDescriptorSetLayout->numSlots += binding.count;
        }

        rootParameter.DescriptorTable.NumDescriptorRanges = pNativeDescriptorSetLayout->vDescriptorRanges.GetCount();
        rootParameter.DescriptorTable.pDescriptorRanges   = pNativeDescriptorSetLayout->vDescriptorRanges.GetData();

        return pNativeDescriptorSetLayout;
    }

    void CDDI::DestroyDescriptorSetLayout( NativeAPI::DescriptorSetLayout* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    VKE::Result CDDI::CreateDescriptorSets( const AllocateDescs::SDescSet& EngineDescriptorSetInfo,
                                            NativeAPI::DescriptorSet*      pOutNativeDescriptorSets )
    {
        VKE_ASSERT2( m_hDevice != NativeAPI::Null, "CDDI::CreateDescriptorSets: m_hDevice can't be null" );

        auto pNativePool = EngineDescriptorSetInfo.hPool;

        VKE::Result result = VKE_OK;

        for( uint32_t layoutIndex = 0; layoutIndex < EngineDescriptorSetInfo.count; layoutIndex++ )
        {
            pOutNativeDescriptorSets[ layoutIndex ] = nullptr;
            NativeAPI::DescriptorSet& pCurrentSet   = pOutNativeDescriptorSets[ layoutIndex ];

            if( VKE_FAILED( Memory::CreateObject( &pNativePool->DescriptorSetMemMgr, &pCurrentSet ) ) )
            {
                VKE_LOG_ERR( "CDDI::CreateDescriptorSets: Unable to allocate memory" );
                return VKE_FAIL;
            }

            const NativeAPI::DescriptorSetLayout pLayout = EngineDescriptorSetInfo.phLayouts[ layoutIndex ];
            pCurrentSet->pLayout                         = pLayout;

            // CreateDescriptorSet is supposed to only handle D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV.
            // We're ignoring RTV, DSV as they are handled by CreateRenderPass().
            // TODO(blturkot): Handle SAMPLER heaps if ever used.
            VKE_ASSERT( pLayout->numSlots > 0 );
            auto firstSlotIndex = EngineDescriptorSetInfo.hPool->SlotMgr.AllocateSlots( pLayout->numSlots );
            if( firstSlotIndex != UNDEFINED_U32 )
            {
                pCurrentSet->PoolSlots             = { firstSlotIndex, pLayout->numSlots };
                pCurrentSet->pPool                 = EngineDescriptorSetInfo.hPool;
                pCurrentSet->descTableCPUStartAddr = pCurrentSet->GetCpuDescriptorHandle( 0 ).ptr;
                pCurrentSet->descTableGPUStartAddr = pCurrentSet->GetGpuDescriptorHandle( 0 ).ptr;
            }
            else
            {
                result = VKE_FAIL;
                // Not an error since it is possible to create new descriptor pool
                VKE_LOG_WARN( "Not enough free slot ranges in descriptor heap pool of type: " << pLayout->type );
            }
        }
        if( VKE_FAILED( result ) )
        {
            FreeDescs::SDescSet Free;
            Free.hPool  = EngineDescriptorSetInfo.hPool;
            Free.phSets = pOutNativeDescriptorSets;
            Free.count  = EngineDescriptorSetInfo.count;
            FreeObjects( Free );
        }
        return result;
    }

    void CDDI::FreeObjects( const FreeDescs::SDescSet& )
    {
        // Must add freed ranges to heaps
        // The best option would be to not destroy memory directly but to place objects to some free list instead
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::Update( const SUpdateBufferDescriptorSetInfo& Info )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::Update( const SUpdateTextureDescriptorSetInfo& Info )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::Update( const NativeAPI::DescriptorSet& hDDISet, const SUpdateBindingsHelper& Info )
    {
        VKE_ASSERT2( Info.vSamplerAndTextures.GetCount() == 0,
                     "CDDI::Update: Sampler and texture heaps are not supported in DX12" );

        auto&                           pPool          = hDDISet->pPool;
        NativeAPI::D3D12DescriptorHeap* DescriptorHeap = pPool->pHeap;
        const UINT                      srvDescriptorSize =
            m_hDevice->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
        (void)srvDescriptorSize;
        (void)DescriptorHeap;

        for( auto& Binding: Info.vRTs )
        {
            (void)Binding;
            /*NativeAPI::D3D12DescriptorHeap* DescriptorHeap = hDDISet->Pool->Heaps[ D3D12_DESCRIPTOR_HEAP_TYPE_RTV ];
            VKE_ASSERT( DescriptorHeap != NativeAPI::Null );
            Binding.ahHandles;
            Binding.binding;
            Binding.count;
            Binding.type;*/

            UNIMPLEMENTED_D3D12_METHOD();
        }

        // Texture is meant to be write
        for( auto& Binding: Info.vTexs )
        {
            UNIMPLEMENTED_D3D12_METHOD();

            VKE_ASSERT( Binding.type == BindingTypes::STORAGE_TEXTURE );

            for( uint32_t i = 0; i < Binding.count; ++i )
            {
                D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc;
                const auto                       pTexture = m_pCtx->GetTexture( Binding.ahHandles[ i ] );
                const auto&                      ViewDesc = pTexture->GetView()->GetDesc();
                // const auto&                      NativeDesc = pTexture->GetDDIObject()->GetDesc();

                UavDesc.Format        = pTexture->GetDDIObject()->GetDesc().Format; /// TODO: handle typeless format
                UavDesc.ViewDimension = Map::DimmensionToUAVDimmension( pTexture->GetDDIObject()->GetDesc().Dimension );
                Helper::CreateUnorderedAccessView( ViewDesc, &UavDesc );

                D3D12_CPU_DESCRIPTOR_HANDLE hCpu = hDDISet->GetCpuDescriptorHandle( Binding.binding );

                m_hDevice->CreateUnorderedAccessView( pTexture->GetDDIObject(), nullptr, &UavDesc, hCpu );
            }
        }

        /// TODO: validate
        for( auto& Binding: Info.vTexViews )
        {
            UNIMPLEMENTED_D3D12_METHOD();

            for( uint32_t i = 0; i < Binding.count; ++i )
            {
                const auto  pTexView    = m_pCtx->GetTextureView( Binding.ahHandles[ i ] );
                const auto& TexViewDesc = pTexView->GetDesc();
                const auto  pTexture    = m_pCtx->GetTexture( TexViewDesc.hTexture );

                D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc;
                Helper::CreateShaderResourceView( TexViewDesc, &SrvDesc );

                auto hCpu = hDDISet->GetCpuDescriptorHandle( Binding.binding );
                m_hDevice->CreateShaderResourceView( pTexture->GetDDIObject(), &SrvDesc, hCpu );
            }
        }

        for( auto& Binding: Info.vSamplers )
        {
            (void)Binding;
            UNIMPLEMENTED_D3D12_METHOD();
        }

        for( auto& Binding: Info.vBuffers )
        {
            for( uint32_t index = 0; index < Binding.count; index++ )
            {
                const auto EngineBuffer    = m_pCtx->GetBuffer( Binding.ahHandles[ index ] );
                const auto pNativeResource = EngineBuffer->GetDDIObject();

                D3D12_CPU_DESCRIPTOR_HANDLE hCpuDescriptorHandle = hDDISet->GetCpuDescriptorHandle( Binding.binding );

                switch( Binding.type )
                {
                    case BINDING_TYPE::CONSTANT_BUFFER:
                    case BINDING_TYPE::DYNAMIC_CONSTANT_BUFFER:
                        // Initialize as CBV
                        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
                        cbvDesc.BufferLocation = pNativeResource->GetGPUVirtualAddress();
                        cbvDesc.SizeInBytes    = Binding.elementSize * Binding.elementCount;

                        m_hDevice->CreateConstantBufferView( &cbvDesc, hCpuDescriptorHandle );
                        break;

                    case BINDING_TYPE::BUFFER:
                    case BINDING_TYPE::DYNAMIC_BUFFER:
                        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
                        srvDesc.Format                     = DXGI_FORMAT_UNKNOWN;
                        srvDesc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
                        srvDesc.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                        srvDesc.Buffer.FirstElement        = Binding.offset;
                        srvDesc.Buffer.NumElements         = Binding.elementCount;
                        srvDesc.Buffer.StructureByteStride = Binding.elementSize;
                        srvDesc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;

                        m_hDevice->CreateShaderResourceView( pNativeResource, &srvDesc, hCpuDescriptorHandle );
                        break;

                    case BINDING_TYPE::READ_ONLY_TEXEL_BUFFER:
                    case BINDING_TYPE::READ_WRITE_TEXEL_BUFFER:
                        D3D12_RESOURCE_DESC ResourceDesc;
                        ResourceDesc = pNativeResource->GetDesc();

                        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
                        uavDesc.Format                      = ResourceDesc.Format;
                        uavDesc.ViewDimension               = D3D12_UAV_DIMENSION_BUFFER;
                        uavDesc.Buffer.FirstElement         = Binding.offset;
                        uavDesc.Buffer.NumElements          = Binding.elementCount;
                        uavDesc.Buffer.StructureByteStride  = Binding.elementSize;
                        uavDesc.Buffer.CounterOffsetInBytes = 0;
                        uavDesc.Buffer.Flags                = D3D12_BUFFER_UAV_FLAG_NONE;

                        m_hDevice->CreateUnorderedAccessView(
                            pNativeResource, nullptr, &uavDesc, hCpuDescriptorHandle );
                        VKE_LOG_ERR( "CDDI::Update: Unhandled buffer type" );
                        break;

                    default:
                        VKE_LOG_ERR( "CDDI::Update: Invalid buffer type" );
                        break;
                }
            }
        }
    }

    void CDDI::Update( const NativeAPI::DescriptorSet& hDDISrcSet, NativeAPI::DescriptorSet* phDDIDstOut )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    NativeAPI::Pipeline CDDI::CreatePipeline( const SPipelineDesc& Desc, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return NativeAPI::Null;
    }

    void CDDI::DestroyPipeline( NativeAPI::Pipeline* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    NativeAPI::PipelineLayout CDDI::CreatePipelineLayout( const SPipelineLayoutDesc& Desc, const void* pAllocator )
    {
        VKE_ASSERT2( m_hDevice != NativeAPI::Null, "CDDI::CreatePipelineLayout: m_hDevice can't be null" );
        VKE_ASSERT2( m_pCtx != nullptr, "CDDI::CreatePipelineLayout: m_pCtx can't be null" );

        D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionedRootSignature;
        versionedRootSignature.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;

        D3D12_ROOT_SIGNATURE_DESC1& rootSignatureDesc = versionedRootSignature.Desc_1_1;

        rootSignatureDesc.NumStaticSamplers = 0;
        rootSignatureDesc.pStaticSamplers   = nullptr;
        rootSignatureDesc.NumParameters     = 0;
        rootSignatureDesc.Flags             = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        Utils::TCDynamicArray< D3D12_ROOT_PARAMETER1, 32 > vRootParameters;
        for( auto& layout: Desc.vDescriptorSetLayouts )
        {
            const NativeAPI::DescriptorSetLayout& hDescriptorSetLayout = m_pCtx->GetDescriptorSetLayout( layout );
            vRootParameters.PushBack( hDescriptorSetLayout->RootParameter );
        }

        // Right now push constants are not being used in Engine. Root signature includes information about push
        // constants should be enabled. Enable this once completed.
        /* for( auto& pushConst: Desc.vPushConstants )
        {
            D3D12_ROOT_PARAMETER1 param;
            param.ParameterType    = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
            param.ShaderVisibility = Convert::getShaderVisibility( pushConst.stages );

            // stages, size, offset
            param.Constants.Num32BitValues = pushConst.size;
            param.Constants.RegisterSpace  = 0;
            param.Constants.ShaderRegister;
        } */

        rootSignatureDesc.NumParameters = vRootParameters.GetCount();
        rootSignatureDesc.pParameters   = vRootParameters.GetData();

        ID3DBlob* pSignatureBlob = nullptr;
        ID3DBlob* pErrorBlob     = nullptr;

        if( FAILED( D3D12SerializeVersionedRootSignature( &versionedRootSignature, &pSignatureBlob, &pErrorBlob ) ) )
        {
            VKE_LOG_ERR( "CDDI::CreatePipelineLayout: Unable to serialize root signature." );
        }

        NativeAPI::D3D12RootSignature* pRootSignature;

        if( FAILED( m_hDevice->CreateRootSignature( Helper::GetNodeMask(),
                                                    pSignatureBlob->GetBufferPointer(),
                                                    pSignatureBlob->GetBufferSize(),
                                                    IID_PPV_ARGS( &pRootSignature ) ) ) )
        {
            VKE_LOG_ERR( "CDDI::CreatePipelineLayout: Unable to create root signature." );
        }

        return pRootSignature;
    }

    void CDDI::DestroyPipelineLayout( NativeAPI::PipelineLayout* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    NativeAPI::Shader CDDI::CreateShader( const SShaderData& Desc, const void* pAllocator )
    {
        NativeAPI::Shader shader;
        Memory::CreateObject( &HeapAllocator, &shader );

        shader->pShaderBytecode = (BYTE*)Desc.pCode;
        shader->BytecodeLength  = Desc.codeSize;

        return shader;
    }

    void CDDI::DestroyShader( NativeAPI::Shader* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    NativeAPI::Sampler CDDI::CreateSampler( const SSamplerDesc& Desc, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return NativeAPI::Null;
    }

    void CDDI::DestroySampler( NativeAPI::Sampler* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    NativeAPI::Event CDDI::CreateEvent( const SEventDesc& Desc, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return NativeAPI::Null;
    }

    void CDDI::DestroyEvent( NativeAPI::Event* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    Result CDDI::CreateCommandBuffers( const SAllocateCommandBufferInfo& Info, NativeAPI::CommandBuffer* pBuffers )
    {
        Result result = VKE_OK;
        VKE_ASSERT2( m_hDevice != NativeAPI::Null, "CDDI::AllocateObjects: m_hDevice can't be null" );

        D3D12_COMMAND_LIST_TYPE type = ( Info.level == COMMAND_BUFFER_LEVEL::PRIMARY ) ? Info.hDDIPool->NativeType
                                                                                       : D3D12_COMMAND_LIST_TYPE_BUNDLE;

        if( type == D3D12_COMMAND_LIST_TYPE_NONE )
        {
            VKE_LOG_WARN( "CDDI::AllocateObjects: Unsupported command list type" );
            return result;
        }

        for( uint32_t i = 0; i < Info.count; i++ )
        {
            NativeAPI::CustomTypes::SCommandBufferPool::SCommandListWithAllocator Pair;

            if( FAILED( m_hDevice->CreateCommandAllocator( type, IID_PPV_ARGS( &Pair.pAllocator ) ) ) )
            {
                VKE_LOG_ERR( "CDDI::AllocateObjects: Failed to create command allocator" );
                result = VKE_FAIL;
                break;
            }

            if( FAILED( m_hDevice->CreateCommandList(
                    0, type, Pair.pAllocator, NativeAPI::Null, IID_PPV_ARGS( &Pair.pCmdList ) ) ) )
            {
                VKE_LOG_ERR( "CDDI::AllocateObjects: Failed to create command list" );
                result = VKE_FAIL;
                break;
            }

            // CreateCommandList always create command list in open state. In order to allocate more objects
            // every command list needs to be in closed state.
            Pair.pCmdList->Close();
            pBuffers[ i ] = Pair.pCmdList;

            Info.hDDIPool->vCommandListsWithAllocators.PushBack( Pair );
        }

        return result;
    }

    void CDDI::FreeObjects( const SFreeCommandBufferInfo& )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    Result CDDI::GetBufferMemoryRequirements( const SBufferDesc& InDesc, SAllocationMemoryRequirementInfo* pOut )
    {
        // TODO(any): Consider not writing D3D12_RESOURCE_DESC twice - here and CreateBuffer.
        NativeAPI::D3D12ResourceDesc ResourceDesc = Convert::GetResourceDesc( InDesc, m_Implementation.Features );

        D3D12_RESOURCE_ALLOCATION_INFO AllocInfo = m_hDevice->GetResourceAllocationInfo( 0, 1, &ResourceDesc );

        pOut->alignment = static_cast< uint32_t >( AllocInfo.Alignment );
        pOut->size      = static_cast< uint32_t >( AllocInfo.SizeInBytes );

        // Constants buffers in D3D12 has to be aligned to 256B.
        if( InDesc.usage & BufferUsages::CONSTANT_BUFFER )
        {
            pOut->alignment = 256u;
        }

        return Result::OK;
    }

    Result CDDI::GetTextureMemoryRequirements( const STextureDesc& Desc, SAllocationMemoryRequirementInfo* pOut )
    {
        // TODO(any): Consider not writing D3D12_RESOURCE_DESC twice - here and CreateTexture.
        NativeAPI::D3D12ResourceDesc ResourceDesc;

        Convert::GetResourceDesc( Desc, m_Implementation.Features, &ResourceDesc );

        D3D12_RESOURCE_ALLOCATION_INFO AllocInfo = m_hDevice->GetResourceAllocationInfo( 0, 1, &ResourceDesc );

        pOut->alignment = static_cast< uint32_t >( AllocInfo.Alignment );
        pOut->size      = static_cast< uint32_t >( AllocInfo.SizeInBytes );

        return Result::OK;
    }

    void CDDI::UpdateDesc( SBufferDesc* pInOut )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::GetFormatFeatures( FORMAT fmt, STextureFormatFeatures* pOut ) const
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::Bind( const SBindPipelineInfo& Info )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::Bind( const SBindDDIDescriptorSetsInfo& Info )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    /*void CDDI::Bind( const SBindRenderPassInfo& Info )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }*/

    void CDDI::Bind( const NativeAPI::CommandBuffer& hDDICmdBuffer, const NativeAPI::Buffer& hDDIBuffer,
                     const uint32_t offset )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::Bind( const NativeAPI::CommandBuffer& hDDICmdBuffer, const NativeAPI::Buffer& hDDIBuffer,
                     const uint32_t offset, const INDEX_TYPE& type )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::UnbindPipeline( const NativeAPI::CommandBuffer&, const NativeAPI::Pipeline& )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::UnbindRenderPass( const NativeAPI::CommandBuffer&, const NativeAPI::RenderPass& )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::Free( NativeAPI::Memory* phMemory, const void* )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    Result CDDI::Allocate( const SAllocateMemoryDesc& Desc, SAllocateMemoryData* pOut )
    {
        VKE_ASSERT2( m_hDevice != NativeAPI::Null, "CDDI::Allocate: m_hDevice can't be null" );

        D3D12_HEAP_DESC heapDesc =
            Convert::GetMemoryHeapDesc( Desc.usage, m_Implementation.Features.ResourceHeapTier >= 2 );
        heapDesc.SizeInBytes = Desc.size;

        // TODO(any): For multi-adapter systems, need to set proper masks.
        heapDesc.Properties.CreationNodeMask = Helper::GetNodeMask();
        heapDesc.Properties.VisibleNodeMask  = Helper::GetNodeMask();

        Result  res = Result::OK;
        HRESULT hr  = S_OK;
        if( FAILED( hr = m_hDevice->CreateHeap( &heapDesc, IID_PPV_ARGS( &pOut->hDDIMemory ) ) ) )
        {
            if( hr == E_OUTOFMEMORY )
            {
                res = Result::NO_MEMORY;
            }
            else
            {
                res = Result::FAIL;
            }
        }
        else
        {
            pOut->heapType = GetMemoryHeapType( Desc.usage );
            pOut->sizeLeft = Desc.size;
        }

        return res;
    }

    MEMORY_HEAP_TYPE CDDI::GetMemoryHeapType( MEMORY_USAGE usage ) const
    {
        /*
        MEMORY_HEAP_TYPE:
        - CPU,
        - GPU,
        - UPLOAD,
        - CPU_CACHED,
        - CPU_COHERENT,
        - OTHER,
        */

        static const MEMORY_USAGE cMask =
            MemoryUsages::CPU_ACCESS | MemoryUsages::GPU_ACCESS | MemoryUsages::CPU_NO_FLUSH | MemoryUsages::CPU_CACHED;

        static MEMORY_USAGE vMap[] = {
            MemoryUsages::CPU_ACCESS,                                                         // CPU
            MemoryUsages::GPU_ACCESS,                                                         // GPU
            MemoryUsages::CPU_ACCESS | MemoryUsages::GPU_ACCESS | MemoryUsages::CPU_NO_FLUSH, // UPLOAD
            MemoryUsages::CPU_ACCESS | MemoryUsages::GPU_ACCESS | MemoryUsages::CPU_CACHED,   // CPU_CACHED
            MemoryUsages::CPU_ACCESS | MemoryUsages::GPU_ACCESS,                              // CPU_COHERENT
            MemoryUsages::UNDEFINED,                                                          // OTHER
        };

        MEMORY_HEAP_TYPE result = MEMORY_HEAP_TYPE::_MAX_COUNT;

        for( uint32_t i = 0; i < MemoryHeapTypes::_MAX_COUNT; i++ )
        {
            if( vMap[ i ] == ( usage & cMask ) )
            {
                result = static_cast< MEMORY_HEAP_TYPE >( i );
                break;
            }
        }

        return result;
    }

    size_t CDDI::GetMemoryHeapTotalSize( MEMORY_HEAP_TYPE type ) const
    {
        VKE_ASSERT2( type < MemoryHeapTypes::_MAX_COUNT, "Incorrect MEMORY_HEAP_TYPE" );
        return m_Implementation.Properties.Memory.HeapProperties[ type ].SizeInBytes;
    }

    size_t CDDI::GetMemoryHeapCurrentSize( MEMORY_HEAP_TYPE ) const
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return 0;
    }

    void* CDDI::MapMemory( const SMapMemoryInfo& Info )
    {
        VKE_ASSERT2( Info.hBuffer != NativeAPI::Null, "CDDI::MapMemory: DX12 can map only resources, not memory." );

        D3D12_RANGE range;
        range.Begin = Info.offset;
        range.End   = Info.offset + Info.size;

        void* pData = nullptr;
        if( FAILED( Info.hBuffer->Map( 0, &range, &pData ) ) )
        {
            VKE_LOG_ERR( "CDDI::MapMemory: Failed to map memory" );
        }

        return pData;
    }

    void CDDI::UnmapMemory( const SMapMemoryInfo& Info )
    {
        VKE_ASSERT2( Info.hBuffer != NativeAPI::Null, "CDDI::MapMemory: DX12 can map only resources, not memory." );
        Info.hBuffer->Unmap( 0, nullptr );
    }

    void CDDI::Reset( const NativeAPI::CommandBuffer&     hCommandBuffer,
                      const NativeAPI::CommandBufferPool& hCommandBufferPool )
    {
        // No-op
    }

    void CDDI::BeginCommandBuffer( const NativeAPI::CommandBuffer&     hCommandBuffer,
                                   const NativeAPI::CommandBufferPool& hCommandBufferPool )
    {
        NativeAPI::D3D12CommandAllocator* pCommandAllocator = hCommandBufferPool->getAllocator( hCommandBuffer );
        VKE_ASSERT( pCommandAllocator != nullptr );

        if( FAILED( pCommandAllocator->Reset() ) )
        {
            VKE_LOG_ERR( "CDDI::Reset: Failed to reset command buffer pool" );
        }

        if( FAILED( hCommandBuffer->Reset( pCommandAllocator, NativeAPI::Null ) ) )
        {
            VKE_LOG_ERR( "CDDI::Reset: Failed to reset command buffer" );
        }
    }

    void CDDI::EndCommandBuffer( const NativeAPI::CommandBuffer& hCommandBuffer )
    {
        if( FAILED( hCommandBuffer->Close() ) )
        {
            VKE_LOG_ERR( "CDDI::EndCommandBuffer: Failed to close command buffer" );
        }
    }

    void CDDI::Barrier( const NativeAPI::CommandBuffer& hCommandBuffer, const SBarrierInfo& Info )
    {
        DDIBarrierArray vBarriers( 0 );

        // Global memory barriers are not supported in D3D12
        // for( auto& barrier: Info.vMemoryBarriers )
        //{
        //    Convert::TransitionBarrier( vBarriers[ barrierIndex++ ], barrier );
        //}

        for( auto& barrier: Info.vTextureBarriers )
        {
            Helper::CreateLegacySubresourceBarriers( barrier, vBarriers );
        }

        for( auto& barrier: Info.vBufferBarriers )
        {
            Helper::CreateLegacySubresourceBarriers( barrier, vBarriers );
        }

        if( vBarriers.GetCount() > 0 )
        {
            hCommandBuffer->ResourceBarrier( vBarriers.GetCount(), vBarriers.GetData() );
        }
        else
        {
            VKE_LOG_WARN( "CDDI::Barrier: Requested barrier resulted in 0 actual barriers." );
        }
    }

    void CDDI::SetState( const NativeAPI::CommandBuffer& hCommandBuffer, const SViewportDesc& Desc )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::SetState( const NativeAPI::CommandBuffer& hCommandBuffer, const SScissorDesc& Desc )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::Draw( const NativeAPI::CommandBuffer& hCommandBuffer, const uint32_t& vertexCount,
                     const uint32_t& instanceCount, const uint32_t& firstVertex, const uint32_t& firstInstance )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::DrawIndexed( const NativeAPI::CommandBuffer& hCommandBuffer, const SDrawParams& Params )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::DrawMesh( const NativeAPI::CommandBuffer& hCommandBuffer, uint32_t width, uint32_t height,
                         uint32_t depth )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::BeginRenderPass( NativeAPI::CommandBuffer     pNativeCommandBuffer,
                                const SBeginRenderPassInfo2& EngineRenderPassInfo )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::BeginRenderPass( NativeAPI::CommandBuffer    pNativeCommandBuffer,
                                const SBeginRenderPassInfo& EngineRenderPassInfo )
    {
        if( EngineRenderPassInfo.hDDIRenderPass == NativeAPI::Null )
        {
            VKE_LOG_ERR( "CDDI::BeginRenderPass: Render pass is NULL" );
            return;
        }

        const NativeAPI::RenderPass pNativeRenderPass = EngineRenderPassInfo.hDDIRenderPass;
        pNativeRenderPass->Reset();

        const auto& NativeFirstSubpass = pNativeRenderPass->CurrentSubpass();
        const auto& NativeClearInfo    = pNativeRenderPass->Clear;

        // Add custom breadcrumb before BeginRenderPass() insert anything on cmdlist
        PIXBeginEvent( pNativeCommandBuffer,
                       PIX_COLOR( 255, 0, 0 ),
                       "EmulatedRenderPass: Begin: %s",
                       EngineRenderPassInfo.hDDIRenderPass->pName );

        // Record barriers for potential clear operations.
        if( NativeClearInfo.Barriers.count > 0 )
        {
            pNativeCommandBuffer->ResourceBarrier( NativeClearInfo.count, &NativeClearInfo.Barriers.Data[ 0 ] );
        }

        // Record clear operations
        for( uint32_t index = 0; index < NativeClearInfo.count; index++ )
        {
            const auto& NativeClearInfoSurface = pNativeRenderPass->Clear.Data[ index ];

            if( NativeClearInfoSurface.Type == NativeClearInfoSurface.RENDER_TARGET )
            {
                const auto& Args = NativeClearInfoSurface.RenderTargetView;
                pNativeCommandBuffer->ClearRenderTargetView( Args.hRenderTargetView, Args.aColorRGBA, 1, &Args.Rect );
            }
            else if( NativeClearInfoSurface.Type == NativeClearInfoSurface.DEPTH_STENCIL_VIEW )
            {
                const auto& Args = NativeClearInfoSurface.DepthStencilView;
                pNativeCommandBuffer->ClearDepthStencilView(
                    Args.hDepthStencilView, Args.ClearFlags, Args.depth, Args.stencil, 1, &Args.Rect );
            }
        }

        // Record barriers for default subpass
        if( NativeFirstSubpass.BeginBarriers.count > 0 )
        {
            pNativeCommandBuffer->ResourceBarrier( NativeFirstSubpass.BeginBarriers.count,
                                                   &NativeFirstSubpass.BeginBarriers.Data[ 0 ] );
        }

        // TODO(blturkot): DepthStencilView need to be passed to
        // D3D12_GRAPHICS_PIPELINE_STATE_DESC::D3D12_DEPTH_STENCIL_DESC
        // TODO(blturkot): D3D12_GRAPHICS_PIPELINE_STATE_DESC also knows about RTVFormat, DSVFormat and number of render
        // targets.

        // Configure output merger
        pNativeCommandBuffer->OMSetRenderTargets( pNativeRenderPass->RenderTargetViews.count,
                                                  &pNativeRenderPass->RenderTargetViews.Data[ 0 ],
                                                  FALSE, // TRUE if render targets are like descriptor table
                                                  &pNativeRenderPass->DepthStencilView.hCPUDescriptor );

        // Configure rasterizer
        D3D12_RECT scizzorRect;
        Convert::GetRect( EngineRenderPassInfo.RenderArea, &scizzorRect );
        pNativeCommandBuffer->RSSetScissorRects( 1, &scizzorRect );
    }

    void CDDI::EndRenderPass( NativeAPI::CommandBuffer pNativeCommandBuffer, NativeAPI::RenderPass pNativeRenderPass )
    {
        if( pNativeRenderPass == NativeAPI::Null )
        {
            VKE_LOG_ERR( "CDDI::EndRenderPass: Render pass is NULL" );
            return;
        }

        // TODO(blturkot): When subpasses will be fully implemented in engine, we'll need to check which was last and
        // issue proper barrier to end state. Right now it's primitive technology and assumes just one subpass with so
        // end barriers are baked into render pass.
        if( pNativeRenderPass->EndBarriers.count > 0 )
        {
            pNativeCommandBuffer->ResourceBarrier( pNativeRenderPass->EndBarriers.count,
                                                   &pNativeRenderPass->EndBarriers.Data[ 0 ] );
        }

        // Add end custom breadcrumb after everything was done on cmdlist.
        PIXEndEvent( pNativeCommandBuffer );
    }

    void CDDI::Copy( const NativeAPI::CommandBuffer& hDDICmdBuffer, const SCopyTextureInfoEx& Info )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::Copy( const NativeAPI::CommandBuffer& hCmdBuffer, const SCopyBufferInfo& Info )
    {
        hCmdBuffer->CopyBufferRegion( Info.pDstBuffer->GetDDIObject(),
                                      Info.Region.dstBufferOffset,
                                      Info.hDDISrcBuffer,
                                      Info.Region.srcBufferOffset,
                                      Info.Region.size );
    }

    void CDDI::Copy( const NativeAPI::CommandBuffer& hDDICmdBuffer, const SCopyBufferToTextureInfo& Info )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::Blit( const NativeAPI::CommandBuffer& hAPICmdBuffer, const SBlitTextureInfo& Info )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::SetEvent( const NativeAPI::Event& hDDIEvent )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::SetEvent( const NativeAPI::CommandBuffer& hDDICmdBuffer, const NativeAPI::Event& hDDIEvent,
                         const PIPELINE_STAGES& stages )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::Reset( const NativeAPI::Event& hDDIInOut )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::Reset( const NativeAPI::CommandBuffer& hDDICmdBuffer, const NativeAPI::Event& hDDIEvent,
                      const PIPELINE_STAGES& stages )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    bool CDDI::IsSet( const NativeAPI::Event& hDDIEvent )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return false;
    }

    Result CDDI::Submit( const SSubmitInfo& Info )
    {
        NativeAPI::D3D12CommandList* const* ppCommandLists =
            (NativeAPI::D3D12CommandList* const*)&Info.pDDICommandBuffers[ 0 ];
        Info.hDDIQueue->ExecuteCommandLists( Info.commandBufferCount, ppCommandLists );
        return Result::OK;
    }

    Result CDDI::Present( const SPresentData& Info )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return Result::OK;
    }

    Result CDDI::CreateSwapChain( const SSwapChainDesc& Desc, const void*, SDDISwapChain* pOut )
    {
        VKE_ASSERT2( m_hDevice != NativeAPI::Null, "CDDI::CreateSwapChain: m_hDevice can't be null" );

        QueryPresentSurfaceCaps( nullptr, &pOut->Caps );

        UINT backBufferCount = Desc.backBufferCount;
        if( Constants::_SOptimal::IsOptimal( Desc.backBufferCount ) )
        {
            // Default to triple buffering
            backBufferCount = 2u;
        }
        else if( Desc.backBufferCount > DXGI_MAX_SWAP_CHAIN_BUFFERS )
        {
            VKE_LOG_ERR( "CDDI::CreateSwapChain: Unspecified number of backBufferCount or exceeds max supported." );
            return Result::NOT_SUPPORTED;
        }

        auto dxgiFormat = Convert::GetDXGIFormat( Desc.format );
        if( dxgiFormat == DXGI_FORMAT_UNKNOWN )
        {
            VKE_LOG_ERR( "CDDI::CreateSwapChain: Unsupported swapchain format (no matching engine with DXGI format)." );
            return Result::NOT_SUPPORTED;
        }

        D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport = { .Format = dxgiFormat };
        m_hDevice->CheckFeatureSupport( D3D12_FEATURE_FORMAT_SUPPORT, &formatSupport, sizeof( formatSupport ) );

        if( ( formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_RENDER_TARGET ) == 0 )
        {
            VKE_LOG_ERR( "CDDI::CreateSwapChain: Format can't be used as render target (required)." );
            return Result::NOT_SUPPORTED;
        }

        if( ( formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE ) == 0 )
        {
            VKE_LOG_ERR( "CDDI::CreateSwapChain: Format can't be used as shader resource (required)." );
            return Result::NOT_SUPPORTED;
        }

        if( ( formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_DISPLAY ) == 0 )
        {
            VKE_LOG_ERR( "CDDI::CreateSwapChain: Format can't be used to present() (required)." );
            return Result::NOT_SUPPORTED;
        }

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
        swapChainDesc.Width  = Desc.Size.width;
        swapChainDesc.Height = Desc.Size.height;
        // TODO(blturkot): Validate for proper swapchain render target
        swapChainDesc.Format             = dxgiFormat;
        swapChainDesc.Stereo             = FALSE; // This is to enable stereoscopic rendering for 3D glasses.
        swapChainDesc.SampleDesc.Count   = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
        swapChainDesc.BufferCount = backBufferCount;
        swapChainDesc.Scaling     = DXGI_SCALING_NONE;
        swapChainDesc.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        swapChainDesc.AlphaMode   = DXGI_ALPHA_MODE_UNSPECIFIED;
        swapChainDesc.Flags       = 0;

        // Enable tearing if supported and allowed by application.
        // if( NativeAPI::SImplementation::sTearingSupported )
        //{
        //    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        //}

        // TODO(blturkot): Implement support for fullscreen.
        // TODO(blturkot): Support for vsync
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc;
        fullscreenDesc.RefreshRate.Numerator   = 0;
        fullscreenDesc.RefreshRate.Denominator = 0;
        fullscreenDesc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        fullscreenDesc.Scaling                 = DXGI_MODE_SCALING_UNSPECIFIED;
        fullscreenDesc.Windowed                = TRUE;
        (void)fullscreenDesc;

        HWND  hWnd  = reinterpret_cast< HWND >( Desc.pWindow->GetDesc().hWnd );
        auto& queue = m_DeviceProperties.vQueueFamilies[ Desc.queueFamilyIndex ].vQueues[ 0 ];

        IDXGISwapChain1* pSwapChain1 = nullptr;
        if( FAILED( m_Implementation.spFactory->CreateSwapChainForHwnd(
                queue, hWnd, &swapChainDesc, NULL, NULL, &pSwapChain1 ) ) )
        {
            VKE_LOG_ERR( "CDDI::CreateSwapChain: Failed to create swap chain" );
            return Result::FAIL;
        }

        // Update out handle to higher SwapChain ptr. Similar to .As() on ComPtr
        pOut->hSwapChain = reinterpret_cast< NativeAPI::SwapChain >( pSwapChain1 );

        // DXGI has already bound the swapchain to the window.
        IDXGIOutput* pPresentOutput;
        pOut->hSwapChain->GetContainingOutput( &pPresentOutput );
        pOut->hSurface = reinterpret_cast< NativeAPI::PresentSurface >( pPresentOutput );

        DXGI_COLOR_SPACE_TYPE dxgiColorSpace        = Map::GetDXGIColorSpace( Desc.colorSpace );
        UINT                  dxgiColorSpaceSupport = 0;

        pOut->hSwapChain->CheckColorSpaceSupport( dxgiColorSpace, &dxgiColorSpaceSupport );
        if( ( dxgiColorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT ) == 0 )
        {
            VKE_LOG_WARN( "CDDI::CreateSwapChain: Color space not supported for swapchain." );
        }
        else
        {
            pOut->hSwapChain->SetColorSpace1( dxgiColorSpace );
        }

        for( UINT i = 0; i < backBufferCount; i++ )
        {
            NativeAPI::Texture bbTexture = NativeAPI::Null;
            // This is required when NativeAPI::Texture is a custom object.
            // if( VKE_FAILED( Memory::CreateObject( &HeapAllocator, &bbTexture ) ) )
            //{
            //    VKE_LOG_ERR( "CDDI::CreateSwapChain: Failed to create back buffer texture object." );
            //}

            pOut->hSwapChain->GetBuffer( i, IID_PPV_ARGS( &bbTexture ) );

            // Create swapchain already creates required resources but doesn't have views.
            // To cheat engine, we can store the texture in vImages and create null views.
            pOut->vImages.PushBack( bbTexture );
            pOut->vImageViews.PushBack( nullptr );
        }

        // Update size from SwapChain after creation
        UINT width = 0, height = 0;
        pOut->hSwapChain->GetSourceSize( &width, &height );
        pOut->Size.width        = (VKE::RenderSystem::TextureSizeType)width;
        pOut->Size.height       = (VKE::RenderSystem::TextureSizeType)height;
        pOut->Format.format     = Desc.format;
        pOut->Format.colorSpace = Desc.colorSpace;

        return Result::OK;
    }

    void CDDI::DestroySwapChain( SDDISwapChain* pInOut, const void* )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    Result CDDI::ReCreateSwapChain( const SSwapChainDesc& Desc, SDDISwapChain* pOut )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return Result::OK;
    }

    Result CDDI::QueryPresentSurfaceCaps( const NativeAPI::PresentSurface& hSurface, SPresentSurfaceCaps* pOut )
    {
        // No enum or list for DXGI, need to loop and query supported formats.
        struct DXGI_ENGINE_FORMAT_PAIR
        {
            DXGI_FORMAT dxgiFormat;
            FORMAT      engineFormat;
        };

        static const DXGI_ENGINE_FORMAT_PAIR vApiAllowedFormats[] = {
            // Feature l1evel >= 9.1
            { DXGI_FORMAT_R8G8B8A8_UNORM, FORMAT::R8G8B8A8_UNORM },
            { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, FORMAT::R8G8B8A8_SRGB },
            { DXGI_FORMAT_B8G8R8A8_UNORM, FORMAT::B8G8R8A8_UNORM },     // Except 10.x on Windows Vista
            { DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, FORMAT::B8G8R8A8_SRGB }, // Except 10.x on Windows Vista
            // Feature level >= 10.0
            { DXGI_FORMAT_R16G16B16A16_FLOAT, FORMAT::R16G16B16A16_SFLOAT },
            { DXGI_FORMAT_R10G10B10A2_UNORM, FORMAT::R10G10B10A2_UNORM },
            // Feature level >= 11.0
            { DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM, FORMAT::R10G10B10_XR_BIAS_A2_UNORM },
        };

        D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport = {};

        for( auto& format: vApiAllowedFormats )
        {
            formatSupport.Format = format.dxgiFormat;
            m_hDevice->CheckFeatureSupport( D3D12_FEATURE_FORMAT_SUPPORT, &formatSupport, sizeof( formatSupport ) );
            if( ( formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_DISPLAY ) != 0 )
            {
                pOut->vFormats.PushBack( { format.engineFormat, COLOR_SPACE::SRGB } );
            }
        }

        // Present modes - only FIFO is guaranteed to be supported.
        pOut->vModes.PushBack( PRESENT_MODE::FIFO );

        // Mailbox is similar to FIFO but allows replacing the queued frames if the queue is full.
        // This is not supported in DXGI natively and requires emulation.
        // TODO(Any): Uncomment once emulation is implemented.
        // pOut->vModes.PushBack( PRESENT_MODE::MAILBOX );

        if( NativeAPI::SImplementation::SDeviceFeatures::sTearingSupported )
        {
            pOut->vModes.PushBack( PRESENT_MODE::IMMEDIATE );
        }

        pOut->canBeUsedAsRenderTarget = true;
        pOut->minImageCount           = 1;
        pOut->maxImageCount           = DXGI_MAX_SWAP_CHAIN_BUFFERS;

        // If size is 0, 0 CreateSwapChain will use the window's client area size.
        pOut->MinSize = { 1, 1 };
        pOut->MaxSize = { D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION, D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION };

        return Result::OK;
    }

    Result CDDI::GetCurrentBackBufferIndex( const SDDISwapChain& SwapChain, const SDDIGetBackBufferInfo& Info,
                                            uint32_t* pOut )
    {
        *pOut = static_cast< uint32_t >( SwapChain.hSwapChain->GetCurrentBackBufferIndex() );
        return Result::OK;
    }

    void CDDI::Convert( const SClearValue& In, NativeAPI::ClearValue* pOut )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    // Debug

    void CDDI::BeginDebugInfo( const NativeAPI::CommandBuffer& hDDICmdBuff, const SDebugInfo* pInfo )
    {
        if( pInfo != nullptr )
        {
            PIXBeginEvent( hDDICmdBuff, Convert::GetPixColor( pInfo->Color ), pInfo->pText );
        }
    }

    void CDDI::EndDebugInfo( const NativeAPI::CommandBuffer& hDDICmdBuff )
    {
        PIXEndEvent( hDDICmdBuff );
    }

    void CDDI::SetObjectDebugName( const uint64_t& handle, const uint32_t& objType, cstr_t pName ) const
    {
        VKE_ASSERT2( handle != 0, "CDDI::SetObjectDebugName: Attempting to SetName on Null object." );

        wchar_t buffer[ 256 ];
        MultiByteToWideChar( CP_UTF8, 0, pName, -1, buffer, 256 );

        switch( objType )
        {
            case ApiObjectTypes::ADAPTER:
                ( (NativeAPI::Adapter)handle )->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( pName ), pName );
                break;
            case ApiObjectTypes::DEVICE:
                ( (NativeAPI::Device)handle )->SetName( buffer );
                break;
            case ApiObjectTypes::GPU_FENCE:
                ( (NativeAPI::GPUFence)handle )->pObject->SetName( buffer );
                break;
            case ApiObjectTypes::COMMAND_BUFFER:
                ( (NativeAPI::CommandBuffer)handle )->SetName( buffer );
                break;
            case ApiObjectTypes::CPU_FENCE:
                ( (NativeAPI::CPUFence)handle )->pObject->SetName( buffer );
                break;
            case ApiObjectTypes::BUFFER:
                ( (NativeAPI::Buffer)handle )->SetName( buffer );
                break;
            case ApiObjectTypes::TEXTURE:
                ( (NativeAPI::Texture)handle )->SetName( buffer );
                break;
            /*case ApiObjectTypes::BUFFER_VIEW:
                ( (NativeAPI::BufferView)handle )->SetName( buffer );
                break;
            case ApiObjectTypes::TEXTURE_VIEW:
                ( (NativeAPI::BufferView)handle )->SetName( buffer );
                break;*/
            case ApiObjectTypes::PIPELINE_LAYOUT:
                ( (NativeAPI::PipelineLayout)handle )->SetName( buffer );
                break;
            case ApiObjectTypes::PIPELINE:
                ( (NativeAPI::Pipeline)handle )->SetName( buffer );
                break;
                /* case ApiObjectTypes::DESCRIPTOR_SET:
                     ( (NativeAPI::DescriptorSet)handle )->SetName( buffer );
                     break;*/
            case ApiObjectTypes::FRAMEBUFFER:
                ( (NativeAPI::Framebuffer)handle )->SetName( buffer );
                break;
            case ApiObjectTypes::COMMAND_POOL:
                ( (NativeAPI::CommandBufferPool)handle )->SetName( buffer );
                break;

            case ApiObjectTypes::CONTEXT:
            case ApiObjectTypes::DEVICE_MEMORY:
            case ApiObjectTypes::QUERY_POOL:
            case ApiObjectTypes::EVENT:
            case ApiObjectTypes::SHADER:
            case ApiObjectTypes::PIPELINE_CACHE:
            case ApiObjectTypes::RENDER_PASS:
            case ApiObjectTypes::DESCRIPTOR_SET_LAYOUT:
            case ApiObjectTypes::SAMPLER:
            case ApiObjectTypes::DESCRIPTOR_POOL:
                VKE_LOG_WARN( "CDDI::SetObjectDebugName: Unsupported objType" );
                break;

            default:
                VKE_LOG_ERR( "CDDI::SetObjectDebugName: Unhandled objType" );
                break;
        }
    }

    void CDDI::SetQueueDebugName( uint64_t handle, cstr_t pName ) const
    {
        NativeAPI::Queue pQueue = (NativeAPI::Queue)handle;
        VKE_ASSERT2( pQueue != NativeAPI::Null, "CDDI::SetQueueDebugName: Queue is null" );
        pQueue->SetName( (LPCWSTR)pName );
    }

    bool CDDI::IsSignaled( const NativeAPI::CPUFence& hFence ) const
    {
        VKE_ASSERT2( m_hDevice != NativeAPI::Null, "CDDI::IsSignaled: m_hDevice is null" );
        return hFence->pObject->GetCompletedValue() >= hFence->Value;
    }

    void CDDI::Reset( NativeAPI::CPUFence* phFence )
    {
        NativeAPI::CPUFence pFence = *phFence;
        pFence->Value              = 0;

        if( FAILED( pFence->pObject->Signal( 0 ) ) )
        {
            VKE_LOG_ERR( "CDDI::Reset: Failed to reset fence" );
        }
    }

    void CDDI::Reset( NativeAPI::Fence* phFence, NativeAPI::FenceValue value )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    NativeAPI::FenceValue CDDI::GetCompletedValue( const NativeAPI::Fence& hFence ) const
    {
        UNIMPLEMENTED_D3D12_METHOD();

        return 0;
    }

    Result CDDI::WaitForFences( const NativeAPI::CPUFence& hFence, uint64_t timeout ) const
    {
        UNIMPLEMENTED_D3D12_METHOD();
        // TODO(blturkot): Wait for fence implementation.
        return Result::OK;
    }

    Result CDDI::WaitForFence( NativeAPI::Fence Fence, NativeAPI::FenceValue value ) const
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return Result::OK;
    }

    Result CDDI::WaitForQueue( const NativeAPI::Queue& hQueue )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        // TODO(blturkot): Each queue needs to have its own fence for synchronization.
        return Result::OK;
    }

    Result CDDI::WaitForDevice()
    {
        UNIMPLEMENTED_D3D12_METHOD();
        // TODO(blturkot): Get all queues and wait for them.
        return Result::OK;
    }

} // namespace VKE::RenderSystem

#endif // VKE_RENDER_SYSTEM_D3D12
