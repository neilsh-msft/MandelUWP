//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"

#include "MainPage.xaml.h"
#include <windows.ui.xaml.media.dxinterop.h>

using namespace MandelIoTCore;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

MainPage::MainPage()
{
	InitializeComponent();

	m_mandel = ref new MandelPanel(panel);
	m_mandel->Init();
	m_mandel->Run();
}

void MainPage::OnNavigatedTo(NavigationEventArgs ^e)
{
	m_mandel->StartRenderLoop();
}
void MainPage::OnNavigatedFrom(NavigationEventArgs ^e)
{
	m_mandel->StopRenderLoop();
}

