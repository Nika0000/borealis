#include <borealis/core/logger.hpp>
#include <borealis/platforms/driver/d3d11.hpp>

#define NANOVG_D3D11_IMPLEMENTATION
#include <dxgi1_6.h>
#include <nanovg_d3d11.h>
#include <versionhelpers.h>
#ifdef __GLFW__
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#elif defined(__SDL3__)
#include <SDL3/SDL.h>
#endif
#ifdef __WINRT__
#include <windows.ui.core.h>
#include <winrt/Windows.Graphics.Display.h>
#endif

#include <d3d11_4.h>

namespace brls
{

static const int SwapChainBufferCount    = 3;
static const DXGI_SAMPLE_DESC sampleDesc = { 1, 0 };

static DXGI_FORMAT getSwapChainFormatForHDR(bool enabled) { return enabled ? DXGI_FORMAT_R10G10B10A2_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM; }

static DXGI_COLOR_SPACE_TYPE getColorSpaceForHDR(bool enabled)
{
    return enabled ? DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 : DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
}

#ifdef __GLFW__
D3D11Context::D3D11Context(GLFWwindow* window, int width, int height)
{
    m_hWnd = glfwGetWin32Window(window);
    initDX(m_hWnd, nullptr, width, height);
}
#elif defined(__SDL3__)
D3D11Context::D3D11Context(SDL_Window* window, int width, int height)
{
#ifdef __ALLOW_TEARING__
    m_allowTearing = true;
#endif

#ifdef __WINRT__
    // winrt 代码需要特别编译
    ABI::Windows::UI::Core::ICoreWindow* coreWindow = nullptr;
    if (FAILED(wi.info.winrt.window->QueryInterface(&coreWindow)))
    {
        return;
    }
    initDX(nullptr, coreWindow, width, height);
#else
    m_hWnd = (HWND) SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    initDX(m_hWnd, nullptr, width, height);
#endif
}
#endif

D3D11Context::~D3D11Context() { unInitDX(); }

bool D3D11Context::initDX(HWND window, IUnknown* coreWindow, int width, int height)
{
    HRESULT hr = S_OK;

    IDXGIDevice* dxgiDevice    = nullptr;
    IDXGIAdapter* dxgiAdapter  = nullptr;
    IDXGIFactory2* dxgiFactory = nullptr;

    // Track tearing support
    static const D3D_DRIVER_TYPE driverAttempts[] = {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };

    static const D3D_FEATURE_LEVEL levelAttempts[] = {
        D3D_FEATURE_LEVEL_11_1, // Direct3D 11.1 SM 6
        D3D_FEATURE_LEVEL_11_0, // Direct3D 11.0 SM 5
        D3D_FEATURE_LEVEL_10_1, // Direct3D 10.1 SM 4
        D3D_FEATURE_LEVEL_10_0, // Direct3D 10.0 SM 4
        D3D_FEATURE_LEVEL_9_3,  // Direct3D 9.3  SM 3
        D3D_FEATURE_LEVEL_9_2,  // Direct3D 9.2  SM 2
        D3D_FEATURE_LEVEL_9_1,  // Direct3D 9.1  SM 2
    };

    for (size_t driver = 0; driver < ARRAYSIZE(driverAttempts); driver++)
    {
        UINT createFlags = D3D11_CREATE_DEVICE_VIDEO_SUPPORT | D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
        createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        hr = D3D11CreateDevice(
            nullptr,
            driverAttempts[driver],
            nullptr,
            createFlags,
            levelAttempts,
            ARRAYSIZE(levelAttempts),
            D3D11_SDK_VERSION,
            &m_device,
            nullptr,
            &m_deviceContext
        );

        if (SUCCEEDED(hr))
        {
            break;
        }
    }
    // Enable multithread protection since we use the immediate context across threads
    if (SUCCEEDED(hr))
    {
        ID3D11Multithread* mt = nullptr;
        if (SUCCEEDED(m_deviceContext->QueryInterface(IID_PPV_ARGS(&mt))))
        {
            mt->SetMultithreadProtected(TRUE);
            mt->Release();
        }
    }
    if (SUCCEEDED(hr))
    {
        hr = m_device->QueryInterface(IID_PPV_ARGS(&dxgiDevice));
    }
    if (SUCCEEDED(hr))
    {
        hr = dxgiDevice->GetAdapter(&dxgiAdapter);
    }
    if (SUCCEEDED(hr))
    {
        hr = dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));
    }

    if (SUCCEEDED(hr))
    {
        // Use frame latency waitable object so Present() returns immediately.
        // This prevents the D3D11 device context from being held during vsync
        // wait, allowing decoder threads to submit GPU work concurrently.
        m_swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

        DXGI_SWAP_CHAIN_DESC1 swapDesc;
        ZeroMemory(&swapDesc, sizeof(swapDesc));
        swapDesc.SampleDesc.Count   = sampleDesc.Count;
        swapDesc.SampleDesc.Quality = sampleDesc.Quality;
        swapDesc.Format             = m_swapChainFormat;
        swapDesc.Stereo             = FALSE;
        swapDesc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapDesc.BufferCount        = SwapChainBufferCount;
        swapDesc.Flags              = m_swapChainFlags;
        swapDesc.Scaling            = DXGI_SCALING_STRETCH;
        if (IsWindows10OrGreater())
        {
            swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        }
        else
        {
            swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        }

#ifdef __ALLOW_TEARING__
        {
            IDXGIFactory5* factory5 = nullptr;
            if (SUCCEEDED(dxgiFactory->QueryInterface(IID_PPV_ARGS(&factory5))))
            {
                factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &m_allowTearing, sizeof(m_allowTearing));
                factory5->Release();
            }
            if (m_allowTearing && swapDesc.SwapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD)
            {
                m_swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
                swapDesc.Flags = m_swapChainFlags;
            }
        }
