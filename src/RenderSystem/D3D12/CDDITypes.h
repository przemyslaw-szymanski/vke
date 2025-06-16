#pragma once
#if VKE_D3D12_RENDER_SYSTEM

#include <d3d12.h>
#include <dxgi1_6.h>

namespace VKE
{
    namespace RenderSystem
    {
        static constexpr auto DDI_NULL_HANDLE = nullptr;
        using DDIBuffer              = ID3D12Resource*;
        using DDIPipeline            = ID3D12PipelineState*;
        using DDITexture             = ID3D12Resource*;
        using DDISampler             = void*;
        using DDIRenderPass          = ID3D12Object*;
        using DDICommandBuffer       = ID3D12CommandList*;
        using DDITextureView         = void*;
        using DDIBufferView          = void*;
        using DDIFence               = ID3D12Fence1*;
        using DDISemaphore           = ID3D12Fence1*;
        using DDIDevice              = ID3D12Device10*;
        using DDIDescriptorPool      = ID3D12DescriptorHeap*;
        using DDIDescriptorSet       = D3D12_DESCRIPTOR_RANGE1;
        using DDIDescriptorSetLayout = D3D12_DESCRIPTOR_RANGE1;
        using DDICommandBufferPool   = ID3D12CommandAllocator*;
        using DDIFramebuffer         = ID3D12Resource*;
        using DDIClearValue          = D3D12_CLEAR_VALUE;
        using DDIQueue               = ID3D12CommandQueue*;
        using DDIFormat              = DXGI_FORMAT;
        using DDIImageType           = D3D12_RESOURCE_DIMENSION;

        // TODO(blturkot): D3D12 use single desc for all shader bindings
        enum DDIImageViewType
        {
            D3D12_VIEW_TYPE_BUFFER,
            D3D12_VIEW_TYPE_TEX1D,
            D3D12_VIEW_TYPE_TEX1D_ARRAY,
            D3D12_VIEW_TYPE_TEX2D,
            D3D12_VIEW_TYPE_TEX2D_ARRAY,
            D3D12_VIEW_TYPE_TEX2DMS,
            D3D12_VIEW_TYPE_TEX2DMS_ARRAY,
            D3D12_VIEW_TYPE_TEX3D,
            D3D12_VIEW_TYPE_TEXCUBE,
            D3D12_VIEW_TYPE_TEXCUBE_ARRAY,
            D3D12_VIEW_TYPE_RT_ACC_STRUCT,
        };

        using DDIImageLayout     = D3D12_RESOURCE_FLAGS;
        using DDIImageUsageFlags = D3D12_RESOURCE_FLAGS;
        using DDIMemory          = ID3D12Object*;
        using DDIPresentSurface  = ID3D12Resource*;
        using DDISwapChain       = IDXGISwapChain4*;
        using DDIAdapter         = IDXGIAdapter4*;
        using DDIShader          = D3D12_SHADER_BYTECODE;
        using DDIPipelineLayout  = void*; // = D3D12_GRAPHICS_PIPELINE_STATE_DESC;
        using DDIDeviceSize      = DXGI_QUERY_VIDEO_MEMORY_INFO;
        using DDIEvent           = HANDLE;
    } // RenderSystem
    namespace RenderSystem::NativeAPI
    {
        static const decltype( nullptr ) Null;
        using Buffer              = ID3D12Resource;
        using Pipeline            = ID3D12PipelineState;
        using Texture             = ID3D12Resource;
        using Sampler             = void*;
        using RenderPass          = ID3D12Object;
        using CommandBuffer       = ID3D12CommandList;
        using TextureView         = void*;
        using BufferView          = void*;
        using CPUFence            = ID3D12Fence;
        using GPUFence            = ID3D12Fence;
        using Device              = ID3D12Device10;
        using DescriptorPool      = ID3D12DescriptorHeap;
        using DescriptorSet       = D3D12_DESCRIPTOR_RANGE1;
        using DescriptorSetLayout = D3D12_DESCRIPTOR_RANGE1;
        using CommandBufferPool   = ID3D12CommandAllocator;
        using Framebuffer         = ID3D12Resource;
        using ClearValue          = D3D12_CLEAR_VALUE;
        using Queue               = ID3D12CommandQueue;
        using Format              = DXGI_FORMAT;
        using ImageType           = D3D12_RESOURCE_DIMENSION;
        
        enum ImageViewType
        {
            D3D12_VIEW_TYPE_BUFFER,
            D3D12_VIEW_TYPE_TEX1D,
            D3D12_VIEW_TYPE_TEX1D_ARRAY,
            D3D12_VIEW_TYPE_TEX2D,
            D3D12_VIEW_TYPE_TEX2D_ARRAY,
            D3D12_VIEW_TYPE_TEX2DMS,
            D3D12_VIEW_TYPE_TEX2DMS_ARRAY,
            D3D12_VIEW_TYPE_TEX3D,
            D3D12_VIEW_TYPE_TEXCUBE,
            D3D12_VIEW_TYPE_TEXCUBE_ARRAY,
            D3D12_VIEW_TYPE_RT_ACC_STRUCT,
        };

        using ImageLayout     = D3D12_RESOURCE_FLAGS;
        using ImageUsageFlags = D3D12_RESOURCE_FLAGS;
        using Memory          = ID3D12Object;
        using PresentSurface  = ID3D12Resource;
        using SwapChain       = IDXGISwapChain4;
        using Adapter         = IDXGIAdapter4;
        using Shader          = D3D12_SHADER_BYTECODE;
        using PipelineLayout  = D3D12_GRAPHICS_PIPELINE_STATE_DESC;
        using DeviceSize      = DXGI_QUERY_VIDEO_MEMORY_INFO;
        using Event           = HANDLE;
       
    }
} // VKE

#endif // VKE_D3D12_RENDER_SYSTEM