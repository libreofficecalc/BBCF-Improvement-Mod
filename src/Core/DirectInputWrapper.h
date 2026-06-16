#pragma once

#include <dinput.h>

class DirectInput8AWrapper : public IDirectInput8A
{
public:
        explicit DirectInput8AWrapper(IDirectInput8A* original);

        /*** IUnknown methods ***/
        STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID* ppvObj) override;
        STDMETHOD_(ULONG, AddRef)(THIS) override;
        STDMETHOD_(ULONG, Release)(THIS) override;

        /*** IDirectInput8A methods ***/
        STDMETHOD(CreateDevice)(THIS_ REFGUID, LPDIRECTINPUTDEVICE8A*, LPUNKNOWN) override;
        STDMETHOD(EnumDevices)(THIS_ DWORD, LPDIENUMDEVICESCALLBACKA, LPVOID, DWORD) override;
        STDMETHOD(GetDeviceStatus)(THIS_ REFGUID) override;
        STDMETHOD(RunControlPanel)(THIS_ HWND, DWORD) override;
        STDMETHOD(Initialize)(THIS_ HINSTANCE, DWORD) override;
        STDMETHOD(FindDevice)(THIS_ REFGUID, LPCSTR, LPGUID) override;
        STDMETHOD(EnumDevicesBySemantics)(THIS_ LPCSTR, LPDIACTIONFORMATA, LPDIENUMDEVICESBYSEMANTICSCBA, LPVOID, DWORD) override;
        STDMETHOD(ConfigureDevices)(THIS_ LPDICONFIGUREDEVICESCALLBACK, LPDICONFIGUREDEVICESPARAMSA, DWORD, LPVOID) override;

private:
        ULONG m_refCount = 1;
        IDirectInput8A* m_original;
};

class DirectInput8WWrapper : public IDirectInput8W
{
public:
        explicit DirectInput8WWrapper(IDirectInput8W* original);

        /*** IUnknown methods ***/
        STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID* ppvObj) override;
        STDMETHOD_(ULONG, AddRef)(THIS) override;
        STDMETHOD_(ULONG, Release)(THIS) override;

        /*** IDirectInput8W methods ***/
        STDMETHOD(CreateDevice)(THIS_ REFGUID, LPDIRECTINPUTDEVICE8W*, LPUNKNOWN) override;
        STDMETHOD(EnumDevices)(THIS_ DWORD, LPDIENUMDEVICESCALLBACKW, LPVOID, DWORD) override;
        STDMETHOD(GetDeviceStatus)(THIS_ REFGUID) override;
        STDMETHOD(RunControlPanel)(THIS_ HWND, DWORD) override;
        STDMETHOD(Initialize)(THIS_ HINSTANCE, DWORD) override;
        STDMETHOD(FindDevice)(THIS_ REFGUID, LPCWSTR, LPGUID) override;
        STDMETHOD(EnumDevicesBySemantics)(THIS_ LPCWSTR, LPDIACTIONFORMATW, LPDIENUMDEVICESBYSEMANTICSCBW, LPVOID, DWORD) override;
        STDMETHOD(ConfigureDevices)(THIS_ LPDICONFIGUREDEVICESCALLBACK, LPDICONFIGUREDEVICESPARAMSW, DWORD, LPVOID) override;

private:
        ULONG m_refCount = 1;
        IDirectInput8W* m_original;
};
