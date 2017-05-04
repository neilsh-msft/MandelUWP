#pragma once

namespace MandelIoTCore
{
	// DirectX based Mandelbrot generator panel
	[Windows::Foundation::Metadata::WebHostHidden]
	ref class MandelPanel sealed : public DirectXPanels::DirectXPanelBase
	{
	private:
		Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_shader;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_constantBuffer;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> m_swapchainTexture;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_computeOutputUAV;

		double a, b;	// real and imaginary origin for set
		double d;		// inverse magnification

	public:
		MandelPanel(Windows::UI::Xaml::Controls::SwapChainPanel ^ panel);

		virtual void Render() override;
		virtual void CreateDeviceResources() override;
		virtual void CreateSizeDependentResources() override;

		virtual void OnDeviceLost() override;
		virtual void OnSizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e) override;
		virtual void OnCompositionScaleChanged(Windows::UI::Xaml::Controls::SwapChainPanel ^sender, Platform::Object ^args) override;
		virtual void OnResuming(Platform::Object^ sender, Platform::Object^ args) override;

		void Init();
		void Run();

		void StartRenderLoop();
		void StopRenderLoop();

		void OnPointerWheelChanged(Platform::Object ^sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs ^e);
		void OnPointerPressed(Platform::Object ^sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs ^e);
	};
}
