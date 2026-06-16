#include "DirectInputWrapper.h"

#include "ControllerOverrideManager.h"
#include "logger.h"

#include <vector>

DirectInput8AWrapper::DirectInput8AWrapper(IDirectInput8A* original)
        : m_original(original)
{
}

HRESULT STDMETHODCALLTYPE DirectInput8AWrapper::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
        if (riid == IID_IDirectInput8A || riid == IID_IUnknown)
        {
                *ppvObj = this;
                AddRef();
                return DI_OK;
        }

        return m_original->QueryInterface(riid, ppvObj);
}

ULONG STDMETHODCALLTYPE DirectInput8AWrapper::AddRef()
{
        ++m_refCount;
        return m_original->AddRef();
}

ULONG STDMETHODCALLTYPE DirectInput8AWrapper::Release()
{
        ULONG ref = m_original->Release();
        if (--m_refCount == 0)
        {
                delete this;
        }
        return ref;
}

HRESULT STDMETHODCALLTYPE DirectInput8AWrapper::CreateDevice(REFGUID rguid, LPDIRECTINPUTDEVICE8A* lplpDirectInputDevice, LPUNKNOWN pUnkOuter)
{
        LOG(1, "DirectInput8AWrapper::CreateDevice - guid=%s\n", GuidToString(rguid).c_str());
        if (!ControllerOverrideManager::GetInstance().IsDeviceAllowed(rguid))
        {
                return DIERR_DEVICENOTREG;
        }

        HRESULT result = m_original->CreateDevice(rguid, lplpDirectInputDevice, pUnkOuter);
        LOG(1, "DirectInput8AWrapper::CreateDevice - hr=0x%08X device=%p\n", result, lplpDirectInputDevice ? *lplpDirectInputDevice : nullptr);
        if (SUCCEEDED(result) && lplpDirectInputDevice && *lplpDirectInputDevice)
        {
                ControllerOverrideManager::GetInstance().RegisterCreatedDevice(*lplpDirectInputDevice);
        }

        return result;
}

HRESULT STDMETHODCALLTYPE DirectInput8AWrapper::EnumDevices(DWORD dwDevType, LPDIENUMDEVICESCALLBACKA lpCallback, LPVOID pvRef, DWORD dwFlags)
{
        LOG(1, "DirectInput8AWrapper::EnumDevices - type=0x%08X flags=0x%08X\n", dwDevType, dwFlags);
        std::vector<DIDEVICEINSTANCEA> devices;
        auto collector = [](const DIDEVICEINSTANCEA* inst, LPVOID ref) -> BOOL {
                auto* list = reinterpret_cast<std::vector<DIDEVICEINSTANCEA>*>(ref);
                list->push_back(*inst);
                return DIENUM_CONTINUE;
        };

        HRESULT result = m_original->EnumDevices(dwDevType, collector, &devices, dwFlags);
        LOG(1, "DirectInput8AWrapper::EnumDevices - hr=0x%08X collected=%zu\n", result, devices.size());

        if (FAILED(result) || !lpCallback)
        {
                return result;
        }

        ControllerOverrideManager::GetInstance().ApplyOrdering(devices);

        for (const auto& device : devices)
        {
                if (lpCallback(&device, pvRef) == DIENUM_STOP)
                {
                        break;
                }
        }

        return result;
}

HRESULT STDMETHODCALLTYPE DirectInput8AWrapper::GetDeviceStatus(REFGUID rguidInstance)
{
        return m_original->GetDeviceStatus(rguidInstance);
}

HRESULT STDMETHODCALLTYPE DirectInput8AWrapper::RunControlPanel(HWND hwndOwner, DWORD dwFlags)
{
        return m_original->RunControlPanel(hwndOwner, dwFlags);
}

HRESULT STDMETHODCALLTYPE DirectInput8AWrapper::Initialize(HINSTANCE hinst, DWORD dwVersion)
{
        return m_original->Initialize(hinst, dwVersion);
}

HRESULT STDMETHODCALLTYPE DirectInput8AWrapper::FindDevice(REFGUID rguidClass, LPCSTR ptszName, LPGUID pguidInstance)
{
        return m_original->FindDevice(rguidClass, ptszName, pguidInstance);
}

HRESULT STDMETHODCALLTYPE DirectInput8AWrapper::EnumDevicesBySemantics(LPCSTR ptszUserName, LPDIACTIONFORMATA lpdiActionFormat, LPDIENUMDEVICESBYSEMANTICSCBA lpCallback, LPVOID pvRef, DWORD dwFlags)
{
        return m_original->EnumDevicesBySemantics(ptszUserName, lpdiActionFormat, lpCallback, pvRef, dwFlags);
}

HRESULT STDMETHODCALLTYPE DirectInput8AWrapper::ConfigureDevices(LPDICONFIGUREDEVICESCALLBACK lpdiCallback, LPDICONFIGUREDEVICESPARAMSA lpdiCDParams, DWORD dwFlags, LPVOID pvRefData)
{
        return m_original->ConfigureDevices(lpdiCallback, lpdiCDParams, dwFlags, pvRefData);
}

