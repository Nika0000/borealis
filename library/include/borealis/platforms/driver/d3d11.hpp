/*
    Copyright 2023 zeromake

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#pragma once
#include <d3d11.h>
#include <d3d11_1.h>
#include <dxgi1_6.h>
#include <nanovg.h>

#ifdef __GLFW__
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#elif defined(__SDL3__)
#include <SDL3/SDL.h>
#endif

namespace brls
{

/** Direct3D 11 rendering context with swap chain, HDR, and tearing support. */
class D3D11Context
{
  public:
    /**
     * Creates a D3D11 device and swap chain for the given window.
     * @param window platform window handle
     * @param width initial framebuffer width in pixels
     * @param height initial framebuffer height in pixels
     */
#ifdef __GLFW__
    D3D11Context(GLFWwindow* window, int width, int height);
#elif defined(__SDL3__)
    D3D11Context(SDL_Window* window, int width, int height);
#endif
    ~D3D11Context();

    /** Returns the DPI scale factor for the window. */
    double getScaleFactor();

    /** Clears the render target with the specified color. */
    void clear(NVGcolor color);

    /** Begins a new render frame (binds render target and sets viewport). */
    void beginFrame();

    /** Ends the current frame and presents the swap chain. */
    void endFrame();

    /**
     * Sets the swap chain present interval.
     * @param interval 0 for immediate (no vsync), 1+ for vsync
     */
    void setSwapInterval(int interval);

    /**
     * Enables or disables DXGI tearing (variable refresh rate).
     * @param tearing true to allow tearing when supported
     */
    void setAllowTearing(bool tearing);

    /**
     * Enables or disables HDR output on the swap chain.
     * @param enabled true to switch to an HDR color space
     * @return true if the HDR state was changed successfully
     */
    bool setHDREnabled(bool enabled);

    /** Returns whether HDR output is currently enabled. */
    bool isHDREnabled() const { return m_hdrEnabled; }

    /** Returns whether the window is currently occluded (minimized/hidden/cloaked). */
    bool isOccluded() const { return m_occluded; }

    /**
     * Refreshes the cached occlusion state from the current window state and, on
     * a transition.
     */
    void hitOcclusion();

    /**
     * Handles framebuffer resize by recreating the swap chain buffers.
     * @param width new framebuffer width in pixels
     * @param height new framebuffer height in pixels
     * @param init true when called during initial setup
     * @return true if the resize succeeded
     */
    bool onFramebufferSize(int width, int height, bool init = false);

    /** Returns the underlying D3D11 device. */
    ID3D11Device* getDevice() { return m_device; }

    /** Returns the underlying DXGI swap chain. */
    IDXGISwapChain* getSwapChain() { return m_swapChain; }

  private:
    ID3D11Device* m_device                     = nullptr;
    ID3D11DeviceContext* m_deviceContext       = nullptr;
    IDXGISwapChain1* m_swapChain               = nullptr;
    ID3D11RenderTargetView* m_renderTargetView = nullptr;
    ID3D11DepthStencilView* m_depthStencilView = nullptr;

    int m_swapInterval  = 1;
    bool m_allowTearing = false;
    bool m_hdrEnabled   = false;

    int m_framebufferWidth  = 0;
    int m_framebufferHeight = 0;

    UINT m_swapChainFlags = 0;

    HANDLE m_frameLatencyWaitableObject = nullptr;

    DXGI_FORMAT m_swapChainFormat      = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_COLOR_SPACE_TYPE m_colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;

    UINT(WINAPI* m_getDpiForWindow)(HWND);

    HWND m_hWnd = nullptr;
#ifdef __SDL3__
    uint32_t m_windowID = 0;
#endif

    bool m_occluded = false;

    bool initDX(HWND window, IUnknown* coreWindow, int width, int height);
    bool applySwapChainColorSpace();
    void unInitDX();
};

}
