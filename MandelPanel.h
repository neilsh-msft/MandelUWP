#pragma once

namespace MandelIoTCore
{
	// Base class for a SwapChainPanel-based DirectX rendering surface to be used in XAML apps.
	[Windows::Foundation::Metadata::WebHostHidden]
	ref class MandelPanel sealed : public DirectXPanels::DirectXPanelBase
	{
	private:
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_computeOutput;

	public:
		MandelPanel(Windows::UI::Xaml::Controls::SwapChainPanel ^ panel);

		virtual void Render() override;
		virtual void CreateDeviceResources() override;

		virtual void OnDeviceLost() override;
		virtual void OnSizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e) override;
		virtual void OnCompositionScaleChanged(Windows::UI::Xaml::Controls::SwapChainPanel ^sender, Platform::Object ^args) override;
		virtual void OnResuming(Platform::Object^ sender, Platform::Object^ args) override;

		void Init();
		void Run();
	};
}
