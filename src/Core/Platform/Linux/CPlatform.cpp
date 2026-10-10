#include "Core/Platform/CPlatform.h"
#include "Core/Utils/CLogger.h"

#if VKE_LINUX

#include <errno.h>
#include <cxxabi.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <filesystem>
#include <string>
#include <string_view>
#include <fcntl.h>
#include <libgen.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

namespace VKE::Platform
{
    namespace Internal
    {
        // File descriptor cannot be simply handle_t as '0' is a valid handle descriptor.
        static inline constexpr int VKE_NATIVE_LINUX_BASE_OFFSET = 0x10FF;

        // CLOCK_MONOTONIC is reported in nanoseconds, so the frequency is fixed.
        static constexpr Time::TimePoint NANOSECONDS_PER_SECOND = 1000000000;

        // Linux thread names are limited to 16 bytes including the terminating '\0'.
        static constexpr size_t THREAD_NAME_MAX_LENGTH = 15;

        static int HandleToFileDescriptor( handle_t hHandle )
        {
            return static_cast< int >( hHandle ) - Internal::VKE_NATIVE_LINUX_BASE_OFFSET;
        }

        static handle_t FileDescriptorToHandle( int fileDescriptor )
        {
            return static_cast< handle_t >( fileDescriptor + Internal::VKE_NATIVE_LINUX_BASE_OFFSET );
        }

        struct SCPUInfo
        {
            uint32_t logicalProcessorCount = 0;
            uint32_t physicalCoreCount     = 0;
            uint32_t packageCount          = 0;
            uint32_t cacheSize             = 0;
        };

        static std::string ReadProcFile( cstr_t pFileName )
        {
            std::string str;

            handle_t hFile = File::Open( pFileName, File::Modes::READ );
            if( hFile == INVALID_HANDLE )
            {
                return str;
            }

            char            aChunk[ 4096 ];
            File::SReadData readInfo{};
            readInfo.pData         = aChunk;
            readInfo.readByteCount = sizeof( aChunk );

            uint32_t bytesRead = 0;
            while( ( bytesRead = File::Read( hFile, &readInfo ) ) > 0 )
            {
                str.append( aChunk, bytesRead );
            }

            File::Close( &hFile );
            return str;
        }

        SCPUInfo LoadCPUInfo()
        {
            SCPUInfo cpuInfo;

            const std::string content = ReadProcFile( "/proc/cpuinfo" );
            if( content.empty() )
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

            size_t lineBegin = 0;
            while( lineBegin < content.size() )
            {
                size_t lineEnd = content.find( '\n', lineBegin );
                if( lineEnd == std::string::npos )
                {
                    lineEnd = content.size();
                }

                const std::string_view line( content.data() + lineBegin, lineEnd - lineBegin );
                lineBegin = lineEnd + 1;

                const auto colon = line.find( ':' );
                if( colon == std::string_view::npos )
                {
                    // Empty line separates processor blocks
                    Flush();
                    continue;
                }

                std::string_view key = line.substr( 0, colon );
                while( !key.empty() && ( key.back() == ' ' || key.back() == '\t' ) )
                {
                    key.remove_suffix( 1 );
                }
                // strtol/strtoul stop at '\n', and content is '\0'-terminated, so this is safe.
                const char* pValue = line.data() + colon + 1;

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

        static uint32_t GetSysconfValue( int name )
        {
            const long value = sysconf( name );
            return value > 0 ? static_cast< uint32_t >( value ) : 0u;
        }

        static std::string ReadCmdLine()
        {
            std::string str = Internal::ReadProcFile( "/proc/self/cmdline" );

            // Arguments are '\0'-separated (with a trailing '\0'); join with spaces.
            while( !str.empty() && str.back() == '\0' )
            {
                str.pop_back();
            }

            std::replace( str.begin(), str.end(), '\0', ' ' );
            return str;
        }

    } // namespace Internal

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

            const uint32_t           logicalCount = Internal::GetSysconfValue( _SC_NPROCESSORS_ONLN );
            const Internal::SCPUInfo cpuInfo      = Internal::LoadCPUInfo();

            sInfo.logicalCoreCount =
                static_cast< uint16_t >( logicalCount > 0 ? logicalCount : cpuInfo.logicalProcessorCount );
            sInfo.coreCount = static_cast< uint16_t >( cpuInfo.physicalCoreCount > 0 ? cpuInfo.physicalCoreCount
                                                                                     : sInfo.logicalCoreCount );

            // glibc extensions; may return 0 (e.g. on ARM or in some VMs)
            sInfo.l1CacheSize = Internal::GetSysconfValue( _SC_LEVEL1_DCACHE_SIZE );
            sInfo.l2CacheSize = Internal::GetSysconfValue( _SC_LEVEL2_CACHE_SIZE );
            sInfo.l3CacheSize = Internal::GetSysconfValue( _SC_LEVEL3_CACHE_SIZE );

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
        // Reads once, cached
        static const std::string sCommandLine = Internal::ReadCmdLine();
        return sCommandLine.c_str();
    }

