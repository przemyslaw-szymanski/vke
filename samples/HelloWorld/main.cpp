

#include "VKE.h"
#include "vke/Core/Utils/TCConstantArray.h"
#include "vke/Core/Memory/CMemoryPoolManager.h"
#include "vke/Core/Utils/TCString.h"

struct SGfxContextListener : public VKE::RenderSystem::EventListeners::IGraphicsContext
{
    bool OnRenderFrame(VKE::RenderSystem::CGraphicsContext* /*pCtx*/) override
    {
        return true;
    }
};

namespace VKE
{
    template
    <
        typename T,
        uint32_t DEFAULT_COUNT = 1024,
        typename HandleType = uint32_t,
        class DataContainerType = VKE::Utils::TCDynamicArray< T, DEFAULT_COUNT >,
        class HandleContainerType = VKE::Utils::TCDynamicArray< HandleType, DEFAULT_COUNT >
    >
    class TCHandledBuffer
    {
        public:

            using HandleBuffer = Utils::TCDynamicArray< HandleContainerType >;
            using FreeHandleBuffer = Utils::TCDynamicArray< HandleType, DEFAULT_COUNT >;

        public:

            TCHandledBuffer()
            {
                // Add null handle
                PushBack( T() );
            }

            ~TCHandledBuffer()
            {
            }

            HandleType PushBack(const T& Data)
            {
                HandleType idx;
                if( m_vFreeHandles.PopBack( &idx ) )
                {
                    m_vBuffer[ idx ] = Data;
                    m_vHandleMap[ idx ] = idx;
                }
                else
                {
                    uint32_t handleIdx = m_vBuffer.PushBack( Data );
                    idx = m_vHandleMap.PushBack( handleIdx );
                }
                return idx;
            }

            void Free(const HandleType& idx)
            {
                m_vFreeHandles.PushBack( idx );
            }

            void Remove(const HandleType& idx)
            {
                m_vBuffer.Remove( idx );
                m_vHandleMap.Remove( idx );
                for( uint32_t i = idx; i < m_vHandleMap.GetCount(); ++i )
                {
                    m_vHandleMap[ i ] = i;
                }
            }

            T& At(const HandleType& idx)
            {
                return m_vBuffer[ _GetDataIdx( idx ) ];
            }

            const T& At(const HandleType& idx) const
            {
                return m_vBuffer[ _GetDataIdx( idx ) ];
            }

        protected:

            vke_force_inline
            HandleType _GetDataIdx(const HandleType& idx) const
            {
                return m_vHandleMap[ idx ];
            }

        protected:

            FreeHandleBuffer    m_vFreeHandles;
            DataContainerType   m_vBuffer;
            HandleBuffer        m_vHandleMap;
    };
}

//__forceinline
//__inline
static bool __fastcall PushConstantsMemcmp( const void* pPtr1, const void* pPtr2, size_t num )
{
    const uint64_t* pFirstU64 = reinterpret_cast<const uint64_t*>( pPtr1 );
    const uint64_t* pSecondU64 = reinterpret_cast<const uint64_t*>( pPtr2 );
    if( *pFirstU64++ != *pSecondU64++ )
    {
        return false;
    }
    size_t currSize = sizeof(uint64_t);
    while( currSize < num )
    {
        if( *pFirstU64++ != *pSecondU64++ )
        {
            return false;
        }
        currSize += sizeof( uint64_t );
    }
    if( currSize < num )
    {
        const uint32_t* pFirstU32 = reinterpret_cast< const uint32_t* >( pFirstU64 );
        const uint32_t* pSecondU32 = reinterpret_cast< const uint32_t* >( pSecondU64 );
        {
            if( *pFirstU32++ != *pSecondU32++ )
            {
                return false;
            }
        }
    }
    return true;
}

static int __sse_memcmp_tail( const uint16_t *a, const uint16_t *b, int len )
{
    switch( len )
    {
        case 8:
        if( *a++ != *b++ ) return -1;
        case 7:
        if( *a++ != *b++ ) return -1;
        case 6:
        if( *a++ != *b++ ) return -1;
        case 5:
        if( *a++ != *b++ ) return -1;
        case 4:
        if( *a++ != *b++ ) return -1;
        case 3:
        if( *a++ != *b++ ) return -1;
        case 2:
        if( *a++ != *b++ ) return -1;
        case 1:
        if( *a != *b ) return -1;
    }
    return 0;
}


