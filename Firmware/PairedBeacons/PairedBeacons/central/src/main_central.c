/**
 * Copyright (c) 2014 - 2019, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/**
 * @brief BLE LED Button Service central and client application main file.
 *
 * This file contains the source code for a sample client application using the LED Button service.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "nrf_pwr_mgmt.h"
#include "app_timer.h"
#include "boards.h"
#include "bsp.h"
#include "bsp_btn_ble.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "ble_db_discovery.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_scan.h"
#include "nrf_sdh.h"
#include "ble_gap.h"
#include "nrf_power.h"
#include "ble_central.h"
#include "storage_central.h"



#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"


#define CENTRAL_SCANNING_LED            BSP_BOARD_LED_0                     /**< Scanning LED will be on when the device is scanning. */
#define CENTRAL_CONNECTED_LED           BSP_BOARD_LED_1                     /**< Connected LED will be on when the device is connected. */
#define LEDBUTTON_LED                   BSP_BOARD_LED_2                     /**< LED to indicate a change of state of the the Button characteristic on the peer. */

#define SCAN_INTERVAL                   0x00A0                              /**< Determines scan interval in units of 0.625 millisecond. */
#define SCAN_WINDOW                     0x0050                              /**< Determines scan window in units of 0.625 millisecond. */
#define SCAN_DURATION                   0x0000                              /**< Timout when scanning. 0x0000 disables timeout. */


#define MIN_CONNECTION_INTERVAL         MSEC_TO_UNITS(7.5, UNIT_1_25_MS)    /**< Determines minimum connection interval in milliseconds. */
#define MAX_CONNECTION_INTERVAL         MSEC_TO_UNITS(30, UNIT_1_25_MS)     /**< Determines maximum connection interval in milliseconds. */
#define SLAVE_LATENCY                   0                                   /**< Determines slave latency in terms of connection events. */
#define SUPERVISION_TIMEOUT             MSEC_TO_UNITS(4000, UNIT_10_MS)     /**< Determines supervision time-out in units of 10 milliseconds. */

#define LEDBUTTON_BUTTON_PIN            18                      /**< Button that will write to the LED characteristic of the peer */
#define BUTTON_DETECTION_DELAY          APP_TIMER_TICKS(50)                 /**< Delay from a GPIOTE event until a button is reported as pushed (in number of timer ticks). */

#define APP_BLE_CONN_CFG_TAG            1                                   /**< A tag identifying the SoftDevice BLE configuration. */
#define APP_BLE_OBSERVER_PRIO           3                                   /**< Application's BLE observer priority. You shouldn't need to modify this value. */
#define SCAN_DURATION_WITELIST          3000                                /**< Duration of the scanning in units of 10 milliseconds. */


#define LDO_PIN                         06
#define LED_PIN                         10


NRF_BLE_SCAN_DEF(m_scan);                                       /**< Scanning module instance. */
BLE_LBS_C_DEF(m_ble_lbs_c);                                     /**< Main structure used by the LBS client module. */
NRF_BLE_GATT_DEF(m_gatt);                                       /**< GATT module instance. */
BLE_DB_DISCOVERY_DEF(m_db_disc);                                /**< DB discovery module instance. */
NRF_BLE_GQ_DEF(m_ble_gatt_queue,                                /**< BLE GATT Queue instance. */
               NRF_SDH_BLE_CENTRAL_LINK_COUNT,
               NRF_BLE_GQ_QUEUE_SIZE);

APP_TIMER_DEF(m_ble_data_send_timer);
APP_TIMER_DEF(m_scanning_timeout);
APP_TIMER_DEF(m_power_off_timer);
APP_TIMER_DEF(m_erase_mac_timer);

