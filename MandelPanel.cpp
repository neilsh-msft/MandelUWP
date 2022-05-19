#include "pch.h"
#include <windows.ui.xaml.media.dxinterop.h>
#include "DirectXHelper.h"
#include "DirectXPanelBase.h"
#include "D3DCompiler.h"

#include "MandelPanel.h"

#include "CSHelper.h"

using namespace MandelIoTCore;

using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Controls;

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Details;

// QC doesn't support double precision
#if 0
struct ConstantBuffer
{
	double a0, b0, da, db;
	double  ja0, jb0; // julia set point

	int max_iterations;
	bool julia;  // julia or mandel
	int  cycle;
};
#else
struct ConstantBuffer
{
	float a0, b0, da, db;
	float  ja0, jb0; // julia set point

	int max_iterations;
	bool julia;  // julia or mandel
	int  cycle;
	int reserved[3];  // padding out to 16 byte boundary
};
#endif

ConstantBuffer g_constants;

static_assert(sizeof(g_constants) % 16 == 0, "ConstantBuffer not multiple of 16");

MandelPanel::MandelPanel(Windows::UI::Xaml::Controls::SwapChainPanel ^ panel) : DirectXPanelBase(panel)
{
	a = 0.0;
	b = 0.0;
	d = 1.0;

	panel->PointerWheelChanged += ref new Windows::UI::Xaml::Input::PointerEventHandler(this, &MandelIoTCore::MandelPanel::OnPointerWheelChanged);
	panel->PointerPressed += ref new Windows::UI::Xaml::Input::PointerEventHandler(this, &MandelIoTCore::MandelPanel::OnPointerPressed);
}

void MandelPanel::Init()
{
	CreateDeviceIndependentResources();
	CreateDeviceResources();
	CreateSizeDependentResources();
}

void MandelPanel::CreateDeviceResources()
{
	DirectXPanelBase::CreateDeviceResources();

	// Set D2D's unit mode to pixels so that drawing operation units are interpreted in pixels rather than DIPS. 
//	m_d2dContext->SetUnitMode(D2D1_UNIT_MODE::D2D1_UNIT_MODE_PIXELS);

	m_loadingComplete = true;
	auto d3d = m_d3dDevice.Get();
	auto shader = m_shader.GetAddressOf();
	auto result = CreateComputeShader(L"CSMandelJulia_scalarFloat.hlsl", "CSMandelJulia_scalarFloat", d3d, shader);
	DX::ThrowIfFailed(result);
}

void MandelPanel::CreateSizeDependentResources()
{
	// free previous output textures and create new UAV target
	m_constantBuffer.Reset();
	m_swapchainTexture.Reset();
	m_computeOutputUAV.Reset();

	DirectXPanelBase::CreateSizeDependentResources();

	UINT height = static_cast<UINT>(m_renderTargetHeight / m_compositionScaleY);
	UINT width = static_cast<UINT>(m_renderTargetWidth / m_compositionScaleX);

#ifdef _DEBUG
	DXGI_SWAP_CHAIN_DESC sd;
	DX::ThrowIfFailed(m_swapChain->GetDesc(&sd));
#endif 

	g_constants.a0 = (float)(a - (d * 2.0));
	g_constants.b0 = (float)(b - (d * 2.0));
	g_constants.da = (float)(4.0 * d / width);
	g_constants.db = (float)(4.0 * d / width);  // would use height, but then output aspect ratio is incorrect
	g_constants.cycle = -110;
	g_constants.max_iterations = 1024;
	g_constants.julia = false;
	g_constants.ja0 = 0.0;
	g_constants.jb0 = 0.0;

	// create constant buffer for CS
	auto hr = CreateConstantBuffer(m_d3dDevice.Get(), sizeof(g_constants), m_constantBuffer.ReleaseAndGetAddressOf());
	DX::ThrowIfFailed(hr);

	// get the swapchain background texture buffer and map to a UAV
	hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)m_swapchainTexture.ReleaseAndGetAddressOf());
	DX::ThrowIfFailed(hr);
	
	// get a UAV on the swapchain background texture buffer for the CS to write to
	hr = CreateTextureUAV(m_d3dDevice.Get(), m_swapchainTexture.Get(), m_computeOutputUAV.ReleaseAndGetAddressOf());
	DX::ThrowIfFailed(hr);
}

