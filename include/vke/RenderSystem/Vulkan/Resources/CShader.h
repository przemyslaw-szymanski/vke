#pragma once
#include "RenderSystem/Common.h"
#include "Core/Utils/TCSmartPtr.h"
#if VKE_VULKAN_RENDERER
#include "ThirdParty/SpirV/glslang/Include/ShHandle.h"
#include "RenderSystem/Vulkan/Vulkan.h"
#include "Core/Resources/CFile.h"
#include "Core/VKEConfig.h"

namespace VKE
{
    namespace RenderSystem
    {
        struct SShaderInitInfo
        {
        };

        class VKE_API CShader final : public Resources::CResource
        {
            friend class CShaderManager;
            friend class SShaderCompiler;
            public:

                using InitInfo = SShaderInitInfo;
                using ShaderBinaryBuffer = Utils::TCDynamicArray< uint8_t, Config::Resource::Shader::DEFAULT_SHADER_BINARY_SIZE >;

            public:

                        CShader(SHADER_TYPE type);
                        ~CShader();

                void    Init(const InitInfo& Info);
                void    Release(CShaderManager* pMgr);

            protected:

                glslang::TShader    m_Shader;
                FilePtr             m_pFile;
                VkShaderModule      m_vkModule = VK_NULL_HANDLE;
                SShaderDesc         m_Desc;
                InitInfo            m_Info;
        };


        class VKE_API CShaderProgram final : public Resources::CResource
        {
            friend class CShaderManager;
            public:

            public:

                        CShaderProgram();
                        ~CShaderProgram();

                void    Release(CShaderManager* pMgr);

            protected:

                glslang::TProgram   m_Program;
                FilePtr             m_pFile;
                SShaderProgramDesc  m_Desc;
        };

    } // RenderSystem
    using ShaderPtr = Utils::TCWeakPtr< RenderSystem::CShader >;
    using ShaderRefPtr = Utils::TCObjectSmartPtr< RenderSystem::CShader >;
    using ShaderProgramPtr = Utils::TCWeakPtr< RenderSystem::CShaderProgram >;
    using ShaderProgramRefPtr = Utils::TCObjectSmartPtr< RenderSystem::CShaderProgram >;
} // VKE
#endif // VKE_VULKAN_RENDERER