
#if VKE_USE_DIRECTX_MATH
namespace VKE
{
    namespace Math
    {
        void CMatrix4x4::SetLookAt( const CVector3& Position, const CVector3& AtPosition, const CVector3& Up )
        {
#if VKE_USE_RIGHT_HANDED_COORDINATES
            //_Native = DirectX::XMMatrixLookAtRH( Position._Native, AtPosition._Native, Up._Native );
            DirectX::XMStoreFloat4x4( &_Native,
                DirectX::XMMatrixLookAtRH(
                    DirectX::XMLoadFloat3( &Position._Native ),
                    DirectX::XMLoadFloat3( &AtPosition._Native ),
                    DirectX::XMLoadFloat3( &Up._Native ) ) );
#else
            _Native = DirectX::XMMatrixLookAtLH( Position.m_Native, AtPosition.m_Native, Up.m_Native );
#endif
        }

        void CMatrix4x4::SetLookTo( const CVector3& Position, const CVector3& Direction, const CVector3& Up )
        {
#if VKE_USE_RIGHT_HANDED_COORDINATES
            //_Native = DirectX::XMMatrixLookToRH( Position._Native, Direction._Native, Up._Native );
#else
            _Native = DirectX::XMMatrixLookToLH( Position.m_Native, Direction.m_Native, Up.m_Native );
#endif
        }

        void CMatrix4x4::SetPerspective( const ExtentF32& Viewport, const ExtentF32& Planes )
        {
#if VKE_USE_RIGHT_HANDED_COORDINATES
            //_Native = DirectX::XMMatrixPerspectiveRH( Viewport.width, Viewport.height, Planes.min, Planes.max );
#else
            _Native = DirectX::XMMatrixPerspectiveLH( Viewport.width, Viewport.height, Planes.min, Planes.max );
#endif
        }

        void CMatrix4x4::SetPerspectiveFOV( float fovAngleY, float aspectRatio, const ExtentF32& Planes )
        {
#if VKE_USE_RIGHT_HANDED_COORDINATES
            //_Native = DirectX::XMMatrixPerspectiveFovRH( fovAngleY, aspectRatio, Planes.min, Planes.max );
            DirectX::XMStoreFloat4x4( &_Native, DirectX::XMMatrixPerspectiveFovRH( fovAngleY, aspectRatio, Planes.min, Planes.max ) );
#else
            _Native = DirectX::XMMatrixPerspectiveFovLH( fovAngleY, aspectRatio, Planes.min, Planes.max );
#endif
        }

        void CMatrix4x4::Mul( const CMatrix4x4& Left, const CMatrix4x4& Right, CMatrix4x4* pOut )
        {
            //pOut->_Native = DirectX::XMMatrixMultiply( Left._Native, Right._Native );
            DirectX::XMStoreFloat4x4( &pOut->_Native,
                DirectX::XMMatrixMultiply(
                    DirectX::XMLoadFloat4x4( &Left._Native ),
                    DirectX::XMLoadFloat4x4( &Right._Native ) ) );
        }

        void CMatrix4x4::Transpose( const CMatrix4x4& Src, CMatrix4x4* pDst )
        {
            //pDst->_Native = DirectX::XMMatrixTranspose( Src._Native );
        }

        void CMatrix4x4::Transpose( CMatrix4x4* pInOut )
        {
            //pInOut->_Native = DirectX::XMMatrixTranspose( pInOut->_Native );
        }

        void CMatrix4x4::Invert( CVector3* pDeterminant, const CMatrix4x4& Src, CMatrix4x4* pDst )
        {
            //pDst->_Native = DirectX::XMMatrixInverse( &pDeterminant->_Native, Src._Native );
        }

        void CMatrix4x4::Invert( CVector3* pDeterminant, CMatrix4x4* pInOut )
        {
            //pInOut->_Native = DirectX::XMMatrixInverse( &pDeterminant->_Native, pInOut->_Native );
        }

        void CMatrix4x4::Translate( const CVector3& Vec, CMatrix4x4* pOut )
        {
            /*pOut->_Native = DirectX::XMMatrixAffineTransformation( CVector3::ONE._Native, CVector3::ONE._Native,
                CVector3::ONE._Native, Vec._Native );*/
            DirectX::XMStoreFloat4x4( &pOut->_Native, DirectX::XMMatrixTranslation( Vec.x, Vec.y, Vec.z ) );
        }

        void CMatrix4x4::Identity( CMatrix4x4* pOut )
        {
            DirectX::XMStoreFloat4x4( &pOut->_Native, DirectX::XMMatrixIdentity() );
        }

        CMatrix4x4 CMatrix4x4::Identity()
        {
            CMatrix4x4 tmp;
            Identity( &tmp );
            return tmp;
        }

    } // Math
} // VKE
#endif // #if VKE_USE_DIRECTX_MATH