void MandelPanel::Render()
{
	if (!m_loadingComplete)
	{
		return;
	}

	UINT height = static_cast<UINT>(m_renderTargetHeight) * m_compositionScaleY;
	UINT width = static_cast<UINT>(m_renderTargetWidth) * m_compositionScaleX;

	RunComputeShader(m_d3dDevice.Get(), m_d3dContext.Get(), m_shader.Get(), 0, nullptr, m_constantBuffer.Get(), &g_constants, sizeof(g_constants), m_computeOutputUAV.Get(),
		width, height, 1);

#if _DEBUG
	// map the CS output to CPU memory so we can look at it.
	ComPtr<ID3D11Texture2D> textureBuf;
	*textureBuf.GetAddressOf() = CreateAndCopyToDebugBuf(m_d3dDevice.Get(), m_d3dContext.Get(), m_swapchainTexture.Get());
	D3D11_MAPPED_SUBRESOURCE mappedTexture = { 0 };
	D3D11_TEXTURE2D_DESC textureDesc = { 0 };
	textureBuf->GetDesc(&textureDesc);

	DX::ThrowIfFailed(m_d3dContext->Map(textureBuf.Get(), 0, D3D11_MAP_READ, 0, &mappedTexture));

	ULONG* src = (ULONG *)mappedTexture.pData;
	m_d3dContext->Unmap(textureBuf.Get(), 0);
#endif

	Present();
}
void MandelPanel::Run()
{
	Render();
	Present();
}

void MandelPanel::OnDeviceLost()
{
	// Handle device lost, then re-render.
	DirectXPanelBase::OnDeviceLost();
	Render();
}

void MandelPanel::OnSizeChanged(Platform::Object^ sender, SizeChangedEventArgs^ e)
{
	// Process SizeChanged event, then re-render at the new size.
	DirectXPanelBase::OnSizeChanged(sender, e);
	Render();
}

void MandelPanel::OnCompositionScaleChanged(SwapChainPanel ^sender, Platform::Object ^args)
{
	// Process CompositionScaleChanged event, then re-render at the new scale.
	DirectXPanelBase::OnCompositionScaleChanged(sender, args);
	Render();
}

void MandelPanel::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
	// Ensure content is rendered when the app is resumed.
	Render();
}

void MandelIoTCore::MandelPanel::OnPointerWheelChanged(Platform::Object ^sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs ^e)
{
	Windows::UI::Input::PointerPoint ^p = e->GetCurrentPoint(this);
	
	double change = pow(2.0, - p->Properties->MouseWheelDelta / 240.0);

	d *= change;
	if (d > 4.0)
	{
		d = 4.0;
	}

	CreateSizeDependentResources();
	Render();
}

void MandelIoTCore::MandelPanel::OnPointerPressed(Platform::Object ^sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs ^e)
{
	Windows::UI::Input::PointerPoint ^p = e->GetCurrentPoint(this);

	float x = p->Position.X;
	float y = p->Position.Y;

	a += ((x * m_compositionScaleX) / m_renderTargetWidth - 0.5) * d * 4;
	b += ((y * m_compositionScaleY) / m_renderTargetHeight - 0.5) * (m_renderTargetWidth / m_renderTargetHeight) * d * 4;  // would use height, but display aspect ratio is off

	a = min(a, 2.0);
	a = max(a, -2.0);
	b = min(b, 2.0);
	b = max(b, -2.0);

	CreateSizeDependentResources();
	Render();
}

void MandelPanel::StartRenderLoop()
{
}

void MandelPanel::StopRenderLoop()
{
}