#endif

#ifdef __WINRT__
        if (IsWindows8OrGreater())
        {
            swapDesc.Scaling = DXGI_SCALING_NONE;
        }
        else
        {
            swapDesc.Scaling = DXGI_SCALING_STRETCH;
        }
#endif
#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
        swapDesc.Scaling    = DXGI_SCALING_STRETCH;
        swapDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
#endif
        // this->sd.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
        if (coreWindow)
        {
            hr = dxgiFactory->CreateSwapChainForCoreWindow(m_device, coreWindow, &swapDesc, nullptr, &m_swapChain);
        }
        else
        {
            hr = dxgiFactory->CreateSwapChainForHwnd(m_device, window, &swapDesc, nullptr, nullptr, &m_swapChain);
        }
    }
    if (SUCCEEDED(hr))
    {
        this->applySwapChainColorSpace();
    }

    // Set up frame latency waitable object for non-blocking Present.
    if (SUCCEEDED(hr))
    {
        IDXGISwapChain2* swapChain2 = nullptr;
        if (SUCCEEDED(m_swapChain->QueryInterface(IID_PPV_ARGS(&swapChain2))))
        {
            swapChain2->SetMaximumFrameLatency(1);
            m_frameLatencyWaitableObject = swapChain2->GetFrameLatencyWaitableObject();
            swapChain2->Release();
        }
    }

    // Disable Alt+Enter fullscreen toggle, PrintScreen and window message snooping.
    if (dxgiFactory && window)
    {
        dxgiFactory->MakeWindowAssociation(window, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_PRINT_SCREEN);
    }

    D3D_API_RELEASE(dxgiDevice);
    D3D_API_RELEASE(dxgiAdapter);
    D3D_API_RELEASE(dxgiFactory);

    if (FAILED(hr))
    {
        Logger::error("Failed init D3D11 {:#010x}", hr);
        unInitDX();

        char errorText[256] = { 0 };
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, hr, 0, errorText, sizeof(errorText), nullptr);
        MessageBox(window, errorText, "Init D3D11 Failed", MB_ICONERROR);
        ExitProcess(hr);
        return false;
    }

    m_getDpiForWindow = (UINT(WINAPI*)(HWND)) GetProcAddress(GetModuleHandleW(L"USER32.DLL"), "GetDpiForWindow");

    // Cache tearing support state for Present
#ifdef __ALLOW_TEARING__
    m_allowTearing = !!m_allowTearing;
#else
    m_allowTearing = false;
#endif

    return true;
}

