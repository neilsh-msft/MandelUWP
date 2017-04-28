#pragma once

namespace MandelIoTCore
{
	// Base class for a SwapChainPanel-based DirectX rendering surface to be used in XAML apps.
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class MandelPanel sealed : public DirectXPanels::DirectXPanelBase
	{
	public:
		MandelPanel();

		void Run(SwapChainPanel^ panel);
	};
}
