//
// Main.cpp
//

#include "pch.h"
#include "Game.h"

using namespace winrt::Windows::ApplicationModel;
using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::ApplicationModel::Activation;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::Foundation;
using namespace DirectX;

bool g_HDRMode = false;

class ViewProvider final : public winrt::implements<ViewProvider, IFrameworkView>
{
public:
    ViewProvider() :
        m_exit(false)
    {
    }

    // IFrameworkView methods
    void Initialize(CoreApplicationView const & applicationView)
    {
        applicationView.Activated({ this, &ViewProvider::OnActivated });

        CoreApplication::Suspending({ this, &ViewProvider::OnSuspending });

        CoreApplication::Resuming({ this, &ViewProvider::OnResuming });

        CoreApplication::DisableKinectGpuReservation(true);

        m_game = std::make_unique<Game>();

        if (m_game->RequestHDRMode())
        {
            // Request HDR mode.
            auto determineHDR = winrt::Windows::Xbox::Graphics::Display::DisplayConfiguration::TrySetHdrModeAsync();

            // In a real game, you'd do some initialization here to hide the HDR mode switch.

            // Finish up HDR mode detection
            while (determineHDR.Status() == AsyncStatus::Started) { Sleep(100); };
            if (determineHDR.Status() != AsyncStatus::Completed)
            {
                throw std::exception("TrySetHdrModeAsync");
            }
            g_HDRMode = determineHDR.get().HdrEnabled();

#ifdef _DEBUG
            OutputDebugStringA((g_HDRMode) ? "INFO: Display in HDR Mode\n" : "INFO: Display in SDR Mode\n");
#endif
        }
    }

    void Uninitialize()
    {
        m_game.reset();
    }

    void SetWindow(CoreWindow const & window)
    {
        window.Closed({ this, &ViewProvider::OnWindowClosed });

        ::IUnknown* windowPtr = winrt::get_abi(window);
        m_game->Initialize(windowPtr);
    }

    void Load(winrt::hstring const & /*entryPoint*/)
    {
    }

    void Run()
    {
        while (!m_exit)
        {
            m_game->Tick();

            CoreWindow::GetForCurrentThread().Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
        }
    }

protected:
    // Event handlers
    void OnActivated(CoreApplicationView const & /*applicationView*/, IActivatedEventArgs const & /*args*/)
    {
        CoreWindow::GetForCurrentThread().Activate();
    }

    void OnSuspending(IInspectable const & /*sender*/, SuspendingEventArgs const & args)
    {
        auto deferral = args.SuspendingOperation().GetDeferral();

        std::async(std::launch::async, [this, deferral]()
        {
            m_game->OnSuspending();

            deferral.Complete();
        });
    }

    void OnResuming(IInspectable const & /*sender*/, IInspectable const & /*args*/)
    {
        m_game->OnResuming();
    }

    void OnWindowClosed(CoreWindow const & /*sender*/, CoreWindowEventArgs const & /*args*/)
    {
        m_exit = true;
    }

private:
    bool                    m_exit;
    std::unique_ptr<Game>   m_game;
};

class ViewProviderFactory final : public winrt::implements<ViewProviderFactory, IFrameworkViewSource>
{
public:
    IFrameworkView CreateView()
    {
        return winrt::make<ViewProvider>();
    }
};


// Entry point
int WINAPIV WinMain()
{
    winrt::init_apartment();

    ViewProviderFactory viewProviderFactory;
    CoreApplication::Run(viewProviderFactory);

    winrt::uninit_apartment();
    return 0;
}


// Exit helper
void ExitGame()
{
    winrt::Windows::ApplicationModel::Core::CoreApplication::Exit();
}
