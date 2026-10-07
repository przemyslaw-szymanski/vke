#include "Core/Platform/CPlatform.h"
#include "Core/Utils/CLogger.h"

#if VKE_LINUX

#include <errno.h>
#include <cstdlib>
#include <fstream>
#include <set>
#include <string>
#include <string_view>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <unistd.h>

namespace VKE::Platform
{
    struct SCPUInfo
    {
        uint32_t logicalProcessorCount = 0;
        uint32_t physicalCoreCount     = 0;
        uint32_t packageCount          = 0;
        uint32_t cacheSize             = 0;

        static SCPUInfo Load()
        {
            SCPUInfo cpuInfo;

            std::ifstream file( "/proc/cpuinfo" );
            if( !file.is_open() )
            {
                return cpuInfo;
            }

            // Unique (physical id, core id) pairs give physical cores across all packages.
            std::set< std::pair< int32_t, int32_t > > cores;
            std::set< int32_t >                       packages;
            int32_t                                   physicalId = -1;
            int32_t                                   coreId     = -1;

            auto Flush = [ & ]() {
                if( coreId >= 0 )
                {
                    cores.emplace( physicalId, coreId );
                }
                if( physicalId >= 0 )
                {
                    packages.insert( physicalId );
                }
                physicalId = -1;
                coreId     = -1;
            };

            std::string line;
            while( std::getline( file, line ) )
            {
                const auto colon = line.find( ':' );
                if( colon == std::string::npos )
                {
                    // Empty line separates processor blocks
                    Flush();
                    continue;
                }

                std::string_view key( line.data(), colon );
                while( !key.empty() && ( key.back() == ' ' || key.back() == '\t' ) )
                {
                    key.remove_suffix( 1 );
                }
                const char* pValue = line.c_str() + colon + 1;

                if( key == "processor" )
                {
                    ++cpuInfo.logicalProcessorCount;
                }
                else if( key == "physical id" )
                {
                    physicalId = static_cast< int32_t >( std::strtol( pValue, nullptr, 10 ) );
                }
                else if( key == "core id" )
                {
                    coreId = static_cast< int32_t >( std::strtol( pValue, nullptr, 10 ) );
                }
                else if( key == "cache size" && cpuInfo.cacheSize == 0 )
                {
                    // Format: "cache size : 32768 KB"
                    cpuInfo.cacheSize = static_cast< uint32_t >( std::strtoul( pValue, nullptr, 10 ) ) * 1024u;
                }
            }
            Flush();

            cpuInfo.packageCount = static_cast< uint32_t >( packages.size() );
            // ARM and some VMs don't expose "core id"; assume no SMT then.
            cpuInfo.physicalCoreCount =
                cores.empty() ? cpuInfo.logicalProcessorCount : static_cast< uint32_t >( cores.size() );

            return cpuInfo;
        }
    };

    static uint32_t GetSysconfValue( int name )
    {
        const long value = sysconf( name );
        return value > 0 ? static_cast< uint32_t >( value ) : 0u;
    }

    void GetErrorMessage( int errorCode, char* pBuffer, size_t bufferSize )
    {
        const char*  systemError = std::strerror( errorCode );
        const size_t size        = std::min( std::strlen( systemError ), bufferSize );
        memcpy( pBuffer, systemError, size );
    }

    void LogError( cstr_t pText = "" )
    {
        char pBuffer[ 2048 ];
        GetErrorMessage( errno, &pBuffer[ 0 ], sizeof( pBuffer ) );
        VKE_LOG_ERR( "System Error: " << pBuffer << "\t" << pText );
    }

    const SProcessorInfo& GetProcessorInfo()
    {
        static SProcessorInfo sInfo;

        if( sInfo.count == 0 )
        {
            struct utsname name{};

            if( uname( &name ) == 0 )
            {
                const std::string_view machine = name.machine;
                if( machine == "x86_64" || machine == "amd64" )
                {
                    sInfo.architecture = Architectures::X64;
                }
                else if( machine.size() == 4 && machine[ 0 ] == 'i' && machine.ends_with( "86" ) )
                {
                    // i386, i486, i586, i686
                    sInfo.architecture = Architectures::X86;
                }
                else if( machine.starts_with( "aarch64" ) || machine.starts_with( "arm64" ) ||
                         machine.starts_with( "armv8" ) )
                {
                    sInfo.architecture = Architectures::ARM64;
                }
                else if( machine.starts_with( "arm" ) )
                {
                    // armv6l, armv7l, ...
                    sInfo.architecture = Architectures::ARM32;
                }
                else
                {
                    sInfo.architecture = Architectures::UNKNOWN;
                }
            }
            else
            {
                LogError( "uname failed" );
            }

            const uint32_t logicalCount = GetSysconfValue( _SC_NPROCESSORS_ONLN );
            const SCPUInfo cpuInfo      = SCPUInfo::Load();

            sInfo.logicalCoreCount =
                static_cast< uint16_t >( logicalCount > 0 ? logicalCount : cpuInfo.logicalProcessorCount );
            sInfo.coreCount = static_cast< uint16_t >( cpuInfo.physicalCoreCount > 0 ? cpuInfo.physicalCoreCount
                                                                                     : sInfo.logicalCoreCount );

            // glibc extensions; may return 0 (e.g. on ARM or in some VMs)
            sInfo.l1CacheSize = GetSysconfValue( _SC_LEVEL1_DCACHE_SIZE );
            sInfo.l2CacheSize = GetSysconfValue( _SC_LEVEL2_CACHE_SIZE );
            sInfo.l3CacheSize = GetSysconfValue( _SC_LEVEL3_CACHE_SIZE );
            if( sInfo.l3CacheSize == 0 && sInfo.l2CacheSize == 0 )
            {
                // /proc/cpuinfo only reports the last level cache
                sInfo.l3CacheSize = cpuInfo.cacheSize;
            }

            // Same meaning as on Windows (dwNumberOfProcessors): logical processors.
            // Must be non-zero as it marks the info as initialized.
            sInfo.count = sInfo.logicalCoreCount > 0 ? sInfo.logicalCoreCount : 1;
        }

        return sInfo;
    }