static int __sse_memcmp( const void *pa, const void *pb, int size )
{
    const uint16_t* a = reinterpret_cast< const uint16_t* >( pa );
    const uint16_t* b = reinterpret_cast< const uint16_t* >( pb );
    const int half_words = size / sizeof( uint64_t );
    int i = 0;
    int len = half_words;
    int aligned_a = 0, aligned_b = 0;
    if( !len ) return 0;
    if( !a && !b ) return 0;
    if( !a || !b ) return -1;
    if( ( unsigned long long )a & 1 ) return -1;
    if( ( unsigned long long )b & 1 ) return -1;
    aligned_a = ( ( unsigned long long )a & ( sizeof( __m128i ) - 1 ) );
    aligned_b = ( ( unsigned long long )b & ( sizeof( __m128i ) - 1 ) );
    if( aligned_a != aligned_b ) return -1; /* both has to be unaligned on the same boundary or aligned */
    if( aligned_a )
    {
        while( len &&
            ( ( unsigned long long )a & ( sizeof( __m128i ) - 1 ) ) )
        {
            if( *a++ != *b++ ) return -1;
            --len;
        }
    }
    if( !len ) return 0;
    while( len && !( len & 7 ) )
    {
        __m128i x = _mm_load_si128( ( __m128i* )&a[ i ] );
        __m128i y = _mm_load_si128( ( __m128i* )&b[ i ] );
        /*
        * _mm_cmpeq_epi16 returns 0xffff for each of the 8 half words when it matches
        */
        __m128i cmp = _mm_cmpeq_epi16( x, y );
        /*
        * _mm_movemask_epi8 creates a 16 bit mask with the MSB for each of the 16 bytes of cmp
        */
        if( ( uint16_t )_mm_movemask_epi8( cmp ) != 0xffffU ) return -1;
        len -= 8;
        i += 8;
    }
    return __sse_memcmp_tail( &a[ i ], &b[ i ], len );
}

static bool PushConstantsMemcmp2( const void* pPtr1, const void* pPtr2, size_t num )
{
    
    const uint32_t count = static_cast< uint32_t >( num / sizeof( __m128i ) );
    for( uint32_t i = 0; i < count; i+=1 )
    {
        __m128i x = _mm_load_si128( ( __m128i* )( pPtr1 ) );
        __m128i y = _mm_load_si128( ( __m128i* )( pPtr2 ) );
        __m128i cmp = _mm_cmpeq_epi32( x, y );
        if( _mm_movemask_epi8( cmp ) != 0xffff ) return i;
    }
    return true;
}

static bool PushConstantsMemcmp3( const void* pPtr1, const void* pPtr2, size_t num )
{
    const uint64_t* pFirstU64 = reinterpret_cast<const uint64_t*>( pPtr1 );
    const uint64_t* pSecondU64 = reinterpret_cast<const uint64_t*>( pPtr2 );
    if( pFirstU64[ 0 ] != pSecondU64[ 0 ] )
    {
        return false;
    }
    size_t curr = 0;
    for( uint32_t i = 1; curr < num; i += 2, curr += sizeof( uint64_t ) )
    {
        if( pFirstU64[ i ] != pSecondU64[ i ] || pFirstU64[ i + 1 ] != pSecondU64[ i + 1 ] )
        {
            return false;
        }
    }
    const uint32_t* pFirstU32 = reinterpret_cast<const uint32_t*>( pFirstU64 );
    const uint32_t* pSecondU32 = reinterpret_cast<const uint32_t*>( pSecondU64 );
    for( uint32_t i = 1; curr < num; i += 2, curr += sizeof( uint32_t ) )
    {
        if( pFirstU32[ i ] != pSecondU32[ i ] || pFirstU32[ i + 1 ] != pSecondU32[ i + 1 ] )
        {
            return false;
        }
    }
    return true;
}