    void Debug::BeginDumpMemoryLeaks()
    {
        // No implementation. The modern standard is to use LeakSanitizer (LSAN from GCC).
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
        std::this_thread::sleep_for( std::chrono::microseconds( us ) );
    }

    void Debug::PrintOutput( const cstr_t msg )
    {
        ::fputs( msg, stderr );
    }

    void Debug::PrintStallstack()
    {
        static constexpr int MAX_FRAMES = 64;
        void*                aFrames[ MAX_FRAMES ];

        const int frameCount = ::backtrace( aFrames, MAX_FRAMES );

        VKE_LOG( "CALLSTACK" );

        // Start at 1 to skip PrintStallstack itself.
        for( int i = 1; i < frameCount; ++i )
        {
            Dl_info     info{};
            const char* pModule    = "??";
            const char* pSymbol    = "??";
            char*       pDemangled = nullptr;
            uintptr_t   symOffset  = 0;
            uintptr_t   modOffset  = 0;

            if( ::dladdr( aFrames[ i ], &info ) != 0 )
            {
                if( info.dli_fname != nullptr )
                {
                    pModule = info.dli_fname;
                    modOffset =
                        reinterpret_cast< uintptr_t >( aFrames[ i ] ) - reinterpret_cast< uintptr_t >( info.dli_fbase );
                }
                if( info.dli_sname != nullptr )
                {
                    int status = 0;
                    pDemangled = abi::__cxa_demangle( info.dli_sname, nullptr, nullptr, &status );
                    pSymbol    = ( status == 0 && pDemangled != nullptr ) ? pDemangled : info.dli_sname;
                    symOffset =
                        reinterpret_cast< uintptr_t >( aFrames[ i ] ) - reinterpret_cast< uintptr_t >( info.dli_saddr );
                }
            }

            // Module offset can be resolved to file:line with: addr2line -C -f -e <module> <offset>
            VKE_LOG( "#" << i << " " << pSymbol << " +0x" << std::hex << symOffset << " (" << pModule << " +0x"
                         << modOffset << ")" << std::dec );

            ::free( pDemangled );
        }
    }

    void Debug::ConvertErrorCodeToText( uint32_t err, char* pBuffOut, uint32_t buffSize )
    {
        GetErrorMessage( static_cast< int >( err ), pBuffOut, buffSize );
    }

    handle_t DynamicLibrary::Load( const cstr_t name )
    {
        handle_t hLib    = INVALID_HANDLE;
        void*    pNative = dlopen( name, RTLD_NOW | RTLD_LOCAL );

        if( pNative )
        {
            hLib = reinterpret_cast< handle_t >( pNative );
        }
        else
        {
            VKE_LOG_ERR( "dlopen() failed with error: " << dlerror() );
        }

        return hLib;
    }

    void DynamicLibrary::Close( const handle_t& handle )
    {
        if( handle && handle != INVALID_HANDLE )
        {
            dlclose( reinterpret_cast< void* >( handle ) );
        }
    }

    void* DynamicLibrary::GetProcAddress( const handle_t& handle, const void* pSymbol )
    {
        void* pNative = reinterpret_cast< void* >( handle );

        dlerror(); // Clears error prior to dlsym
        void* pDlSym = dlsym( pNative, static_cast< const char* >( pSymbol ) );

        const char* dlsym_error = dlerror();
        if( dlsym_error )
        {
            VKE_LOG_ERR( "dlsym() failed with error: " << dlsym_error );
        }

        return pDlSym;
    }

    Time::TimePoint Time::GetHighResClockFrequency()
    {
        return Internal::NANOSECONDS_PER_SECOND;
    }

    Time::TimePoint Time::GetHighResClockTimePoint()
    {
        Time::TimePoint timePoint = 0;
        struct timespec ts{};
        if( ::clock_gettime( CLOCK_MONOTONIC, &ts ) == 0 )
        {
            timePoint = static_cast< Time::TimePoint >( ts.tv_sec ) * Internal::NANOSECONDS_PER_SECOND +
                        static_cast< Time::TimePoint >( ts.tv_nsec );
        }
        return timePoint;
    }

