#include "tp_mode_srv.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"


 


static void on_navigation_char_data_receieved(ble_commonserv_evt_t * p_evt)
{
    char command[3];
    int navigation_code;
    ble_update_navigation_data();
    NRF_LOG_INFO("Navigation states char commands received.");

}


static void commonserv_evt_handler(ble_commonserv_t * p_commonserv, ble_commonserv_evt_t * p_evt)
{
  switch(p_evt->evt_type)
  {
    case BLE_NAVIGATION_STATES_CHAR_EVT_COMMAND_RX:
        on_navigation_char_data_receieved(p_evt);

        break;
    default:
        break;
  }
}

void ble_update_navigation_data()
{

}




void init_commonserv(ble_commonserv_t * p_commonserv)
{
    ret_code_t            err_code;
    ble_commonserv_init_t     commonserv_init;
      
    memset(&commonserv_init, 0, sizeof(commonserv_init));
    commonserv_init.evt_handler = commonserv_evt_handler;


    err_code = ble_commonserv_init(p_commonserv, &commonserv_init);
    
    APP_ERROR_CHECK(err_code);
}

static void on_write(ble_commonserv_t * p_commonserv, ble_evt_t const * p_ble_evt)
{
    ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
    ble_commonserv_evt_t                 evt;

    evt.params_command.commonserv_received_data.p_data = p_evt_write->data;
    evt.params_command.commonserv_received_data.length = p_evt_write->len;

    // writing to the navigation states characteristic
    if (p_evt_write->handle == p_commonserv->navigation_states_char_handles.value_handle)
    {      
        evt.evt_type = BLE_NAVIGATION_STATES_CHAR_EVT_COMMAND_RX; 
    }

    p_commonserv->evt_handler(p_commonserv, &evt);

}


uint16_t get_value_handle(ble_commonserv_t * p_commonserv, ble_commonsrv_char_value_handle handle)
{

    uint16_t           value_handle;
    switch(handle)
    {
        case navigation_states_char_value_handle:
            value_handle = p_commonserv->navigation_states_char_handles.value_handle;
            break;

    }
    return value_handle;
}

void ble_commonserv_on_ble_evt( ble_evt_t const * p_ble_evt, void * p_context)
{
    ble_commonserv_t * p_commonserv = (ble_commonserv_t *) p_context;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTS_EVT_WRITE:
            on_write(p_commonserv, p_ble_evt);
            break;

        default:
            // No implementation needed.
            break;
    }
}

void commonsrv_char_add(ble_commonserv_t * p_commonserv, ble_gatts_char_handles_t * char_handles, uint8_t init_len, uint8_t max_len, uint8_t * init_value, int char_uuid, bool read_access, bool write_access, bool notify_enabled)
{
    ret_code_t              err_code;
    ble_add_char_params_t   add_char_params;


    memset(&add_char_params, 0, sizeof(add_char_params));

    add_char_params.uuid             = char_uuid;
    add_char_params.uuid_type        = p_commonserv->uuid_type;
    add_char_params.init_len         = init_len;
    add_char_params.max_len          = max_len;
    add_char_params.p_init_value     = &init_value;

    if(read_access)
    {
        add_char_params.char_props.read  = 1;
        add_char_params.read_access      = SEC_OPEN;
    }
    if(write_access)
    {
        add_char_params.write_access     = SEC_OPEN;
        add_char_params.char_props.write = 1;
    } 

    if(notify_enabled)
    {
        add_char_params.cccd_write_access = SEC_OPEN;
        add_char_params.char_props.notify = 1;
        
    }

    err_code = characteristic_add(p_commonserv->service_handle, 
                                  &add_char_params, 
                                  char_handles);
    VERIFY_SUCCESS(err_code);            
}

void navigation_state_char_add(ble_commonserv_t * p_commonserv, const ble_commonserv_init_t * p_commonserv_init)
{
    ret_code_t              err_code;
    ble_add_char_params_t   add_char_params;
    uint8_t                 init_len       = 2;
    uint8_t                 max_len        = 2;
    bool                    read_access    = 1;
    bool                    write_access   = 1;
    bool                    notify_enabled = 1;
    uint8_t navigation_char_init_value [1] = {0};
    commonsrv_char_add(p_commonserv, &p_commonserv->navigation_states_char_handles, init_len, max_len, &navigation_char_init_value, BLE_UUID_NAVIGATION_STATES_CHAR, read_access, write_access, notify_enabled);
}


uint32_t ble_commonserv_init(ble_commonserv_t * p_commonserv, const ble_commonserv_init_t * p_commonserv_init)
{

    if (p_commonserv == NULL || p_commonserv_init == NULL)
    {
        NRF_LOG_INFO("Common Service is null");
        //return NRF_ERROR_NULL;
    }
    else{
        NRF_LOG_INFO("Okay! at least, it's not null"); 
    }

    uint32_t                  err_code;
    ble_uuid_t                ble_uuid;
    ble_uuid128_t base_uuid =  {BLE_UUID_COMMONSERVE_SERVICE_BASE};
  
    p_commonserv->evt_handler    = p_commonserv_init->evt_handler;

    NRF_LOG_INFO("uuid_type : %d",  p_commonserv->uuid_type);
    err_code =  sd_ble_uuid_vs_add(&base_uuid, &p_commonserv->uuid_type);
    APP_ERROR_CHECK(err_code);

    ble_uuid.type = p_commonserv->uuid_type;
    ble_uuid.uuid = BLE_UUID_COMMONSERVE_SERVICE;

    //Adding Service
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_commonserv->service_handle);
    VERIFY_SUCCESS(err_code);
    
    //Adding navgation data characterstic
    navigation_state_char_add(p_commonserv, p_commonserv_init);
    

    return NRF_SUCCESS;
}



