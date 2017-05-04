// Compute Shader helpers

#include "pch.h"
#include <windows.ui.xaml.media.dxinterop.h>
#include "DirectXHelper.h"
#include "D3DCompiler.h"
#include "CSHelper.h"

using namespace Microsoft::WRL;

//--------------------------------------------------------------------------------------
// Compile and create the CS
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CreateComputeShader(LPCWSTR pSrcFile, LPCSTR pFunctionName,
	ID3D11Device* pDevice, ID3D11ComputeShader** ppShaderOut)
{
	if (!pDevice || !ppShaderOut)
		return E_INVALIDARG;

	// Finds the correct path for the shader file.
	// This is only required for this sample to be run correctly from within the Sample Browser,
	// in your own projects, these lines could be removed safely
	WCHAR str[MAX_PATH];
	HRESULT hr = FindDXSDKShaderFileCch(str, MAX_PATH, pSrcFile);
	if (FAILED(hr))
		return hr;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;

	// Disable optimizations to further improve shader debugging
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	const D3D_SHADER_MACRO defines[] =
	{
#ifdef USE_STRUCTURED_BUFFERS
		"USE_STRUCTURED_BUFFERS", "1",
#endif

#ifdef TEST_DOUBLE
		"TEST_DOUBLE", "1",
#endif
		nullptr, nullptr
	};

	// We generally prefer to use the higher CS shader profile when possible as CS 5.0 is better performance on 11-class hardware
	LPCSTR pProfile = (pDevice->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0) ? "cs_5_0" : "cs_4_0";

	ComPtr<ID3DBlob> pErrorBlob;
	ComPtr<ID3DBlob> pBlob;

#if D3D_COMPILER_VERSION >= 46

	hr = D3DCompileFromFile(str, defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, pFunctionName, pProfile,
		dwShaderFlags, 0, pBlob.GetAddressOf(), pErrorBlob.GetAddressOf());

#else

	hr = D3DX11CompileFromFile(str, defines, nullptr, pFunctionName, pProfile,
		dwShaderFlags, 0, nullptr, &pBlob, &pErrorBlob, nullptr);

#endif

	if (FAILED(hr))
	{
		if (pErrorBlob)
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());

		return hr;
	}

	hr = pDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, ppShaderOut);

#if defined(_DEBUG) || defined(PROFILE)
	if (SUCCEEDED(hr))
	{
		//		(*ppShaderOut)->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(pFunctionName), pFunctionName);
	}
#endif

	return hr;
}


//--------------------------------------------------------------------------------------
// Tries to find the location of the shader file
// This is a trimmed down version of DXUTFindDXSDKMediaFileCch.
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT FindDXSDKShaderFileCch(WCHAR* strDestPath,
	int cchDest,
	LPCWSTR strFilename)
{
	if (!strFilename || strFilename[0] == 0 || !strDestPath || cchDest < 10)
		return E_INVALIDARG;

	// Get the exe name, and exe path
	WCHAR strExePath[MAX_PATH] =
	{
		0
	};
	WCHAR strExeName[MAX_PATH] =
	{
		0
	};
	WCHAR* strLastSlash = nullptr;
	GetModuleFileName(nullptr, strExePath, MAX_PATH);
	strExePath[MAX_PATH - 1] = 0;
	strLastSlash = wcsrchr(strExePath, TEXT('\\'));
	if (strLastSlash)
	{
		wcscpy_s(strExeName, MAX_PATH, &strLastSlash[1]);

		// Chop the exe name from the exe path
		*strLastSlash = 0;

		// Chop the .exe from the exe name
		strLastSlash = wcsrchr(strExeName, TEXT('.'));
		if (strLastSlash)
			*strLastSlash = 0;
	}

	// Search in directories:
	//      .\
		    //      %EXE_DIR%\..\..\%EXE_NAME%

	wcscpy_s(strDestPath, cchDest, strFilename);
	if (GetFileAttributes(strDestPath) != 0xFFFFFFFF)
		return S_OK;

	swprintf_s(strDestPath, cchDest, L"%s\\..\\..\\%s\\%s", strExePath, strExeName, strFilename);
	if (GetFileAttributes(strDestPath) != 0xFFFFFFFF)
		return S_OK;

	// On failure, return the file as the path but also return an error code
	wcscpy_s(strDestPath, cchDest, strFilename);

	return E_FAIL;
}

