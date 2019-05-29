#if VKE_USE_DIRECTX_MATH
namespace VKE
{
#define XMVEC(_vec) DirectX::XMLoadFloat3(&(_vec._Native))

    namespace Math
    {
        CVector3::CVector3( float f ) :
            _Native{ f, f, f }
        {
            
        }

        CVector3::CVector3(float x, float y, float z) :
            _Native{ x, y, z }
        {}

        CVector3::CVector3(const CVector3& Other) :
            _Native{ Other._Native }
        {}

        void CVector3::operator=( const float v )
        {
            DirectX::XMStoreFloat3( &_Native, DirectX::XMVectorSet( v, v, v, 0.0f ) );
        }

        bool CVector3::IsZero() const
        {
            return Equals( *this, ZERO );
        }

        bool CVector3::Equals( const CVector3& Left, const CVector3& Right )
        {
            return DirectX::XMVector3Equal( XMVEC( Left ), XMVEC( Right ) );
        }

        bool CVector3::Less( const CVector3& Left, const CVector3& Right )
        {
            return DirectX::XMVector3Less( XMVEC( Left ), XMVEC( Right ) );
        }

        bool CVector3::Greater( const CVector3& Left, const CVector3& Right )
        {
            return DirectX::XMVector3Greater( XMVEC( Left ), XMVEC( Right ) );
        }

        bool CVector3::LessEquals( const CVector3& Left, const CVector3& Right )
        {
            return DirectX::XMVector3LessOrEqual( XMVEC( Left ), XMVEC( Right ) );
        }

        bool CVector3::GreaterEquals( const CVector3& Left, const CVector3& Right )
        {
            return DirectX::XMVector3GreaterOrEqual( XMVEC( Left ), XMVEC( Right ) );
        }

        void CVector3::Add( const CVector3& Left, const CVector3& Right, CVector3* pOut )
        {
            DirectX::XMStoreFloat3( &pOut->_Native,
                DirectX::XMVectorAdd( XMVEC( Left ), XMVEC( Right ) ) );
        }

        void CVector3::Set( const float x, const float y, const float z, CVector3* pOut )
        {
            DirectX::XMStoreFloat3( &pOut->_Native, DirectX::XMVectorSet( x, y, z, 0.0f ) );
        }

        void CVector3::Set( const float v, CVector3* pOut )
        {
            Set( v, v, v, pOut );
        }

        CVector3 CVector3::Add( const CVector3& Left, const CVector3& Right )
        {
            CVector3 Tmp;
            Add( Left, Right, &Tmp );
            return Tmp;
        }

        void CVector3::Sub( const CVector3& Left, const CVector3& Right, CVector3* pOut )
        {
            DirectX::XMStoreFloat3( &pOut->_Native,
                DirectX::XMVectorSubtract( XMVEC( Left ), XMVEC( Right ) ) );
        }

        CVector3 CVector3::Sub( const CVector3& Left, const CVector3& Right )
        {
            CVector3 Tmp;
            Sub( Left, Right, &Tmp );
            return Tmp;
        }

        void CVector3::Mul( const CVector3& Left, const CVector3& Right, CVector3* pOut )
        {
            DirectX::XMStoreFloat3( &pOut->_Native,
                DirectX::XMVectorMultiply( XMVEC( Left ), XMVEC( Right ) ) );
        }

        CVector3 CVector3::Mul( const CVector3& Left, const CVector3& Right )
        {
            CVector3 Tmp;
            Mul( Left, Right, &Tmp );
            return Tmp;
        }

        void CVector3::Div( const CVector3& Left, const CVector3& Right, CVector3* pOut )
        {
            DirectX::XMStoreFloat3( &pOut->_Native,
                DirectX::XMVectorDivide( XMVEC( Left ), XMVEC( Right ) ) );
        }