    cstr_t GetCmdLine()
    {
        cstr_t pCmdLine = "";
        return pCmdLine;
    };

    void Debug::BeginDumpMemoryLeaks()
    {
    }

    void Debug::EndDumpMemoryLeaks()
    {
    }

    void Debug::BreakAtAllocation( uint32_t idx )
    {
    }

    void Debug::CMemoryLeakDetector::Start( cstr_t pName )
    {
    }

    bool Debug::CMemoryLeakDetector::End()
    {
        return true;
    }

    void Time::Sleep( uint32_t us )
    {
    }

    void Debug::PrintOutput( const cstr_t msg )
    {
    }

    void Debug::PrintStallstack()
    {
    }

    void Debug::ConvertErrorCodeToText( uint32_t err, char* pBuffOut, uint32_t buffSize )
    {
    }

    handle_t DynamicLibrary::Load( const cstr_t name )
    {
        return 0;
    }

    void DynamicLibrary::Close( const handle_t& handle )
    {
    }

    void* DynamicLibrary::GetProcAddress( const handle_t& handle, const void* pSymbol )
    {
        return nullptr;
    }

    Time::TimePoint Time::GetHighResClockFrequency()
    {
        Time::TimePoint highResClockFrequency = 0;
        return highResClockFrequency;
    }

    Time::TimePoint Time::GetHighResClockTimePoint()
    {
        Time::TimePoint highResClockTimePoint = 0;
        return highResClockTimePoint;
    }

    double Time::TimePointToMicroseconds( TimePoint ticks, TimePoint freq )
    {
        double delta = 0.0;
        return delta;
    }

    bool File::Exists( cstr_t pFileName )
    {
        return true;
    }

    bool File::IsDirectory( cstr_t pFileName )
    {
        return true;
    }

    uint32_t File::GetSize( cstr_t pFileName )
    {
        return 0;
    }

    uint32_t File::GetSize( handle_t hFile )
    {
        return 0;
    }

    uint32_t File::GetDirectory( cstr_t pFileName, uint32_t fileNameSize, char** ppOut )
    {
        return 0;
    }

    bool File::GetWorkingDirectory( const uint32_t bufferSize, char** ppOut )
    {
        return true;
    }

    handle_t File::Create( cstr_t pFileName, MODE mode )
    {
        return 0;
    }

    bool File::CreateDir( cstr_t pDirPath )
    {
        return true;
    }

    bool File::IsRelativePath( cstr_t pPath )
    {
        return true;
    }

    bool File::IsAbsolutePath( cstr_t pPath )
    {
        return true;
    }

    handle_t File::Open( cstr_t pFileName, MODE mode )
    {
        return 0;
    }

    void File::Close( handle_t* phFile )
    {
    }

    void File::Flush( handle_t hFile )
    {
    }

    void File::FlushSync( handle_t hFile )
    {
    }

    bool File::Seek( handle_t hFile, uint32_t offset, SEEK_MODE mode )
    {
        return true;
    }

    uint32_t File::Read( handle_t hFile, SReadData* pData )
    {
        return 0;
    }

    uint32_t File::Write( handle_t hFile, const SWriteInfo& Info )
    {
        return 0;
    }

    cstr_t File::GetExtension( cstr_t pFileName )
    {
        return nullptr;
    }

    cstr_t File::GetExtension( handle_t /*hFile*/ )
    {
        return nullptr;
    }

    bool File::GetFileName( cstr_t pFilePath, bool includeExtension, char** ppOut )
    {
        return true;
    }

    Thread::ID Thread::GetID()
    {
        return 0;
    }

    Thread::ID Thread::GetID( const handle_t& hThread )
    {
        return 0;
    }

    Thread::ID Thread::GetID( void* pHandle )
    {
        return 0;
    }

    void Thread::Sleep( uint32_t us )
    {
    }

    void Thread::Pause()
    {
    }

    uint32_t Thread::GetMaxConcurrentThreadCount()
    {
        return 0;
    }

    void Thread::CSpinlock::Lock()
    {
    }

    void Thread::CSpinlock::Unlock()
    {
    }

    bool Thread::CSpinlock::TryLock()
    {
        return true;
    }

    void Thread::SetDesc( cstr_t pText )
    {
    }

    bool Thread::Wait( const ThreadFence& hFence, uint32_t value, Time::TimePoint timeout )
    {
        return true;
    }
}; // namespace VKE::Platform

#endif