//--------------------------------------------------------------------------------------
// Create Structured Buffer
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CreateStructuredBuffer(ID3D11Device* pDevice, UINT uElementSize, UINT uCount, void* pInitData, ID3D11Buffer** ppBufOut)
{
	*ppBufOut = nullptr;

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	desc.ByteWidth = uElementSize * uCount;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = uElementSize;

	if (pInitData)
	{
		D3D11_SUBRESOURCE_DATA InitData;
		InitData.pSysMem = pInitData;
		return pDevice->CreateBuffer(&desc, &InitData, ppBufOut);
	}
	else
		return pDevice->CreateBuffer(&desc, nullptr, ppBufOut);
}

//--------------------------------------------------------------------------------------
// Create Texture Buffer - based on passed template
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CreateTextureBuffer(ID3D11Device* pDevice, ID3D11Texture2D * pSourceTemplate, ID3D11Texture2D** ppBufOut)
{
	*ppBufOut = nullptr;

	D3D11_TEXTURE2D_DESC desc = { 0 };
	pSourceTemplate->GetDesc(&desc);

	desc.Usage = D3D11_USAGE_DEFAULT;
	//	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_RENDER_TARGET;
	desc.CPUAccessFlags = 0;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.MipLevels = 1;
	desc.ArraySize = 1;

	return pDevice->CreateTexture2D(&desc, nullptr, ppBufOut);
}

//--------------------------------------------------------------------------------------
// Create Texture Buffer
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CreateTextureBuffer(ID3D11Device* pDevice, UINT x, UINT y, ID3D11Texture2D** ppBufOut)
{
	*ppBufOut = nullptr;

	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.Width = x;
	desc.Height = y;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_RENDER_TARGET;
	//	desc.Format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
	//	desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.CPUAccessFlags = 0;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.MipLevels = 1;
	desc.ArraySize = 1;

	//	desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_INDEX_BUFFER | D3D11_BIND_VERTEX_BUFFER;

	return pDevice->CreateTexture2D(&desc, nullptr, ppBufOut);
}


//--------------------------------------------------------------------------------------
// Create Raw Buffer
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CreateRawBuffer(ID3D11Device* pDevice, UINT uSize, void* pInitData, ID3D11Buffer** ppBufOut)
{
	*ppBufOut = nullptr;

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_INDEX_BUFFER | D3D11_BIND_VERTEX_BUFFER;
	desc.ByteWidth = uSize;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

	if (pInitData)
	{
		D3D11_SUBRESOURCE_DATA InitData;
		InitData.pSysMem = pInitData;
		return pDevice->CreateBuffer(&desc, &InitData, ppBufOut);
	}
	else
		return pDevice->CreateBuffer(&desc, nullptr, ppBufOut);
}

//--------------------------------------------------------------------------------------
// Create Constant Buffer
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CreateConstantBuffer(ID3D11Device* pDevice, UINT uSize, ID3D11Buffer** ppBufOut)
{
	*ppBufOut = nullptr;

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.ByteWidth = uSize;

	return pDevice->CreateBuffer(&desc, nullptr, ppBufOut);
}