    double Time::TimePointToMicroseconds( TimePoint ticks, TimePoint freq )
    {
        double t = static_cast< double >( ticks ) * 1000000;
        return t / static_cast< double >( freq );
    }

    bool File::Exists( cstr_t pFileName )
    {
        struct stat buffer;
        return ( stat( pFileName, &buffer ) == 0 );
    }

    bool File::IsDirectory( cstr_t pFileName )
    {
        struct stat buffer;
        bool        isDirectory = false;
        if( fstatat( AT_FDCWD, pFileName, &buffer, 0 ) == 0 )
        {
            isDirectory = S_ISDIR( buffer.st_mode );
        }
        return isDirectory;
    }

    uint32_t File::GetSize( cstr_t pFileName )
    {
        struct stat buffer;
        uint32_t    fileSize = 0;

        if( fstatat( AT_FDCWD, pFileName, &buffer, 0 ) == 0 )
        {
            fileSize = (uint32_t)buffer.st_size;
        }

        return fileSize;
    }

    uint32_t File::GetSize( handle_t hFile )
    {
        struct stat buffer;
        uint32_t    fileSize = 0;

        if( fstat( Internal::HandleToFileDescriptor( hFile ), &buffer ) == 0 )
        {
            fileSize = (uint32_t)buffer.st_size;
        }

        return fileSize;
    }

    uint32_t File::GetDirectory( cstr_t pFileName, uint32_t fileNameSize, char** ppOut )
    {
        static constexpr size_t MAX_PATH_SIZE = 256;

        // We need to provide local buffer on stack for dirname() because it modifies input buffer. It is intended
        // non-static just to be thread-safe.
        alignas( 32 ) char tmpBuffer[ MAX_PATH_SIZE + 1 ];

        assert( ppOut && *ppOut );
        uint32_t directorySize = 0;

        if( pFileName && fileNameSize > 0 && fileNameSize <= MAX_PATH_SIZE )
        {
            size_t actualLen = strnlen( pFileName, fileNameSize );
            if( actualLen > 0 )
            {
                char* pOut = *ppOut;
                Memory::Copy( tmpBuffer, pFileName, actualLen );
                tmpBuffer[ actualLen ] = '\0';

                char* pDirname = dirname( tmpBuffer );
                directorySize  = static_cast< uint32_t >( strlen( pDirname ) );

                Memory::Copy( pOut, pDirname, directorySize );

                pOut[ directorySize ] = '\0';
            }
        }

        return directorySize;
    }

    bool File::GetWorkingDirectory( const uint32_t bufferSize, char** ppOut )
    {
        assert( ppOut && *ppOut );
        bool res = true;

        if( !ppOut || !*ppOut || bufferSize == 0 )
        {
            res = false;
        }
        else if( !getcwd( *ppOut, static_cast< size_t >( bufferSize ) ) )
        {
            res = false;
        }

        return res;
    }

    handle_t _FileOpen( cstr_t pFileName, File::MODE mode, int extraFlags = 0 )
    {
        handle_t hFile = INVALID_HANDLE;

        const bool read   = ( mode & File::Modes::READ ) != 0;
        const bool append = ( mode & File::Modes::APPEND ) != 0;
        const bool write  = ( mode & File::Modes::WRITE ) != 0 || append;

        int flags = O_CLOEXEC;

        if( read && write )
        {
            flags |= O_RDWR | O_CREAT;
        }
        else if( write )
        {
            flags |= O_WRONLY | O_CREAT;
        }
        else
        {
            flags |= O_RDONLY;
        }

        if( append )
        {
            flags |= O_APPEND;
        }

        const int nativeFile = ::open( pFileName, flags | extraFlags, 0644 );

        if( nativeFile < 0 )
        {
            LogError( pFileName );
        }
        else
        {
            hFile = Internal::FileDescriptorToHandle( nativeFile );
        }

        return hFile;
    }

    handle_t File::Create( cstr_t pFileName, MODE mode )
    {
        return _FileOpen( pFileName, mode, O_CREAT );
    }