bool D3D11Context::applySwapChainColorSpace()
{
    m_colorSpace = getColorSpaceForHDR(m_hdrEnabled);

    IDXGISwapChain3* swapChain3 = nullptr;
    HRESULT hr                  = m_swapChain->QueryInterface(IID_PPV_ARGS(&swapChain3));
    if (FAILED(hr))
    {
        if (m_hdrEnabled)
        {
            Logger::error("D3D11: HDR requires IDXGISwapChain3 support: {:#010x}", hr);
            return false;
        }
        return true;
    }

    UINT colorSpaceSupport = 0;

    hr = swapChain3->CheckColorSpaceSupport(m_colorSpace, &colorSpaceSupport);
    if (FAILED(hr) || !(colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT))
    {
        Logger::error("D3D11: Swapchain color space {} is not supported: {:#010x}", static_cast<int>(m_colorSpace), hr);
        D3D_API_RELEASE(swapChain3);
        return false;
    }

    hr = swapChain3->SetColorSpace1(m_colorSpace);
    D3D_API_RELEASE(swapChain3);
    if (FAILED(hr))
    {
        Logger::error("D3D11: Failed to set swapchain color space {}: {:#010x}", static_cast<int>(m_colorSpace), hr);
        return false;
    }

    return true;
}

bool D3D11Context::setHDREnabled(bool enabled)
{
    if (m_hdrEnabled == enabled)
    {
        return true;
    }

    const bool previousHdrEnabled    = m_hdrEnabled;
    const DXGI_FORMAT previousFormat = m_swapChainFormat;
    const auto previousColorSpace    = m_colorSpace;

    m_hdrEnabled      = enabled;
    m_swapChainFormat = getSwapChainFormatForHDR(enabled);

    if (m_framebufferWidth <= 0 || m_framebufferHeight <= 0)
    {
        if (applySwapChainColorSpace())
        {
            Logger::info("D3D11: HDR output {}", enabled ? "enabled" : "disabled");
            return true;
        }
    }
    else if (onFramebufferSize(m_framebufferWidth, m_framebufferHeight))
    {
        Logger::info("D3D11: HDR output {}", enabled ? "enabled" : "disabled");
        return true;
    }

    m_hdrEnabled      = previousHdrEnabled;
    m_swapChainFormat = previousFormat;
    m_colorSpace      = previousColorSpace;
    if (m_framebufferWidth > 0 && m_framebufferHeight > 0)
    {
        onFramebufferSize(m_framebufferWidth, m_framebufferHeight);
    }
    else
    {
        applySwapChainColorSpace();
    }
    return false;
}

void D3D11Context::unInitDX()
{
    if (m_frameLatencyWaitableObject)
    {
        CloseHandle(m_frameLatencyWaitableObject);
        m_frameLatencyWaitableObject = nullptr;
    }

    // Detach RTs
    if (m_deviceContext)
    {
        ID3D11RenderTargetView* viewList[1] = { nullptr };
        m_deviceContext->OMSetRenderTargets(1, viewList, nullptr);
    }
    D3D_API_RELEASE(m_deviceContext);
    D3D_API_RELEASE(m_device);
    D3D_API_RELEASE(m_swapChain);
    D3D_API_RELEASE(m_renderTargetView);
    D3D_API_RELEASE(m_depthStencilView);
}

double D3D11Context::getScaleFactor()
{
#ifdef __WINRT__
    static auto displayInformation = winrt::Windows::Graphics::Display::DisplayInformation::GetForCurrentView();

    return (unsigned int) displayInformation.LogicalDpi() / 96.0f;
#else
    if (m_getDpiForWindow)
    {
        return m_getDpiForWindow(m_hWnd) / 96.0;
    }

    HDC hdc = GetDC(nullptr);
    if (hdc)
    {
        int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(nullptr, hdc);
        return dpi / 96.0;
    }

    return 1.0;
#endif
}