//--------------------------------------------------------------------------------------
// Create Shader Resource View for Structured or Raw Buffers
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CreateBufferSRV(ID3D11Device* pDevice, ID3D11Buffer* pBuffer, ID3D11ShaderResourceView** ppSRVOut)
{
	D3D11_BUFFER_DESC descBuf;
	ZeroMemory(&descBuf, sizeof(descBuf));
	pBuffer->GetDesc(&descBuf);

	D3D11_SHADER_RESOURCE_VIEW_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	desc.BufferEx.FirstElement = 0;

	if (descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
	{
		// This is a Raw Buffer

		desc.Format = DXGI_FORMAT_R32_TYPELESS;
		desc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
		desc.BufferEx.NumElements = descBuf.ByteWidth / 4;
	}
	else
		if (descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED)
		{
			// This is a Structured Buffer

			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.BufferEx.NumElements = descBuf.ByteWidth / descBuf.StructureByteStride;
		}
		else
		{
			return E_INVALIDARG;
		}

	return pDevice->CreateShaderResourceView(pBuffer, &desc, ppSRVOut);
}

//--------------------------------------------------------------------------------------
// Create Unordered Access View for Structured or Raw Buffers
//-------------------------------------------------------------------------------------- 
_Use_decl_annotations_
HRESULT CreateBufferUAV(ID3D11Device* pDevice, ID3D11Buffer* pBuffer, ID3D11UnorderedAccessView** ppUAVOut)
{
	D3D11_BUFFER_DESC descBuf;
	ZeroMemory(&descBuf, sizeof(descBuf));
	pBuffer->GetDesc(&descBuf);

	D3D11_UNORDERED_ACCESS_VIEW_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	desc.Buffer.FirstElement = 0;

	if (descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
	{
		// This is a Raw Buffer

		desc.Format = DXGI_FORMAT_R32_TYPELESS; // Format must be DXGI_FORMAT_R32_TYPELESS, when creating Raw Unordered Access View
		desc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
		desc.Buffer.NumElements = descBuf.ByteWidth / 4;
	}
	else
		if (descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED)
		{
			// This is a Structured Buffer

			desc.Format = DXGI_FORMAT_UNKNOWN;      // Format must be must be DXGI_FORMAT_UNKNOWN, when creating a View of a Structured Buffer
			desc.Buffer.NumElements = descBuf.ByteWidth / descBuf.StructureByteStride;
		}
		else
		{
			return E_INVALIDARG;
		}

	return pDevice->CreateUnorderedAccessView(pBuffer, &desc, ppUAVOut);
}

//--------------------------------------------------------------------------------------
// Create Unordered Access View for 2D Texture
//-------------------------------------------------------------------------------------- 
_Use_decl_annotations_
HRESULT CreateTextureUAV(ID3D11Device* pDevice, ID3D11Texture2D* pResource, ID3D11UnorderedAccessView** ppUAVOut)
{
#if false
	D3D11_TEXTURE2D_DESC descBuf;
	ZeroMemory(&descBuf, sizeof(descBuf));
	pResource->GetDesc(&descBuf);

	D3D11_UNORDERED_ACCESS_VIEW_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipSlice = 0;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	//	desc.Format = descBuf.Format;
	//	return pDevice->CreateUnorderedAccessView(pResource, &desc, ppUAVOut);
#endif

	return pDevice->CreateUnorderedAccessView(pResource, NULL, ppUAVOut);
}

//--------------------------------------------------------------------------------------
// Create a CPU accessible buffer and download the content of a GPU buffer into it
// This function is very useful for debugging CS programs
//-------------------------------------------------------------------------------------- 
_Use_decl_annotations_
ID3D11Buffer* CreateAndCopyToDebugBuf(ID3D11Device* pDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3D11Buffer* pBuffer)
{
	ID3D11Buffer* debugbuf = nullptr;

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	pBuffer->GetDesc(&desc);
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.Usage = D3D11_USAGE_STAGING;
	desc.BindFlags = 0;
	desc.MiscFlags = 0;
	if (SUCCEEDED(pDevice->CreateBuffer(&desc, nullptr, &debugbuf)))
	{
#if defined(_DEBUG) || defined(PROFILE)
		//		debugbuf->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof("Debug") - 1, "Debug");
#endif

		pd3dImmediateContext->CopyResource(debugbuf, pBuffer);
	}

	return debugbuf;
}

_Use_decl_annotations_
ID3D11Texture2D* CreateAndCopyToDebugBuf(ID3D11Device* pDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3D11Texture2D* pTexture)
{
	ID3D11Texture2D* debugbuf = nullptr;

	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	pTexture->GetDesc(&desc);
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.Usage = D3D11_USAGE_STAGING;
	desc.BindFlags = 0;
	desc.MiscFlags = 0;
	if (SUCCEEDED(pDevice->CreateTexture2D(&desc, nullptr, &debugbuf)))
	{
#if defined(_DEBUG) || defined(PROFILE)
		//		debugbuf->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof("Debug") - 1, "Debug");
#endif

		pd3dImmediateContext->CopyResource(debugbuf, pTexture);
	}

	return debugbuf;
}

ID3D11Texture2D* CreateAndCopyToDynamicBuf(ID3D11Device* pDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3D11Texture2D* pTexture)
{
	ID3D11Texture2D* debugbuf = nullptr;

	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	pTexture->GetDesc(&desc);
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.MiscFlags = 0;

	if (SUCCEEDED(pDevice->CreateTexture2D(&desc, nullptr, &debugbuf)))
	{
#if defined(_DEBUG) || defined(PROFILE)
		//		debugbuf->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof("Debug") - 1, "Debug");
#endif

		pd3dImmediateContext->CopyResource(debugbuf, pTexture);
	}

	return debugbuf;
}

#ifdef DEADCODE
ManualUNORMtoFloatConversion()
{
	ComPtr<ID3D11Texture2D> textureBuf;
	ComPtr<ID3D11Texture2D> swapchainBuf;
	*swapchainBuf.GetAddressOf() = CreateAndCopyToDynamicBuf(m_d3dDevice.Get(), m_d3dContext.Get(), m_swapchainTexture.Get());
	*textureBuf.GetAddressOf() = CreateAndCopyToDebugBuf(m_d3dDevice.Get(), m_d3dContext.Get(), m_textureOutput.Get());

	D3D11_MAPPED_SUBRESOURCE mappedSwapchain = { 0 };
	D3D11_MAPPED_SUBRESOURCE mappedTexture = { 0 };
	D3D11_TEXTURE2D_DESC swapchainDesc = { 0 };
	D3D11_TEXTURE2D_DESC textureDesc = { 0 };
	swapchainBuf->GetDesc(&swapchainDesc);
	textureBuf->GetDesc(&textureDesc);

	assert(swapchainDesc.Height == textureDesc.Height);
	assert(swapchainDesc.Width == textureDesc.Width);

	DX::ThrowIfFailed(m_d3dContext->Map(textureBuf.Get(), 0, D3D11_MAP_READ, 0, &mappedTexture));
	DX::ThrowIfFailed(m_d3dContext->Map(swapchainBuf.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSwapchain));

	float* src = (float *)mappedTexture.pData;
	BYTE* dst = (BYTE *)mappedSwapchain.pData;
	for (UINT y = 0; y < swapchainDesc.Height; y++)
	{
		for (UINT x = 0; x < swapchainDesc.Width; x++)
		{
			/*
			dst[x * 4] = 0x80;
			dst[x * 4 + 1] = 0x80;
			dst[x * 4 + 2] = 0x80;
			dst[x * 4 + 3] = 0xFF;
			*/
			dst[x * 4] = (BYTE)(src[x * 4] * 255);
			dst[x * 4 + 1] = (BYTE)(src[x * 4 + 1] * 255);
			dst[x * 4 + 2] = (BYTE)(src[x * 4 + 2] * 255);
			dst[x * 4 + 3] = (BYTE)(src[x * 4 + 3] * 255);
		}
		src += mappedTexture.RowPitch / sizeof(float);
		dst += mappedSwapchain.RowPitch / sizeof(byte);
	}

	m_d3dContext->Unmap(swapchainBuf.Get(), 0);
	m_d3dContext->Unmap(textureBuf.Get(), 0);

	m_d3dContext->CopyResource(m_swapchainTexture.Get(), swapchainBuf.Get());

	swapchainBuf.Reset();
	textureBuf.Reset();

}

#endif

//--------------------------------------------------------------------------------------
// Run CS
//-------------------------------------------------------------------------------------- 
_Use_decl_annotations_
void RunComputeShader(ID3D11Device *pDevice, ID3D11DeviceContext* pd3dImmediateContext,
	ID3D11ComputeShader* pComputeShader,
	UINT nNumViews, ID3D11ShaderResourceView** pShaderResourceViews,
	ID3D11Buffer* pCBCS, void* pCSData, DWORD dwNumDataBytes,
	ID3D11UnorderedAccessView* pUnorderedAccessView,
	UINT X, UINT Y, UINT Z)
{
	pd3dImmediateContext->CSSetShader(pComputeShader, nullptr, 0);
	pd3dImmediateContext->CSSetShaderResources(0, nNumViews, pShaderResourceViews);
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &pUnorderedAccessView, nullptr);
	if (pCBCS && pCSData)
	{
		D3D11_MAPPED_SUBRESOURCE MappedResource;
		DX::ThrowIfFailed(pd3dImmediateContext->Map(pCBCS, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
		memcpy(MappedResource.pData, pCSData, dwNumDataBytes);
		pd3dImmediateContext->Unmap(pCBCS, 0);
		ID3D11Buffer* ppCB[1] = { pCBCS };
		pd3dImmediateContext->CSSetConstantBuffers(0, 1, ppCB);
	}

	pd3dImmediateContext->Dispatch(X, Y, Z);

	// cleanup
	pd3dImmediateContext->CSSetShader(nullptr, nullptr, 0);

	ID3D11UnorderedAccessView* ppUAViewnullptr[1] = { nullptr };
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, ppUAViewnullptr, nullptr);

	ID3D11ShaderResourceView* ppSRVnullptr[2] = { nullptr, nullptr };
	pd3dImmediateContext->CSSetShaderResources(0, 2, ppSRVnullptr);

	ID3D11Buffer* ppCBnullptr[1] = { nullptr };
	pd3dImmediateContext->CSSetConstantBuffers(0, 1, ppCBnullptr);
}