        CVector3 CVector3::Div( const CVector3& Left, const CVector3& Right )
        {
            CVector3 Tmp;
            Div( Left, Right, &Tmp );
            return Tmp;
        }

    } // Math
} // VKE

// CVector4
namespace VKE
{
#define XMVEC4(_vec) ((_vec)._Native)
    namespace Math
    {
        CVector4::CVector4( float f ) :
            _Native{ f, f, f, f }
        {

        }

        CVector4::CVector4( float x, float y, float z, float w ) :
            _Native{ x, y, z, w }
        {}

        CVector4::CVector4( const CVector4& Other ) :
            _Native{ Other._Native }
        {}

        void CVector4::operator=( const float v )
        {
            _Native = DirectX::XMVectorSet( v, v, v, v );
        }

        bool CVector4::IsZero() const
        {
            return Equals( *this, ZERO );
        }

        bool CVector4::Equals( const CVector4& Left, const CVector4& Right )
        {
            return DirectX::XMVector3Equal( XMVEC4( Left ), XMVEC4( Right ) );
        }

        bool CVector4::Less( const CVector4& Left, const CVector4& Right )
        {
            return DirectX::XMVector3Less( XMVEC4( Left ), XMVEC4( Right ) );
        }

        bool CVector4::Greater( const CVector4& Left, const CVector4& Right )
        {
            return DirectX::XMVector3Greater( XMVEC4( Left ), XMVEC4( Right ) );
        }

        bool CVector4::LessEquals( const CVector4& Left, const CVector4& Right )
        {
            return DirectX::XMVector3LessOrEqual( XMVEC4( Left ), XMVEC4( Right ) );
        }

        bool CVector4::GreaterEquals( const CVector4& Left, const CVector4& Right )
        {
            return DirectX::XMVector3GreaterOrEqual( XMVEC4( Left ), XMVEC4( Right ) );
        }

        void CVector4::Add( const CVector4& Left, const CVector4& Right, CVector4* pOut )
        {
            pOut->_Native = DirectX::XMVectorAdd( XMVEC4( Left ), XMVEC4( Right ) );
        }

        void CVector4::Set( const float x, const float y, const float z, CVector4* pOut )
        {
            pOut->_Native = DirectX::XMVectorSet( x, y, z, 0.0f );
        }

        void CVector4::Set( const float v, CVector4* pOut )
        {
            Set( v, v, v, pOut );
        }

        CVector4 CVector4::Add( const CVector4& Left, const CVector4& Right )
        {
            CVector4 Tmp;
            Add( Left, Right, &Tmp );
            return Tmp;
        }

        void CVector4::Sub( const CVector4& Left, const CVector4& Right, CVector4* pOut )
        {
            pOut->_Native = DirectX::XMVectorSubtract( XMVEC4( Left ), XMVEC4( Right ) );
        }

        CVector4 CVector4::Sub( const CVector4& Left, const CVector4& Right )
        {
            CVector4 Tmp;
            Sub( Left, Right, &Tmp );
            return Tmp;
        }

        void CVector4::Mul( const CVector4& Left, const CVector4& Right, CVector4* pOut )
        {
            pOut->_Native = DirectX::XMVectorMultiply( XMVEC4( Left ), XMVEC4( Right ) );
        }

        CVector4 CVector4::Mul( const CVector4& Left, const CVector4& Right )
        {
            CVector4 Tmp;
            Mul( Left, Right, &Tmp );
            return Tmp;
        }

        void CVector4::Div( const CVector4& Left, const CVector4& Right, CVector4* pOut )
        {
            pOut->_Native = DirectX::XMVectorDivide( XMVEC4( Left ), XMVEC4( Right ) );
        }

        CVector4 CVector4::Div( const CVector4& Left, const CVector4& Right )
        {
            CVector4 Tmp;
            Div( Left, Right, &Tmp );
            return Tmp;
        }

    } // Scene
} // VKE
#endif // VKE_USE_DIRECTX_MATH