bool D3D11Context::onFramebufferSize(int width, int height, bool init)
{
    HRESULT hr = S_OK;

    ID3D11Texture2D* backBuffer         = nullptr;
    ID3D11Texture2D* depthStencil       = nullptr;
    ID3D11RenderTargetView* viewList[1] = { nullptr };
    m_deviceContext->OMSetRenderTargets(1, viewList, nullptr);

    D3D_API_RELEASE(m_renderTargetView);
    D3D_API_RELEASE(m_depthStencilView);

    if (!init)
    {
        hr = m_swapChain->ResizeBuffers(SwapChainBufferCount, width, height, m_swapChainFormat, m_swapChainFlags);
        if (FAILED(hr))
        {
            return false;
        }
    }

    hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(hr))
    {
        return false;
    }

    hr = m_device->CreateRenderTargetView(backBuffer, nullptr, &m_renderTargetView);
    D3D_API_RELEASE(backBuffer);
    if (FAILED(hr))
    {
        return false;
    }

    D3D11_TEXTURE2D_DESC texDesc;
    texDesc.ArraySize          = 1;
    texDesc.BindFlags          = D3D11_BIND_DEPTH_STENCIL;
    texDesc.CPUAccessFlags     = 0;
    texDesc.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
    texDesc.Height             = (UINT) height;
    texDesc.Width              = (UINT) width;
    texDesc.MipLevels          = 1;
    texDesc.MiscFlags          = 0;
    texDesc.SampleDesc.Count   = sampleDesc.Count;
    texDesc.SampleDesc.Quality = sampleDesc.Quality;
    texDesc.Usage              = D3D11_USAGE_DEFAULT;

    hr = m_device->CreateTexture2D(&texDesc, nullptr, &depthStencil);
    if (FAILED(hr))
    {
        return false;
    }

    D3D11_DEPTH_STENCIL_VIEW_DESC depthViewDesc;
    depthViewDesc.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthViewDesc.ViewDimension      = (sampleDesc.Count > 1) ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
    depthViewDesc.Flags              = 0;
    depthViewDesc.Texture2D.MipSlice = 0;

    hr = m_device->CreateDepthStencilView(depthStencil, &depthViewDesc, &m_depthStencilView);
    D3D_API_RELEASE(depthStencil);
    if (FAILED(hr))
    {
        return false;
    }

    D3D11_VIEWPORT viewport;
    viewport.Width    = (float) width;
    viewport.Height   = (float) height;
    viewport.MaxDepth = 1.0f;
    viewport.MinDepth = 0.0f;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    m_deviceContext->RSSetViewports(1, &viewport);

    if (!applySwapChainColorSpace())
    {
        return false;
    }

    m_framebufferWidth  = width;
    m_framebufferHeight = height;

    return true;
}

void D3D11Context::clear(NVGcolor color)
{
    float clearColor[4] = { color.r, color.g, color.b, color.a };
    m_deviceContext->ClearRenderTargetView(m_renderTargetView, clearColor);
    m_deviceContext->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0.0f, 0);
}

void D3D11Context::beginFrame()
{
    // Wait for the swap chain to signal it's ready for a new frame.
    // This blocks here (before any GPU work) instead of inside Present.
    if (m_frameLatencyWaitableObject && m_swapInterval > 0)
    {
        WaitForSingleObjectEx(m_frameLatencyWaitableObject, 1000, TRUE);
    }

    m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);
}

void D3D11Context::endFrame()
{
    DXGI_PRESENT_PARAMETERS presentParameters;
    ZeroMemory(&presentParameters, sizeof(DXGI_PRESENT_PARAMETERS));

#ifdef __ALLOW_TEARING__
    if (m_allowTearing && m_swapInterval == 0)
    {
        m_swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
        return;
    }
#endif

    if (m_frameLatencyWaitableObject && m_swapInterval > 0)
    {
        // Present(0) with flip-model doesn't tear - DWM composites at vsync!
        m_swapChain->Present1(0, 0, &presentParameters);
    }
    else
    {
        m_swapChain->Present1(m_swapInterval, 0, &presentParameters);
    }
}

void D3D11Context::setSwapInterval(int interval)
{
    if (interval < 0 || interval > 4)
        return;

    m_swapInterval = interval;
}

void D3D11Context::setAllowTearing(bool tearing)
{
#ifndef __ALLOW_TEARING__
    Logger::warning("D3D11: Failed to set allow tearing, because its disabled internaly.");
    return;
#endif
    m_allowTearing = tearing;
}
}
