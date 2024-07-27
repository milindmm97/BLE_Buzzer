#include "nrf_fstorage.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_fstorage_sd.h"
#include "nrf_delay.h"


#define FSTORAGE_START_ADDR    0x40000
#define FSTORAGE_END_ADDR      FSTORAGE_START_ADDR + 0x1000

#define MAC_FS_ADDR            FSTORAGE_START_ADDR

ble_gap_addr_t read_mac_address_from_flash();