DirectInput8WWrapper::DirectInput8WWrapper(IDirectInput8W* original)
        : m_original(original)
{
}

HRESULT STDMETHODCALLTYPE DirectInput8WWrapper::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
        if (riid == IID_IDirectInput8W || riid == IID_IUnknown)
        {
                *ppvObj = this;
                AddRef();
                return DI_OK;
        }

        return m_original->QueryInterface(riid, ppvObj);
}

ULONG STDMETHODCALLTYPE DirectInput8WWrapper::AddRef()
{
        ++m_refCount;
        return m_original->AddRef();
}

ULONG STDMETHODCALLTYPE DirectInput8WWrapper::Release()
{
        ULONG ref = m_original->Release();
        if (--m_refCount == 0)
        {
                delete this;
        }
        return ref;
}

HRESULT STDMETHODCALLTYPE DirectInput8WWrapper::CreateDevice(REFGUID rguid, LPDIRECTINPUTDEVICE8W* lplpDirectInputDevice, LPUNKNOWN pUnkOuter)
{
        LOG(1, "DirectInput8WWrapper::CreateDevice - guid=%s\n", GuidToString(rguid).c_str());
        if (!ControllerOverrideManager::GetInstance().IsDeviceAllowed(rguid))
        {
                return DIERR_DEVICENOTREG;
        }

        HRESULT result = m_original->CreateDevice(rguid, lplpDirectInputDevice, pUnkOuter);
        LOG(1, "DirectInput8WWrapper::CreateDevice - hr=0x%08X device=%p\n", result, lplpDirectInputDevice ? *lplpDirectInputDevice : nullptr);
        if (SUCCEEDED(result) && lplpDirectInputDevice && *lplpDirectInputDevice)
        {
                ControllerOverrideManager::GetInstance().RegisterCreatedDevice(*lplpDirectInputDevice);
        }

        return result;
}

HRESULT STDMETHODCALLTYPE DirectInput8WWrapper::EnumDevices(DWORD dwDevType, LPDIENUMDEVICESCALLBACKW lpCallback, LPVOID pvRef, DWORD dwFlags)
{
        LOG(1, "DirectInput8WWrapper::EnumDevices - type=0x%08X flags=0x%08X\n", dwDevType, dwFlags);
        std::vector<DIDEVICEINSTANCEW> devices;
        auto collector = [](const DIDEVICEINSTANCEW* inst, LPVOID ref) -> BOOL {
                auto* list = reinterpret_cast<std::vector<DIDEVICEINSTANCEW>*>(ref);
                list->push_back(*inst);
                return DIENUM_CONTINUE;
        };

        HRESULT result = m_original->EnumDevices(dwDevType, collector, &devices, dwFlags);
        LOG(1, "DirectInput8WWrapper::EnumDevices - hr=0x%08X collected=%zu\n", result, devices.size());

        if (FAILED(result) || !lpCallback)
        {
                return result;
        }

        ControllerOverrideManager::GetInstance().ApplyOrdering(devices);

        for (const auto& device : devices)
        {
                if (lpCallback(&device, pvRef) == DIENUM_STOP)
                {
                        break;
                }
        }

        return result;
}

HRESULT STDMETHODCALLTYPE DirectInput8WWrapper::GetDeviceStatus(REFGUID rguidInstance)
{
        return m_original->GetDeviceStatus(rguidInstance);
}

HRESULT STDMETHODCALLTYPE DirectInput8WWrapper::RunControlPanel(HWND hwndOwner, DWORD dwFlags)
{
        return m_original->RunControlPanel(hwndOwner, dwFlags);
}

HRESULT STDMETHODCALLTYPE DirectInput8WWrapper::Initialize(HINSTANCE hinst, DWORD dwVersion)
{
        return m_original->Initialize(hinst, dwVersion);
}

HRESULT STDMETHODCALLTYPE DirectInput8WWrapper::FindDevice(REFGUID rguidClass, LPCWSTR ptszName, LPGUID pguidInstance)
{
        return m_original->FindDevice(rguidClass, ptszName, pguidInstance);
}

HRESULT STDMETHODCALLTYPE DirectInput8WWrapper::EnumDevicesBySemantics(LPCWSTR ptszUserName, LPDIACTIONFORMATW lpdiActionFormat, LPDIENUMDEVICESBYSEMANTICSCBW lpCallback, LPVOID pvRef, DWORD dwFlags)
{
        return m_original->EnumDevicesBySemantics(ptszUserName, lpdiActionFormat, lpCallback, pvRef, dwFlags);
}

HRESULT STDMETHODCALLTYPE DirectInput8WWrapper::ConfigureDevices(LPDICONFIGUREDEVICESCALLBACK lpdiCallback, LPDICONFIGUREDEVICESPARAMSW lpdiCDParams, DWORD dwFlags, LPVOID pvRefData)
{
        return m_original->ConfigureDevices(lpdiCallback, lpdiCDParams, dwFlags, pvRefData);
}
