/**
 * @file tools/dxgi.cpp
 * @brief Displays information about connected displays and GPUs
 */
#define WINVER 0x0A00
#include <d3dcommon.h>
#include <Windows.h>
#include <dxgi.h>

#include <iostream>
#include <unordered_map>

#include "src/utility.h"

using namespace std::literals;
namespace dxgi {
  template <class T>
  void
  Release(T *dxgi) {
    dxgi->Release();
  }

  using factory1_t = util::safe_ptr<IDXGIFactory1, Release<IDXGIFactory1>>;
  using adapter_t = util::safe_ptr<IDXGIAdapter1, Release<IDXGIAdapter1>>;
  using output_t = util::safe_ptr<IDXGIOutput, Release<IDXGIOutput>>;

}  // namespace dxgi


std::unordered_map<std::wstring, std::wstring> getMonitorDisplayToNameMap() {
    std::unordered_map<std::wstring, std::wstring> result;
    DISPLAY_DEVICEW dd;
    dd.cb = sizeof(dd);
    int deviceIndex = 0;
    while(EnumDisplayDevicesW(0, deviceIndex, &dd, 0))
    {
        std::wstring deviceName = dd.DeviceName;
        while(EnumDisplayDevicesW(deviceName.c_str(), 0, &dd, 0))
        {
            result[deviceName] = dd.DeviceString;
            break;
        }
        ++deviceIndex;
    }
    return result;
} 

int
main(int argc, char *argv[]) {
  auto displays = getMonitorDisplayToNameMap();
  
  HRESULT status;

  // Set ourselves as per-monitor DPI aware for accurate resolution values on High DPI systems
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

  dxgi::factory1_t::pointer factory_p {};
  status = CreateDXGIFactory1(IID_IDXGIFactory1, (void **) &factory_p);
  dxgi::factory1_t factory { factory_p };
  if (FAILED(status)) {
    std::cout << "Failed to create DXGIFactory1 [0x"sv << util::hex(status).to_string_view() << ']' << std::endl;
    return -1;
  }
  

  dxgi::adapter_t::pointer adapter_p {};
  for (int x = 0; factory->EnumAdapters1(x, &adapter_p) != DXGI_ERROR_NOT_FOUND; ++x) {
    dxgi::adapter_t adapter { adapter_p };

    DXGI_ADAPTER_DESC1 adapter_desc;
    adapter->GetDesc1(&adapter_desc);

    std::cout
      << "====== ADAPTER ====="sv << std::endl;
    std::wcout
      << L"Device Name      : "sv << adapter_desc.Description << std::endl;
    std::cout
      << "Device Vendor ID : 0x"sv << util::hex(adapter_desc.VendorId).to_string_view() << std::endl
      << "Device Device ID : 0x"sv << util::hex(adapter_desc.DeviceId).to_string_view() << std::endl
      << "Device Video Mem : "sv << adapter_desc.DedicatedVideoMemory / 1048576 << " MiB"sv << std::endl
      << "Device Sys Mem   : "sv << adapter_desc.DedicatedSystemMemory / 1048576 << " MiB"sv << std::endl
      << "Share Sys Mem    : "sv << adapter_desc.SharedSystemMemory / 1048576 << " MiB"sv << std::endl
      << std::endl
      << "    ====== OUTPUT ======"sv << std::endl;

    dxgi::output_t::pointer output_p {};
    for (int y = 0; adapter->EnumOutputs(y, &output_p) != DXGI_ERROR_NOT_FOUND; ++y) {
      dxgi::output_t output { output_p };

      DXGI_OUTPUT_DESC desc;
      output->GetDesc(&desc);

      auto width = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
      auto height = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;


      auto it = displays.find(std::wstring(desc.DeviceName));

      std::wcout
        << L"    Output Name       : "sv << desc.DeviceName << std::endl;
      if(it != displays.end()) {
        std::wcout
          << L"    Monitor Name      : "sv << it->second << std::endl;
      }
      std::cout
        << "    AttachedToDesktop : "sv << (desc.AttachedToDesktop ? "yes"sv : "no"sv) << std::endl
        << "    Resolution        : "sv << width << 'x' << height << std::endl
        << std::endl;
    }
  }

  return 0;
}