    bool File::CreateDir( cstr_t pDirPath )
    {
        static constexpr size_t ALIGNED_PATH_SIZE = 512;

        if( !pDirPath || pDirPath[ 0 ] == '\0' )
        {
            return false;
        }

        size_t pathLen = strnlen( pDirPath, ALIGNED_PATH_SIZE );
        if( pathLen == 0 || pathLen >= ALIGNED_PATH_SIZE )
        {
            return false;
        }

        alignas( 32 ) char folderBuffer[ ALIGNED_PATH_SIZE ];

        Memory::Copy( folderBuffer, pDirPath, pathLen );
        folderBuffer[ pathLen ] = '\0';

        for( size_t i = 0; i < pathLen; ++i )
        {
            if( folderBuffer[ i ] == '\\' )
            {
                folderBuffer[ i ] = '/';
            }
        }

        bool  ret  = true;
        char* pEnd = strchr( folderBuffer, '/' );

        if( pEnd == folderBuffer )
        {
            pEnd = strchr( pEnd + 1, '/' );
        }

        while( pEnd != nullptr )
        {
            char originalChar = *pEnd;
            *pEnd             = '\0';

            if( mkdir( folderBuffer, 0777 ) != 0 )
            {
                if( errno != EEXIST )
                {
                    LogError();
                    ret   = false;
                    *pEnd = originalChar;
                    break;
                }
            }

            *pEnd = originalChar;
            pEnd  = strchr( pEnd + 1, '/' );
        }

        if( ret )
        {
            if( mkdir( folderBuffer, 0777 ) != 0 )
            {
                if( errno != EEXIST )
                {
                    LogError();
                    ret = false;
                }
            }
        }

        return ret;
    }

    bool File::IsRelativePath( cstr_t pPath )
    {
        std::filesystem::path Path( pPath );
        return Path.is_relative();
    }

    bool File::IsAbsolutePath( cstr_t pPath )
    {
        std::filesystem::path Path( pPath );
        return Path.is_absolute();
    }

    handle_t File::Open( cstr_t pFileName, MODE mode )
    {
        return _FileOpen( pFileName, mode );
    }

    void File::Close( handle_t* phFile )
    {
        if( ::close( Internal::HandleToFileDescriptor( *phFile ) ) != 0 )
        {
            LogError( "::close failed" );
        }
    }

    void File::Flush( handle_t hFile )
    {
        // ::write handles flush to OS buffer
        (void)hFile;
    }

    void File::FlushSync( handle_t hFile )
    {
        if( ::fsync( Internal::HandleToFileDescriptor( hFile ) ) != 0 )
        {
            LogError( "::fsync failed" );
        }
    }

    bool File::Seek( handle_t hFile, uint32_t offset, SEEK_MODE mode )
    {
        static const int ascMap[ SEEK_MODE::_MAX_COUNT ] = { SEEK_SET, SEEK_CUR, SEEK_END };

        bool res = true;
        if( lseek( static_cast< int >( hFile ) - 1, (off_t)offset, ascMap[ mode ] ) == (off_t)-1 )
        {
            LogError( "::lseek failed" );
            res = false;
        }

        return res;
    }

    uint32_t File::Read( handle_t hFile, SReadData* pData )
    {
        int hNative = Internal::HandleToFileDescriptor( hFile );
        if( pData->offset )
        {
            Seek( hFile, pData->offset, SeekModes::BEGIN );
        }

        ssize_t bytesRead = ::read( hNative, pData->pData, pData->readByteCount );
        if( bytesRead < 0 )
        {
            LogError( "::read failed" );
            bytesRead = 0;
        }

        return static_cast< uint32_t >( bytesRead );
    }

    uint32_t File::Write( handle_t hFile, const SWriteInfo& Info )
    {
        int hNative = Internal::HandleToFileDescriptor( hFile );
        if( Info.offset )
        {
            Seek( hFile, Info.offset, SeekModes::BEGIN );
        }

        ssize_t bytesWrite = ::write( hNative, Info.pData, Info.dataSize );
        if( bytesWrite < 0 )
        {
            LogError( "::write failed" );
            bytesWrite = 0;
        }

        return static_cast< uint32_t >( bytesWrite );
    }

    cstr_t File::GetExtension( cstr_t pFileName )
    {
        cstr_t pExt = strrchr( pFileName, '.' );

        if( pExt )
        {
            return pExt + 1;
        }

        return pExt;
    }

    cstr_t File::GetExtension( handle_t /*hFile*/ )
    {
        assert( 0 && "not implemented" );
        return nullptr;
    }