static char const m_target_periph_name[] = "Peripheral";     /**< Name of the device we try to connect to. This name is searched in the scan report data*/
void get_peripheral_mac_address(uint16_t conn_handle);
/**< Scan parameters requested for scanning and connection. */
static ble_gap_scan_params_t const m_scan_param =
{
    .active        = 0x01,
#if (NRF_SD_BLE_API_VERSION > 7)
    .interval_us   = NRF_BLE_SCAN_SCAN_INTERVAL * UNIT_0_625_MS,
    .window_us     = NRF_BLE_SCAN_SCAN_WINDOW * UNIT_0_625_MS,
#else
    .interval      = NRF_BLE_SCAN_SCAN_INTERVAL,
    .window        = NRF_BLE_SCAN_SCAN_WINDOW,
#endif // (NRF_SD_BLE_API_VERSION > 7)
    .filter_policy = BLE_GAP_SCAN_FP_WHITELIST,
    .timeout       = SCAN_DURATION_WITELIST,
    .scan_phys     = BLE_GAP_PHY_1MBPS,
};



static ble_gap_addr_t mac_address;
bool just_connected = false;



/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{
    ret_code_t err_code;
    // Prepare wakeup buttons.
    err_code = bsp_btn_ble_sleep_mode_prepare();
    APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}
void enable_ldo()
{
    // NOTE Currently LDO is not being used to turn off rather nrf is set to deep sleep in function disable_ldo
    nrf_gpio_cfg_output(LDO_PIN);
    nrf_gpio_pin_set(LDO_PIN);
    nrf_gpio_cfg_output(LED_PIN);
    nrf_gpio_pin_set(LED_PIN);
    NRF_LOG_INFO("LDO enabled");
}

void disable_ldo()
{
    //NOTE: The button pin is set in bsp_btn_ble.c as BTN_ID_WAKEUP
    nrf_gpio_pin_clear(LDO_PIN);
    //nrf_gpio_pin_clear(LED_PIN);
    NRF_LOG_INFO("LDO disabled");
    //sleep_mode_enter();
}


/**@brief Function to handle asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num     Line number of the failing ASSERT call.
 * @param[in] p_file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(0xDEADBEEF, line_num, p_file_name);
}


/**@brief Function for handling the LED Button Service client errors.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void lbs_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for the LEDs initialization.
 *
 * @details Initializes all LEDs used by the application.
 */
static void leds_init(void)
{
    bsp_board_init(BSP_INIT_LEDS);
}


/**@brief Function to start scanning.
 */
static void scan_start(void)
{
    ret_code_t err_code;

    err_code = nrf_ble_scan_start(&m_scan);
    APP_ERROR_CHECK(err_code);

    bsp_board_led_off(CENTRAL_CONNECTED_LED);
    bsp_board_led_on(CENTRAL_SCANNING_LED);
}


/**@brief Handles events coming from the LED Button central module.
 */