int my_memcmp( const void* src_1, const void* src_2, size_t size )
{
    const __m256i* src1 = ( const __m256i* )src_1;
    const __m256i* src2 = ( const __m256i* )src_2;
    const size_t n = size / 64u;

    for( size_t i = 0; i < n; ++i, src1 += 2, src2 += 2 )
    {
        __m256i mm11 = _mm256_lddqu_si256( src1 );
        __m256i mm12 = _mm256_lddqu_si256( src1 + 1 );
        __m256i mm21 = _mm256_lddqu_si256( src2 );
        __m256i mm22 = _mm256_lddqu_si256( src2 + 1 );

        __m256i mm1 = _mm256_xor_si256( mm11, mm21 );
        __m256i mm2 = _mm256_xor_si256( mm12, mm22 );

        __m256i mm = _mm256_or_si256( mm1, mm2 );

        if( ( !_mm256_testz_si256( mm, mm ) ) )
        {
            // Find out which of the two 32-byte blocks are different
            if( _mm256_testz_si256( mm1, mm1 ) )
            {
                mm11 = mm12;
                mm21 = mm22;
                mm1 = mm2;
            }

            // Produce the comparison result
            __m256i mm_cmp = _mm256_cmpgt_epi8( mm21, mm11 );
            __m256i mm_rcmp = _mm256_cmpgt_epi8( mm11, mm21 );

            mm_cmp = _mm256_xor_si256( mm1, mm_cmp );
            mm_rcmp = _mm256_xor_si256( mm1, mm_rcmp );

            uint32_t cmp = _mm256_movemask_epi8( mm_cmp );
            uint32_t rcmp = _mm256_movemask_epi8( mm_rcmp );

            cmp = ( cmp - 1u ) ^ cmp;
            rcmp = ( rcmp - 1u ) ^ rcmp;

            return ( int32_t )rcmp - ( int32_t )cmp;
        }
    }

    // Compare tail bytes, if needed

    return 0;
}

//__forceinline
int __fastcall PushConstantsMemcmp4_4( const void* pBuff1, const void* pBuff2)
{
    const __m256i* pSrc1 = reinterpret_cast< const __m256i* >( pBuff1 );
    const __m256i* pSrc2 = reinterpret_cast< const __m256i* >( pBuff2 );
    __m256i buff1[ 4 ];
    __m256i buff2[ 4 ];
    __m256i tmp1[ 4 ];
    __m256i tmp2[ 4 ];

    buff1[ 0 ] = _mm256_lddqu_si256( pSrc1 + 0 );
    buff1[ 1 ] = _mm256_lddqu_si256( pSrc1 + 1 );
    buff1[ 2 ] = _mm256_lddqu_si256( pSrc1 + 2 );
    buff1[ 3 ] = _mm256_lddqu_si256( pSrc1 + 3 );

    buff2[ 0 ] = _mm256_lddqu_si256( pSrc2 + 0 );
    buff2[ 1 ] = _mm256_lddqu_si256( pSrc2 + 1 );
    buff2[ 2 ] = _mm256_lddqu_si256( pSrc2 + 2 );
    buff2[ 3 ] = _mm256_lddqu_si256( pSrc2 + 3 );

    tmp1[ 0 ] = _mm256_xor_si256( buff1[ 0 ], buff2[ 0 ] );
    tmp1[ 1 ] = _mm256_xor_si256( buff1[ 1 ], buff2[ 1 ] );
    tmp1[ 2 ] = _mm256_xor_si256( buff1[ 2 ], buff2[ 2 ] );
    tmp1[ 3 ] = _mm256_xor_si256( buff1[ 3 ], buff2[ 3 ] );

    tmp2[ 0 ] = _mm256_or_si256( tmp1[ 0 ], tmp1[ 1 ] );
    tmp2[ 1 ] = _mm256_or_si256( tmp1[ 2 ], tmp1[ 3 ] );

    int r1 = _mm256_testz_si256( tmp2[ 0 ], tmp2[ 0 ] );
    int r2 = _mm256_testz_si256( tmp2[ 1 ], tmp2[ 1 ] );

    return r1 && r2;
}

//__forceinline
int __fastcall PushConstantsMemcmp4(const void* pBuff1, const void* pBuff2, const size_t size)
{
    const uint32_t count32 = static_cast< uint32_t >( size / sizeof( __m256i ) );
    switch( count32 )
    {
        case 4:
        return PushConstantsMemcmp4_4( pBuff1, pBuff2 );
    }
    return true;
}