    bool File::GetFileName( cstr_t pFilePath, bool includeExtension, char** ppOut )
    {
        bool ret = false;

        std::filesystem::path Path( pFilePath );
        if( Path.has_filename() )
        {
            if( includeExtension )
            {
                const auto& strFileName = Path.filename().string();
                Memory::Copy( *ppOut, strFileName.size(), strFileName.c_str(), strFileName.size() );
                ( *ppOut )[ strFileName.length() ] = 0;
            }
            else
            {
                const auto& strFileName = Path.stem().string();
                Memory::Copy( *ppOut, strFileName.size(), strFileName.c_str(), strFileName.size() );
                ( *ppOut )[ strFileName.length() ] = 0;
            }
            ret = true;
        }
        return ret;
    }

    Thread::ID Thread::GetID()
    {
        return static_cast< Thread::ID >( gettid() );
    }

    void Thread::Sleep( uint32_t us )
    {
        Time::Sleep( us );
    }

    void Thread::Pause()
    {
#if defined( __x86_64__ ) || defined( __i386__ )
        __builtin_ia32_pause();
#elif defined( __aarch64__ ) || defined( __arm__ )
        __asm__ __volatile__( "yield" ::: "memory" );
#else
        std::atomic_signal_fence( std::memory_order_seq_cst );
#endif
    }

    uint32_t Thread::GetMaxConcurrentThreadCount()
    {
        return GetProcessorInfo().logicalCoreCount;
    }

    void Thread::CSpinlock::Lock()
    {
        using ValueType = std::remove_volatile_t< AtomicType >;
        // GCC __atomic builtins reject volatile pointers in C++; atomicity comes from the builtins themselves.
        ValueType*      pThreadId = const_cast< ValueType* >( &m_threadId );
        const ValueType id        = Thread::GetID();
        if( __atomic_load_n( pThreadId, __ATOMIC_RELAXED ) == id )
        {
            ++m_lockCount;
        }
        else
        {
            ValueType expected;

            // Test-and-test-and-set: spin on a plain load to avoid cache-line ping-pong, CAS only when free.
            do
            {
                while( __atomic_load_n( pThreadId, __ATOMIC_RELAXED ) != UNKNOWN_THREAD_ID )
                {
                    Thread::Pause();
                }
                expected = UNKNOWN_THREAD_ID;
            }
            while(
                !__atomic_compare_exchange_n( pThreadId, &expected, id, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED ) );
            m_lockCount = 1;
        }
    }

    void Thread::CSpinlock::Unlock()
    {
        using ValueType      = std::remove_volatile_t< AtomicType >;
        ValueType* pThreadId = const_cast< ValueType* >( &m_threadId );
        if( --m_lockCount == 0 )
        {
            __atomic_store_n( pThreadId, static_cast< ValueType >( UNKNOWN_THREAD_ID ), __ATOMIC_RELEASE );
        }
    }

    bool Thread::CSpinlock::TryLock()
    {
        using ValueType           = std::remove_volatile_t< AtomicType >;
        ValueType*      pThreadId = const_cast< ValueType* >( &m_threadId );
        bool            locked    = false;
        const ValueType id        = Thread::GetID();
        if( __atomic_load_n( pThreadId, __ATOMIC_RELAXED ) == id )
        {
            ++m_lockCount;
            locked = true;
        }
        else
        {
            ValueType expected = UNKNOWN_THREAD_ID;
            locked = __atomic_compare_exchange_n( pThreadId, &expected, id, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED );
            if( locked )
            {
                m_lockCount = 1;
            }
        }
        return locked;
    }

    void Thread::SetDesc( cstr_t pText )
    {
        char aBuffer[ Internal::THREAD_NAME_MAX_LENGTH + 1 ];

        const size_t descLen = std::min( Internal::THREAD_NAME_MAX_LENGTH, std::strlen( pText ) );
        std::memcpy( aBuffer, pText, descLen );
        aBuffer[ descLen ] = '\0';

        // SetDesc always names the calling thread, so pthread_self() is the matching pthread handle.
        ::pthread_setname_np( ::pthread_self(), aBuffer );
    }

    bool Thread::Wait( const ThreadFence& hFence, uint32_t value, Time::TimePoint timeout )
    {
        bool timeoutReached = false;
        if( timeout == 0 )
        {
            timeoutReached = hFence.Load() <= value;
        }
        else
        {
            const auto            freq      = Time::GetHighResClockFrequency();
            const Time::TimePoint startTime = Time::GetHighResClockTimePoint();
            while( !timeoutReached && hFence.Load() != value )
            {
                const Time::TimePoint currTime = Time::GetHighResClockTimePoint();
                timeoutReached                 = Time::TimePointToMicroseconds( currTime - startTime, freq ) > timeout;
                Pause();
            }
        }
        return timeoutReached;
    }
}; // namespace VKE::Platform

#endif