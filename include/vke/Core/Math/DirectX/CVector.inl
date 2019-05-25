#if VKE_USE_DIRECTX_MATH
namespace VKE
{
#define XMVEC(_vec) DirectX::XMLoadFloat3(&(_vec._Native))

    namespace Math
    {
        CVector::CVector( float f ) :
            _Native{ f, f, f }
        {
            
        }

        CVector::CVector(float x, float y, float z) :
            _Native{ x, y, z }
        {}

        CVector::CVector(const CVector& Other) :
            _Native{ Other._Native }
        {}

        void CVector::operator=( const float v )
        {
            DirectX::XMStoreFloat3( &_Native, DirectX::XMVectorSet( v, v, v, 0.0f ) );
        }

        bool CVector::IsZero() const
        {
            return Equals( *this, ZERO );
        }

        bool CVector::Equals( const CVector& Left, const CVector& Right )
        {
            return DirectX::XMVector3Equal( XMVEC( Left ), XMVEC( Right ) );
        }

        bool CVector::Less( const CVector& Left, const CVector& Right )
        {
            return DirectX::XMVector3Less( XMVEC( Left ), XMVEC( Right ) );
        }

        bool CVector::Greater( const CVector& Left, const CVector& Right )
        {
            return DirectX::XMVector3Greater( XMVEC( Left ), XMVEC( Right ) );
        }

        bool CVector::LessEquals( const CVector& Left, const CVector& Right )
        {
            return DirectX::XMVector3LessOrEqual( XMVEC( Left ), XMVEC( Right ) );
        }

        bool CVector::GreaterEquals( const CVector& Left, const CVector& Right )
        {
            return DirectX::XMVector3GreaterOrEqual( XMVEC( Left ), XMVEC( Right ) );
        }

        void CVector::Add( const CVector& Left, const CVector& Right, CVector* pOut )
        {
            DirectX::XMStoreFloat3( &pOut->_Native,
                DirectX::XMVectorAdd( XMVEC( Left ), XMVEC( Right ) ) );
        }

        void CVector::Set( const float x, const float y, const float z, CVector* pOut )
        {
            DirectX::XMStoreFloat3( &pOut->_Native, DirectX::XMVectorSet( x, y, z, 0.0f ) );
        }

        void CVector::Set( const float v, CVector* pOut )
        {
            Set( v, v, v, pOut );
        }

        CVector CVector::Add( const CVector& Left, const CVector& Right )
        {
            CVector Tmp;
            Add( Left, Right, &Tmp );
            return Tmp;
        }

        void CVector::Sub( const CVector& Left, const CVector& Right, CVector* pOut )
        {
            DirectX::XMStoreFloat3( &pOut->_Native,
                DirectX::XMVectorSubtract( XMVEC( Left ), XMVEC( Right ) ) );
        }

        CVector CVector::Sub( const CVector& Left, const CVector& Right )
        {
            CVector Tmp;
            Sub( Left, Right, &Tmp );
            return Tmp;
        }

        void CVector::Mul( const CVector& Left, const CVector& Right, CVector* pOut )
        {
            DirectX::XMStoreFloat3( &pOut->_Native,
                DirectX::XMVectorMultiply( XMVEC( Left ), XMVEC( Right ) ) );
        }

        CVector CVector::Mul( const CVector& Left, const CVector& Right )
        {
            CVector Tmp;
            Mul( Left, Right, &Tmp );
            return Tmp;
        }

        void CVector::Div( const CVector& Left, const CVector& Right, CVector* pOut )
        {
            DirectX::XMStoreFloat3( &pOut->_Native,
                DirectX::XMVectorDivide( XMVEC( Left ), XMVEC( Right ) ) );
        }

        CVector CVector::Div( const CVector& Left, const CVector& Right )
        {
            CVector Tmp;
            Div( Left, Right, &Tmp );
            return Tmp;
        }

    } // Math
} // VKE
#endif // VKE_USE_DIRECTX_MATH