static void lbs_c_evt_handler(ble_lbs_c_t * p_lbs_c, ble_lbs_c_evt_t * p_lbs_c_evt)
{
    switch (p_lbs_c_evt->evt_type)
    {
        case BLE_LBS_C_EVT_DISCOVERY_COMPLETE:
        {
            ret_code_t err_code;

            err_code = ble_lbs_c_handles_assign(&m_ble_lbs_c,
                                                p_lbs_c_evt->conn_handle,
                                                &p_lbs_c_evt->params.peer_db);
            NRF_LOG_INFO("LED Button service discovered on conn_handle 0x%x.", p_lbs_c_evt->conn_handle);

            err_code = app_button_enable();
            APP_ERROR_CHECK(err_code);

            // LED Button service discovered. Enable notification of Button.
            err_code = ble_lbs_c_button_notif_enable(p_lbs_c);
            APP_ERROR_CHECK(err_code);
        } break; // BLE_LBS_C_EVT_DISCOVERY_COMPLETE

        case BLE_LBS_C_EVT_BUTTON_NOTIFICATION:
        {
            NRF_LOG_INFO("Button state changed on peer to 0x%x.", p_lbs_c_evt->params.button.button_state);
            if (p_lbs_c_evt->params.button.button_state)
            {
                bsp_board_led_on(LEDBUTTON_LED);
            }
            else
            {
                bsp_board_led_off(LEDBUTTON_LED);
            }
        } break; // BLE_LBS_C_EVT_BUTTON_NOTIFICATION

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    ret_code_t err_code;

    // For readability.
    ble_gap_evt_t const * p_gap_evt = &p_ble_evt->evt.gap_evt;

    switch (p_ble_evt->header.evt_id)
    {
        // Upon connection, check which peripheral has connected (HR or RSC), initiate DB
        // discovery, update LEDs status and resume scanning if necessary. */
        case BLE_GAP_EVT_CONNECTED:
        {
            NRF_LOG_INFO("Connected.");
            err_code = ble_lbs_c_handles_assign(&m_ble_lbs_c, p_gap_evt->conn_handle, NULL);
            APP_ERROR_CHECK(err_code);

            err_code = ble_db_discovery_start(&m_db_disc, p_gap_evt->conn_handle);
            APP_ERROR_CHECK(err_code);
            //NRF_LOG_INFO("p_ble_evt->evt.gap_evt.params.adv_report.peer_addr : %x", p_ble_evt->evt.gap_evt.params.adv_report.peer_addr);
            //mac_address = p_ble_evt->evt.gap_evt.params.adv_report.peer_addr;
            //NRF_LOG_INFO("Mac address from peer_addr"); 
            //log_ble_gap_addr_t(p_ble_evt->evt.gap_evt.params.adv_report.peer_addr);
            //memcpy(&mac_address, &p_ble_evt->evt.gap_evt.params.adv_report.peer_addr, sizeof(ble_gap_addr_t));
            memcpy(mac_address.addr, p_ble_evt->evt.gap_evt.params.connected.peer_addr.addr, 6);
            NRF_LOG_INFO("mac_address address : %x", &mac_address.addr);
            NRF_LOG_INFO("Peer Mac Address");
            nrf_delay_ms(20);
            //save_mac_address_to_flash(mac_address);
            log_ble_gap_addr_t(mac_address);
            just_connected = true;
            // Update LEDs status, and check if we should be looking for more
            // peripherals to connect to.
            
            
        } break;

        // Upon disconnection, reset the connection handle of the peer which disconnected, update
        // the LEDs status and start scanning again.
        case BLE_GAP_EVT_DISCONNECTED:
        {
            NRF_LOG_INFO("Disconnected.");
            scan_start();
        } break;

        case BLE_GAP_EVT_TIMEOUT:
        {
            // We have not specified a timeout for scanning, so only connection attemps can timeout.
            if (p_gap_evt->params.timeout.src == BLE_GAP_TIMEOUT_SRC_CONN)
            {
                NRF_LOG_DEBUG("Connection request timed out.");
            }

        } break;

        case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST:
        {
            // Accept parameters requested by peer.
            err_code = sd_ble_gap_conn_param_update(p_gap_evt->conn_handle,
                                        &p_gap_evt->params.conn_param_update_request.conn_params);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GATTC_EVT_TIMEOUT:
        {
            // Disconnect on GATT Client timeout event.
            NRF_LOG_DEBUG("GATT Client Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
        }break;

        case BLE_GATTS_EVT_TIMEOUT:
        {
            // Disconnect on GATT Server timeout event.
            NRF_LOG_DEBUG("GATT Server Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
        } break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief LED Button client initialization.
 */
static void lbs_c_init(void)
{
    ret_code_t       err_code;
    ble_lbs_c_init_t lbs_c_init_obj;

    lbs_c_init_obj.evt_handler   = lbs_c_evt_handler;
    lbs_c_init_obj.p_gatt_queue  = &m_ble_gatt_queue;
    lbs_c_init_obj.error_handler = lbs_error_handler;

    err_code = ble_lbs_c_init(&m_ble_lbs_c, &lbs_c_init_obj);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupts.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}


/**@brief Function for handling events from the button handler module.
 *
 * @param[in] pin_no        The pin that the event applies to.
 * @param[in] button_action The button action (press/release).
 */
static void button_event_handler(uint8_t pin_no, uint8_t button_action)
{
    ret_code_t err_code;
    if(button_action == APP_BUTTON_PUSH)
    {
        NRF_LOG_INFO("Button %d pushed", pin_no);
    }
    else
    {
        NRF_LOG_INFO("Button %d released", pin_no);
        return;
    }
    //switch (pin_no)
    //{
    //    case LEDBUTTON_BUTTON_PIN:
    //        err_code = ble_lbs_led_status_send(&m_ble_lbs_c, button_action);
    //        if (err_code != NRF_SUCCESS &&
    //            err_code != BLE_ERROR_INVALID_CONN_HANDLE &&
    //            err_code != NRF_ERROR_INVALID_STATE)
    //        {
    //            APP_ERROR_CHECK(err_code);
    //        }
    //        if (err_code == NRF_SUCCESS)
    //        {
    //            NRF_LOG_INFO("LBS write LED state %d", button_action);
    //            // TODO Verify if this works
    //            //disable_ldo();
    //        }
    //        break;

    //    default:
    //        APP_ERROR_HANDLER(pin_no);
    //        break;
    //}
}


/**@brief Function for handling Scaning events.
 *
 * @param[in]   p_scan_evt   Scanning event.
 */
static void scan_evt_handler(scan_evt_t const * p_scan_evt)
{
    ret_code_t err_code;

    switch(p_scan_evt->scan_evt_id)
    {
        case NRF_BLE_SCAN_EVT_CONNECTING_ERROR:
            err_code = p_scan_evt->params.connecting_err.err_code;
            APP_ERROR_CHECK(err_code);
            break;
        default:
          break;
    }
}



/**@brief Function for initializing the button handler module.
 */
static void buttons_init(void)
{
    ret_code_t err_code;
    //The array must be static because a pointer to it will be saved in the button handler module.
    static app_button_cfg_t buttons[] =
    {
        {LEDBUTTON_BUTTON_PIN, false, NRF_GPIO_PIN_NOPULL, button_event_handler}
    };

    err_code = app_button_init(buttons, ARRAY_SIZE(buttons),
                               BUTTON_DETECTION_DELAY);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling database discovery events.
 *
 * @details This function is callback function to handle events from the database discovery module.
 *          Depending on the UUIDs that are discovered, this function should forward the events
 *          to their respective services.
 *
 * @param[in] p_event  Pointer to the database discovery event.
 */
static void db_disc_handler(ble_db_discovery_evt_t * p_evt)
{
    ble_lbs_on_db_disc_evt(&m_ble_lbs_c, p_evt);
}


/**@brief Database discovery initialization.
 */
static void db_discovery_init(void)
{
    ble_db_discovery_init_t db_init;

    memset(&db_init, 0, sizeof(db_init));

    db_init.evt_handler  = db_disc_handler;
    db_init.p_gatt_queue = &m_ble_gatt_queue;

    ret_code_t err_code = ble_db_discovery_init(&db_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the log.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);


    NRF_LOG_DEFAULT_BACKENDS_INIT();
}
static void gpio_ble_data_send_handler(void * p_context)
{
    ret_code_t err_code;
    uint8_t data = 1;

    err_code = ble_lbs_led_status_send(&m_ble_lbs_c, &data);
    if (err_code == NRF_SUCCESS)
    {
        APP_ERROR_CHECK(err_code);
        NRF_LOG_INFO("Data Sent!");
    }
    else{
       NRF_LOG_INFO("Data Sending failed");
    }
     //disable_ldo();
    err_code = app_timer_start(m_power_off_timer, APP_TIMER_TICKS(100), NULL);
    APP_ERROR_CHECK(err_code);
}

static void scanning_timeout_handler(void * p_context){
    disable_ldo();
}
static void power_off_handler(void * p_context){
    disable_ldo();
}

static void erase_mac_handler(void * p_context){
    NRF_LOG_INFO("Erasing Mac");
    fstorage_clear_page();
}

/**@brief Function for initializing the timer.
 */
static void timer_init(void)
{
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_ble_data_send_timer, APP_TIMER_MODE_SINGLE_SHOT, gpio_ble_data_send_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_scanning_timeout, APP_TIMER_MODE_SINGLE_SHOT, scanning_timeout_handler);
    APP_ERROR_CHECK(err_code);
    
    err_code = app_timer_create(&m_power_off_timer, APP_TIMER_MODE_SINGLE_SHOT, power_off_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_erase_mac_timer, APP_TIMER_MODE_SINGLE_SHOT, erase_mac_handler);
    APP_ERROR_CHECK(err_code);
    
}


/**@brief Function for initializing the Power manager. */
static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}


static void scan_init(void)
{
    ret_code_t          err_code;
    nrf_ble_scan_init_t init_scan;
    
    memset(&init_scan, 0, sizeof(init_scan));
    //init_scan.p_scan_param     = &m_scan_param;
    init_scan.connect_if_match = true;
    init_scan.conn_cfg_tag     = APP_BLE_CONN_CFG_TAG;

    err_code = nrf_ble_scan_init(&m_scan, &init_scan, scan_evt_handler);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_INFO("MAC address not available, scanning for 'Peripheral'");
    // Setting filters for scanning.
    

    //err_code = nrf_ble_scan_filter_set(&m_scan, SCAN_NAME_FILTER, m_target_periph_name);
    //APP_ERROR_CHECK(err_code);

    //err_code = nrf_ble_scan_filters_enable(&m_scan, NRF_BLE_SCAN_NAME_FILTER, false);
    //APP_ERROR_CHECK(err_code);

    
    //TODO replace false with is_mac_address_available() when whitelist fixed
    if(is_mac_address_available()){
        NRF_LOG_INFO("MAC address available, filtering with it");
        ble_gap_addr_t  address_to_filter = read_mac_address_from_flash();
        address_to_filter.addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC;

        err_code = nrf_ble_scan_filter_set(&m_scan, SCAN_ADDR_FILTER, address_to_filter.addr);
        APP_ERROR_CHECK(err_code);

        err_code = nrf_ble_scan_filters_enable(&m_scan, NRF_BLE_SCAN_ADDR_FILTER, false);
        NRF_LOG_INFO("error_code : %d", err_code);
    }


    else{
        NRF_LOG_INFO("MAC address not available, scanning for 'Peripheral'");
        // Setting filters for scanning.
        

        err_code = nrf_ble_scan_filter_set(&m_scan, SCAN_NAME_FILTER, m_target_periph_name);
        APP_ERROR_CHECK(err_code);

        err_code = nrf_ble_scan_filters_enable(&m_scan, NRF_BLE_SCAN_NAME_FILTER, false);
        APP_ERROR_CHECK(err_code);
    }

    
    
}

/**@brief Function for initializing the GATT module.
 */
static void gatt_init(void)
{
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the idle state (main loop).
 *
 * @details Handle any pending log operation(s), then sleep until the next event occurs.
 */
static void idle_state_handle(void)
{
    NRF_LOG_FLUSH();
    nrf_pwr_mgmt_run();
    //NRF_LOG_INFO("Idle"); 
}



void on_connection_from_main(ble_gap_addr_t mac_address){
    
    save_mac_address_to_flash(mac_address);
    just_connected = false;
    
    ret_code_t err_code = app_timer_start(m_ble_data_send_timer, APP_TIMER_TICKS(500), NULL);
    APP_ERROR_CHECK(err_code);
}


int main(void)
{
    // Initialize.
    enable_ldo();
    log_init();
    timer_init();
    //leds_init();
    buttons_init();
    power_management_init();
    central_fstorage_init();


    ble_stack_init();
    scan_init();                     //81:E8:BF:90:D2:E6
    gatt_init();
    db_discovery_init();
    lbs_c_init();

    // Start execution.
    NRF_LOG_INFO("Blinky CENTRAL example started.");
    scan_start();

    // Turn on the LED to signal scanning.
    bsp_board_led_on(CENTRAL_SCANNING_LED);

    //NRF_LOG_INFO("Local Address");
    //log_ble_gap_addr_t(mac_address);


    // Get the MAC address
    ble_gap_addr_t mac_address_local;
    sd_ble_gap_addr_get(&mac_address_local);
    NRF_LOG_INFO("Local Mac Address");
    log_ble_gap_addr_t(mac_address_local);

    ret_code_t err_code = app_timer_start(m_scanning_timeout, APP_TIMER_TICKS(5000), NULL);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_start(m_erase_mac_timer, APP_TIMER_TICKS(6000), NULL);
    APP_ERROR_CHECK(err_code);
    
    // Enter main loop.
    for(;;)
    {
        if(just_connected)
        {
            on_connection_from_main(mac_address);
        }
        idle_state_handle();
    }
}