struct STest : public VKE::Core::CObject
{
    struct ST
    {
        VKE::Utils::TCDynamicArray< vke_string > vStrs;
    };
    ST s;

    STest()
    {
        s.vStrs.PushBack("a");
    }
};


void Test()
{
    /*VKE::Utils::TCString<> str1, str2;
    str1 = "abc";
    str1 += "_def";
    str1 = "ergd";
    str1.Copy(&str2);*/
    VKE::Utils::TCDynamicArray< int > vA = { 1,2,3,4,5 }, vB = {11,12,13,14};
    vA.Insert(1, 0, 3, vB.GetData());
    vB.Clear();
}

bool Main()
{
    Test();

    VKE::CVkEngine* pEngine = VKECreate();
    VKE::SWindowDesc WndInfos[2];
    WndInfos[ 0 ].mode = VKE::WindowModes::WINDOW;
    WndInfos[0].vSync = false;
    WndInfos[0].hWnd = 0;
    WndInfos[0].pTitle = "main";
    WndInfos[0].Size = { 800, 600 };

    WndInfos[ 1 ].mode = VKE::WindowModes::WINDOW;
    WndInfos[1].vSync = false;
    WndInfos[1].hWnd = 0;
    WndInfos[1].pTitle = "main2";
    WndInfos[1].Size = { 800, 600 };

    VKE::SRenderSystemDesc RenderInfo;

    VKE::SEngineInfo EngineInfo;
    EngineInfo.thread.threadCount = VKE::Constants::Threads::COUNT_OPTIMAL;
    EngineInfo.thread.taskMemSize = 1024; // 1kb per task
    EngineInfo.thread.maxTaskCount = 1024;
    
    auto err = pEngine->Init(EngineInfo);
    if(VKE_FAILED(err))
    {
        VKEDestroy(&pEngine);
        return false;
    }

    auto pWnd1 = pEngine->CreateRenderWindow(WndInfos[ 0 ]);
    auto pWnd2 = pEngine->CreateRenderWindow(WndInfos[ 1 ]);

    VKE::RenderSystem::SRenderSystemDesc RenderSysDesc;
    
    auto pRenderSys = pEngine->CreateRenderSystem( RenderSysDesc );
    if (!pRenderSys)
    {
        return false;
    }
    const auto& vAdapters = pRenderSys->GetAdapters();
    const auto& Adapter = vAdapters[ 0 ];

    // Run on first device only
    VKE::RenderSystem::SDeviceContextDesc DevCtxDesc1, DevCtxDesc2;
    DevCtxDesc1.pAdapterInfo = &Adapter;
    auto pDevCtx = pRenderSys->CreateDeviceContext(DevCtxDesc1);

    VKE::RenderSystem::CGraphicsContext* pGraphicsCtx1, *pGraphicsCtx2;
    {
        VKE::RenderSystem::SGraphicsContextDesc GraphicsDesc;
        GraphicsDesc.SwapChainDesc.pWindow = pWnd1;
        pGraphicsCtx1 = pDevCtx->CreateGraphicsContext(GraphicsDesc);
    }
    {
        VKE::RenderSystem::SGraphicsContextDesc GraphicsDesc;
        GraphicsDesc.SwapChainDesc.pWindow = pWnd2;
        pGraphicsCtx2 = pDevCtx->CreateGraphicsContext(GraphicsDesc);
    }
    {
        VKE::RenderSystem::SShaderCreateDesc ShaderDesc;
        ShaderDesc.Create.async = false;
        ShaderDesc.Shader.Base.pFileName = "data\\shaders\\test.vs";
        ShaderDesc.Shader.type = VKE::RenderSystem::ShaderTypes::VERTEX;

        VKE::RenderSystem::SBufferCreateDesc VBDesc;

        auto pVertexShader = pDevCtx->CreateShader( ShaderDesc );
        auto pVertexBuffer = pDevCtx->CreateBuffer( VBDesc );

        pGraphicsCtx1->SetShader( pVertexShader );
    }
    
    pWnd1->IsVisible(true);
    pWnd2->IsVisible(true);

    pEngine->StartRendering();

    VKEDestroy(&pEngine);

    return true;
}

int main()
{
    Main();
    //VKE::Platform::DumpMemoryLeaks();
    return 0;
}