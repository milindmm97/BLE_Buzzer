#ifndef NAVISOR_BLE_COMMONSRV_H
#define NAVISOR_BLE_COMMONSRV_H

#include <stdint.h>
#include "ble_srv_common.h"
#include "sdk_common.h"
#include "nrf_error.h"
#include "app_error.h"


#define BLE_COMMONSERVE_BLE_OBSERVER_PRIO  2

#define BLE_COMMONSERVE_DEF(_name)                                                             \
static ble_commonserv_t _name;                                                                       \
NRF_SDH_BLE_OBSERVER(_name ## _obs,                                                               \
                     BLE_COMMONSERVE_BLE_OBSERVER_PRIO,                                        \
                     ble_commonserv_on_ble_evt, &_name)


//00001001-4E41-5669-736F-722043530000
#define BLE_UUID_COMMONSERVE_SERVICE_BASE         	{0x00, 0x00, 0x53, 0x43, 0x20, 0x72, 0x6F, 0x73,  \
                                                 	0x69, 0x56, 0x41, 0x4E, 0x00, 0x00, 0x00, 0x00 }

                                                
#define BLE_UUID_COMMONSERVE_SERVICE                	0x1000
	
#define BLE_UUID_NAVIGATION_STATES_CHAR             	0x1001


static uint16_t                 service_handle;



typedef enum
{   
    BLE_NAVIGATION_STATES_CHAR_EVT_COMMAND_RX,
} ble_commonserv_evt_type_t;

typedef struct 
{
    uint8_t const * p_data;   
    uint16_t        length;  
} commonserv_received_data_t;


typedef struct
{
   ble_commonserv_evt_type_t evt_type;                     

   union
   {
       commonserv_received_data_t commonserv_received_data;

   } params_command;

} ble_commonserv_evt_t;



typedef struct ble_commonserv_s ble_commonserv_t;


typedef void (*ble_commonserv_evt_handler_t) (ble_commonserv_t * p_commonserv, ble_commonserv_evt_t * p_evt);



typedef struct
{
    ble_commonserv_evt_handler_t         evt_handler;                    /**< Event handler to be called for handling events in the Custom Service. */  
    

} ble_commonserv_init_t;


struct ble_commonserv_s
{
    ble_commonserv_evt_handler_t         evt_handler;
    uint16_t                          service_handle;  
   
   
    ble_gatts_char_handles_t          navigation_states_char_handles;
    
          
    uint16_t                          conn_handle; 
    uint8_t                           uuid_type;
};

typedef enum
{
    navigation_states_char_value_handle,
}ble_commonsrv_char_value_handle;

int get_navigation_code(char command[]);
static void on_navigation_char_data_receieved(ble_commonserv_evt_t * p_evt);
static void on_brightness_char_data_received(ble_commonserv_evt_t * p_evt);
static void on_autobrightness_char_data_received(ble_commonserv_evt_t * p_evt);
static void commonserv_evt_handler(ble_commonserv_t * p_commonserv, ble_commonserv_evt_t * p_evt);
void ble_update_brightness_level();
void ble_update_autobrightness_mode();
void ble_update_battery_level();
void ble_update_charging();
void init_commonserv(ble_commonserv_t * p_commonserv);
static void on_write(ble_commonserv_t * p_commonserv, ble_evt_t const * p_ble_evt);
void ble_commonserv_on_ble_evt( ble_evt_t const * p_ble_evt, void * p_context);
static void commonsrv_char_add(ble_commonserv_t * p_commonserv, ble_gatts_char_handles_t * char_handles, uint8_t init_len, uint8_t max_len, uint8_t * init_value, int char_uuid, bool read_access, bool write_access, bool notify_enabled);
static void navigation_state_char_add(ble_commonserv_t * p_commonserv, const ble_commonserv_init_t * p_commonserv_init);
static void battery_level_char_add(ble_commonserv_t * p_commonserv, const ble_commonserv_init_t * p_commonserv_init);
static void charging_char_add(ble_commonserv_t * p_commonserv, const ble_commonserv_init_t * p_commonserv_init);
static void brightness_level_char_add(ble_commonserv_t * p_commonserv, const ble_commonserv_init_t * p_commonserv_init);
static void autobrightness_mode_char_add(ble_commonserv_t * p_commonserv, const ble_commonserv_init_t * p_commonserv_init);
uint32_t ble_commonserv_init(ble_commonserv_t * p_commonserv, const ble_commonserv_init_t * p_commonserv_init);

static void on_sos_mode_char_data_received(ble_commonserv_evt_t * p_evt);
void ble_update_sos_mode(uint8_t sos_mode );


uint32_t ble_commonserv_init(ble_commonserv_t * p_commonserv, const ble_commonserv_init_t * p_commonserv_init);




#endif // NAVISOR_BLE_COMMONSRV_H
