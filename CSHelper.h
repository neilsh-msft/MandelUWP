#pragma once

// Helper functions for compute shaders

HRESULT FindDXSDKShaderFileCch(_Out_writes_(cchDest) WCHAR* strDestPath,
	_In_ int cchDest,
	_In_z_ LPCWSTR strFilename);
HRESULT CreateComputeShader(_In_z_ LPCWSTR pSrcFile, _In_z_ LPCSTR pFunctionName,
	_In_ ID3D11Device* pDevice, _Outptr_ ID3D11ComputeShader** ppShaderOut);
HRESULT CreateStructuredBuffer(_In_ ID3D11Device* pDevice, _In_ UINT uElementSize, _In_ UINT uCount,
	_In_reads_(uElementSize*uCount) void* pInitData,
	_Outptr_ ID3D11Buffer** ppBufOut);
HRESULT CreateRawBuffer(_In_ ID3D11Device* pDevice, _In_ UINT uSize, _In_reads_(uSize) void* pInitData, _Outptr_ ID3D11Buffer** ppBufOut);
HRESULT CreateConstantBuffer(_In_ ID3D11Device* pDevice, UINT uSize, _Outptr_ ID3D11Buffer** ppBufOut);
HRESULT CreateTextureBuffer(_In_ ID3D11Device* pDevice, _In_ UINT x, _In_ UINT y, _Outptr_ ID3D11Texture2D** ppBufOut);
HRESULT CreateTextureBuffer(_In_ ID3D11Device* pDevice, _In_ ID3D11Texture2D *pSourceTemplate, _Outptr_ ID3D11Texture2D** ppBufOut);
HRESULT CreateBufferSRV(_In_ ID3D11Device* pDevice, _In_ ID3D11Buffer* pBuffer, _Outptr_ ID3D11ShaderResourceView** ppSRVOut);
HRESULT CreateBufferUAV(_In_ ID3D11Device* pDevice, _In_ ID3D11Buffer* pBuffer, _Outptr_ ID3D11UnorderedAccessView** pUAVOut);
HRESULT CreateTextureUAV(_In_ ID3D11Device* pDevice, _In_ ID3D11Texture2D* pResource, _Outptr_ ID3D11UnorderedAccessView** pUAVOut);
ID3D11Buffer* CreateAndCopyToDebugBuf(_In_ ID3D11Device* pDevice, _In_ ID3D11DeviceContext* pd3dImmediateContext, _In_ ID3D11Buffer* pBuffer);
ID3D11Texture2D* CreateAndCopyToDebugBuf(_In_ ID3D11Device* pDevice, _In_ ID3D11DeviceContext* pd3dImmediateContext, _In_ ID3D11Texture2D* pTexture);
ID3D11Texture2D* CreateAndCopyToDynamicBuf(_In_ ID3D11Device* pDevice, _In_ ID3D11DeviceContext* pd3dImmediateContext, _In_ ID3D11Texture2D* pTexture);

void RunComputeShader(_In_ ID3D11Device* pDevice, _In_ ID3D11DeviceContext* pd3dImmediateContext,
	_In_ ID3D11ComputeShader* pComputeShader,
	_In_ UINT nNumViews, _In_reads_(nNumViews) ID3D11ShaderResourceView** pShaderResourceViews,
	_In_opt_ ID3D11Buffer* pCBCS, _In_reads_opt_(dwNumDataBytes) void* pCSData, _In_ DWORD dwNumDataBytes,
	_In_ ID3D11UnorderedAccessView* pUnorderedAccessView,
	_In_ UINT X, _In_ UINT Y, _In_ UINT Z);
