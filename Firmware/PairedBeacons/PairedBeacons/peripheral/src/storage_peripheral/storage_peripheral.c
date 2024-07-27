#include "storage_peripheral.h"
#include "fds.h"


#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#define FLASH_PAGE_COUNT 1 // Number of flash pages needed for the data section

//NRF_FSTORAGE_DEF(nrf_fstorage_t fstorage);
static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt);



NRF_FSTORAGE_DEF(nrf_fstorage_t fstorage) =
{
    /* Set a handler for fstorage events. */
    .evt_handler = fstorage_evt_handler,

    /* These below are the boundaries of the flash space assigned to this instance of fstorage.
     * You must set these manually, even at runtime, before nrf_fstorage_init() is called.
     * The function nrf5_flash_end_addr_get() can be used to retrieve the last address on the
     * last page of flash available to write data. */
    .start_addr = FSTORAGE_START_ADDR,
    .end_addr   = FSTORAGE_END_ADDR,
};


/**@brief   Sleep until an event is received. */
static void power_manage(void)
{
  if (NRF_LOG_PROCESS() == false)
    {

  #ifdef SOFTDEVICE_PRESENT
    if (nrf_sdh_is_enabled())
    {
        ret_code_t ret_code = sd_app_evt_wait();
        ASSERT((ret_code == NRF_SUCCESS) || (ret_code == NRF_ERROR_SOFTDEVICE_NOT_ENABLED));
        UNUSED_VARIABLE(ret_code);
    }
    else
#endif // SOFTDEVICE_PRESENT
    {
        // Wait for an event.
        __WFE();
        // Clear the internal event register.
        __SEV();
        __WFE();
    }
    }
}

void wait_for_flash_ready(nrf_fstorage_t const * p_fstorage)
{
    /* While fstorage is busy, sleep and wait for an event. */
    while (nrf_fstorage_is_busy(p_fstorage))
    {
        power_manage();
    }
    //nrf_delay_ms(100);
}
static uint32_t flash_end_address(void)
{
    // Get the bootloader start address from the UICR register
    uint32_t bootloader_start_address = NRF_UICR->NRFFW[0];

    // Check if a bootloader is present
    if (bootloader_start_address == 0xFFFFFFFF)
    {
        // If no bootloader is present, return the end address of the flash memory
        return NRF_FICR->CODEPAGESIZE * NRF_FICR->CODESIZE;
    }
    else
    {
        // If a bootloader is present, return the start address of the bootloader
        return bootloader_start_address;
    }
}


static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt)
  
{
    if (p_evt->result != NRF_SUCCESS)
    {
        NRF_LOG_INFO("--> Event received: ERROR while executing an fstorage operation.");
        return;
    }

    switch (p_evt->id)
    {
        //case NRF_FSTORAGE_EVT_WRITE_RESULT:
        //{
        //    NRF_LOG_INFO("--> Event received: wrote %d bytes at address 0x%x.",
        //                  p_evt->len, p_evt->addr);
        //} break;

        case NRF_FSTORAGE_EVT_ERASE_RESULT:
        {
           NRF_LOG_INFO("--> Event received: erased %d page from address 0x%x.",
                          p_evt->len, p_evt->addr);
        } break;

        default:
            break;
    }
}




void peripheral_fstorage_init()
{
    ret_code_t rc;
    nrf_fstorage_api_t * p_fs_api;
    p_fs_api = &nrf_fstorage_sd;
    NRF_LOG_INFO("Initializing Fstorage");
    rc = nrf_fstorage_init(&fstorage, p_fs_api, NULL);

    
    APP_ERROR_CHECK(rc);
}


void peripheral_fstorage_uninit()
{
    nrf_fstorage_uninit(&fstorage,NULL);
}

void fstorage_clear_page()
{
    ret_code_t err_code = nrf_fstorage_erase(&fstorage, FSTORAGE_START_ADDR, 1, NULL);
    APP_ERROR_CHECK(err_code);
    wait_for_flash_ready(&fstorage);
    NRF_LOG_INFO("Erased Mac");
}

void log_ble_gap_addr_t(ble_gap_addr_t mac_address) {
    //NRF_LOG_INFO("Mac at %x", mac_address.addr);
    int n = 1;
    NRF_LOG_INFO("BLE Address: %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_address.addr[0], mac_address.addr[1], mac_address.addr[2],
                 mac_address.addr[3], mac_address.addr[4], mac_address.addr[5]);
}

ble_gap_addr_t combine_buffer_to_mac_address(uint8_t buffer[6]) {
    ble_gap_addr_t mac_address;

    for(int i=0; i<6; i++)
        mac_address.addr[i] = buffer[i];
    return mac_address;
}


void save_mac_address_to_flash(ble_gap_addr_t mac_address) {
    int32_t buffer[6];
    ret_code_t rc;

    NRF_LOG_INFO("just inside save_mac : %x", &mac_address.addr[0]);
    log_ble_gap_addr_t(mac_address);

   
    fstorage_clear_page();
    NRF_LOG_INFO("Before writing to flash");
    log_ble_gap_addr_t(combine_buffer_to_mac_address(mac_address.addr));

    for(int i=0; i<6; i++){
        uint32_t data = (uint32_t)mac_address.addr[i];
        rc = nrf_fstorage_write(&fstorage, MAC_FS_ADDR+(i*0x100), &data, sizeof(uint32_t), NULL);
        APP_ERROR_CHECK(rc);
        //nrf_delay_ms(10);
        wait_for_flash_ready(&fstorage);
    }
}

ble_gap_addr_t read_mac_address_from_flash() {
    ble_gap_addr_t mac_address;
    ret_code_t rc;
    for(int i=0; i<6; i++){
        uint32_t address = MAC_FS_ADDR+(i*0x100);
        uint32_t data;
        nrf_fstorage_read(&fstorage, address ,&data , sizeof(uint32_t));
        mac_address.addr[i] = data;
    }
   

    NRF_LOG_INFO("Read from flash");

    log_ble_gap_addr_t(mac_address);

    return mac_address;

}


bool is_mac_address_available(){
    ble_gap_addr_t mac_address;
    bool is_available = false;
    mac_address = read_mac_address_from_flash();
    for(int i=0; i<6; i++){
        if(mac_address.addr[i] != 0xFF)
            is_available = true;
    }
    NRF_LOG_INFO("Is available : %d", is_available);
    return is_available;
}

// A function to match the mac address
bool is_mac_address_match(ble_gap_addr_t mac_address){
    ble_gap_addr_t mac_address_flash;
    bool is_match = true;
    mac_address_flash = read_mac_address_from_flash();
    NRF_LOG_INFO("Mac address from flash")
    log_ble_gap_addr_t(mac_address_flash);
    NRF_LOG_INFO("Mac address ble")
    log_ble_gap_addr_t(mac_address);
    for(int i=0; i<6; i++){
        if(mac_address.addr[i] != mac_address_flash.addr[i])
            is_match = false;
            break;
    }
    NRF_LOG_INFO("Is match : %d", is_match);
    return is_match;
}