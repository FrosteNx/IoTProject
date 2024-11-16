#include "Wifi.h"

#include <esp_mac.h>

namespace WIFI
{

const char* Wifi::get_mac(void)
{
    static char mac[];
    uint8_t mac_byte_buffer[];

    const esp_err_t status{esp_efuse_mac_get_default()};
}

} // namespace WIFI