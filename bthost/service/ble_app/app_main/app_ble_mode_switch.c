/***************************************************************************
*
*Copyright 2015-2019 BES.
*All rights reserved. All unpublished rights reserved.
*
*No part of this work may be used or reproduced in any form or by any
*means, or stored in a database or retrieval system, without prior written
*permission of BES.
*
*Use of this work is governed by a license granted by BES.
*This work contains confidential and proprietary information of
*BES. which is protected by copyright, trade secret,
*trademark and other intellectual property rights.
*
****************************************************************************/

/*****************************header include********************************/
#include "string.h"
#include "co_math.h" // Common Maths Definition
#include "cmsis_os.h"
#include "ble_app_dbg.h"
#include "stdbool.h"
#include "app_thread.h"
#include "app_utils.h"
#include "apps.h"
#include "app_ble_uart.h"
#include "gapm_le_msg.h"               // GAP Manager Task API
#include "gapc_msg.h"               // GAP Controller Task API
#include "app.h"
#include "app_sec.h"
#include "app_ble_include.h"
#include "nvrecord_bt.h"
#include "bluetooth_bt_api.h"
#include "app_bt_func.h"
#include "hal_timer.h"
#include "app_bt.h"
#include "app_hfp.h"
#include "rwprf_config.h"
#include "gapc_msg.h"
#include "nvrecord_ble.h"
#include "app_sec.h"
#include "besbt.h"
#include "gapc.h"
#include "bt_drv_interface.h"

#ifdef IBRT
#include "app_ibrt_internal.h"
#include "earbud_ux_api.h"
#include "app_tws_ibrt_cmd_handler.h"
#include "app_tws_ctrl_thread.h"
#endif

#ifdef IS_BLE_AUDIO_DEBUG_INFO_COLLECTOR_ENABLED
#include "app_dbg_ble_audio_info.h"
#endif
/************************private macro defination***************************/
#define APP_BLE_UPDATE_CI_TIMEOUT_INTERVEL          (5000)
#define APP_BLE_UPDATE_CI_MAX_TIMES                 (3)

/************************private type defination****************************/

/************************extern function declearation***********************/
#ifdef CUSTOMER_DEFINE_ADV_DATA
extern void app_ble_custom_adv_clear_enabled_flag(BLE_ADV_ACTIVITY_USER_E actv_user);
#endif

/**********************private function declearation************************/
static void app_ble_refresh_adv_state_generic(uint16_t advInterval);
static void app_ble_update_ci_timeout_cb(void const *n);

/************************private variable defination************************/
static BLE_MODE_ENV_T bleModeEnv = {.bleEnv = &app_env};
osTimerDef(APP_BLE_UPDATE_CI_TIMEOUT, app_ble_update_ci_timeout_cb);

/****************************function defination****************************/
void app_ble_mode_init(void)
{
    LOG_I("%s", __func__);

    uint8_t actv_user;
    memset(&bleModeEnv, 0, sizeof(bleModeEnv));

    bleModeEnv.bleEnv = &app_env;

    bleModeEnv.advPendingInterval = BLE_ADVERTISING_INTERVAL;
    bleModeEnv.advPendingDiscmode = GAPM_ADV_MODE_GEN_DISC;

    // by default use legacy adv mode
#if (BLE_AUDIO_ENABLED)
    bleModeEnv.advPendingType = ADV_TYPE_CONN_EXT_ADV;
    bleModeEnv.advertisingPendingMode = ADV_MODE_EXTENDED;
#else
    bleModeEnv.advPendingType = ADV_TYPE_UNDIRECT;
    bleModeEnv.advertisingPendingMode = ADV_MODE_LEGACY;
#endif

    for (actv_user = BLE_ADV_ACTIVITY_USER_0; actv_user < BLE_ADV_ACTIVITY_USER_NUM; actv_user++)
    {
        bleModeEnv.advCurrentTxpwr[actv_user] = BLE_ADV_INVALID_TXPWR;
        bleModeEnv.advPendingTxpwr[actv_user] = BLE_ADV_INVALID_TXPWR;
    }
}

static bool app_ble_update_ci_fail_handle(uint8_t conidx, uint8_t errCode)
{
    APP_BLE_CONN_CONTEXT_T *pContext = &(app_env.context[conidx]);
    // indicates whether the update in progree has ended
    bool isEnd = true;

    if (!pContext->updateCITimerId)
    {
        return false;
    }

    LOG_I("(d%d)%s,tryCnt:%d, exist:%d-%d, errCode:0x%02x", conidx, __func__,
          pContext->ongoingCI.ciUpdateTimes,
          pContext->ongoingCI.exist, pContext->pendingCI.exist, errCode);

    if (pContext->pendingCI.exist)
    {
        memcpy(&pContext->ongoingCI, &pContext->pendingCI, sizeof(APP_BLE_UPDATE_CI_T));
        pContext->pendingCI.exist = false;
    }
    else
    {
        if ((++app_env.context[conidx].ongoingCI.ciUpdateTimes > APP_BLE_UPDATE_CI_MAX_TIMES) ||
                (!app_ble_is_connection_on_by_index(conidx)) ||
                (LL_ERR_CON_TIMEOUT == errCode))
        {
            app_env.context[conidx].ongoingCI.exist = false;
            return true;
        }
        else
        {
            // keep trying
            isEnd = false;
        }
    }
    appm_update_param(conidx, pContext->ongoingCI.minInterval,
                      pContext->ongoingCI.maxInterval,
                      pContext->ongoingCI.supTO,
                      pContext->ongoingCI.conLatency);

    osTimerStop(pContext->updateCITimerId);
    osTimerStart(pContext->updateCITimerId, APP_BLE_UPDATE_CI_TIMEOUT_INTERVEL);
    return isEnd;
}

static void app_ble_update_ci_success_handle(uint8_t conidx)
{
    APP_BLE_CONN_CONTEXT_T *pContext = &(app_env.context[conidx]);

    if (!pContext->updateCITimerId)
    {
        return;
    }

    LOG_I("(d%d)%s, exist:%d-%d", conidx, __func__,
          pContext->ongoingCI.exist, pContext->pendingCI.exist);
    if (pContext->pendingCI.exist)
    {
        memcpy(&pContext->ongoingCI, &pContext->pendingCI, sizeof(APP_BLE_UPDATE_CI_T));
        pContext->pendingCI.exist = false;
        osTimerStop(pContext->updateCITimerId);
        osTimerStart(pContext->updateCITimerId, 100);
    }
    else
    {
        pContext->ongoingCI.exist = false;
        osTimerStop(pContext->updateCITimerId);
    }
}

static void app_ble_update_ci_timeout_cb(void const *n)
{
    APP_BLE_CONN_CONTEXT_T *pContext = (APP_BLE_CONN_CONTEXT_T *)n;
    uint8_t conidx = appm_get_conidx_from_conhdl(pContext->conhdl);
    LOG_I("(d%d)%s", conidx, __func__);
    if (BLE_INVALID_CONNECTION_INDEX != conidx)
    {
        app_bt_start_custom_function_in_bt_thread((uint32_t)conidx, 0,
                                                  (uint32_t)app_ble_update_ci_fail_handle);
    }
}

static void app_ble_clear_update_ci_list(uint8_t conidx)
{
    APP_BLE_CONN_CONTEXT_T *pContext = &(app_env.context[conidx]);
    pContext->ongoingCI.exist = false;
    pContext->pendingCI.exist = false;
    if (pContext->updateCITimerId)
    {
        osTimerStop(pContext->updateCITimerId);
    }
}

bool app_ble_push_update_ci_list(uint8_t conidx, uint32_t min_interval,
                                 uint32_t max_interval, uint32_t supervision_timeout, uint8_t slaveLatency)
{
    APP_BLE_UPDATE_CI_T *pCI = NULL;
    bool ret = false;
    uint32_t lock = int_lock();
    APP_BLE_CONN_CONTEXT_T *pContext = &(app_env.context[conidx]);
    LOG_I("(d%d)push_ci_list,exist:%d, interval:0x%x->0x%x-0x%x,latency:0x%x->0x%x,supTO:0x%x->0x%x",
          conidx, pContext->ongoingCI.exist,
          pContext->connParam.con_interval, min_interval, max_interval,
          pContext->connParam.con_latency, slaveLatency,
          pContext->connParam.sup_to, supervision_timeout);

    if (!pContext->ongoingCI.exist)
    {
        // new CI update, check if the parameters are different from the param in use
        if ((pContext->connParam.con_interval < min_interval) ||
                (pContext->connParam.con_interval > max_interval) ||
                (pContext->connParam.sup_to != (supervision_timeout / 10)) ||
                (pContext->connParam.con_latency != slaveLatency))
        {
            pCI = &pContext->ongoingCI;
            ret = true;
        }
    }
    else
    {
        // CI update ongoing, check if the parameters are different
        if ((pContext->ongoingCI.minInterval != min_interval) ||
                (pContext->ongoingCI.maxInterval != max_interval) ||
                (pContext->ongoingCI.supTO != (supervision_timeout / 10)) ||
                (pContext->ongoingCI.conLatency != slaveLatency))
        {
            pCI = &pContext->pendingCI;
        }
    }
    if (!pCI)
    {
        int_unlock(lock);
        return false;
    }
    pCI->minInterval = min_interval;
    pCI->maxInterval = max_interval;
    pCI->supTO = supervision_timeout;
    pCI->conLatency = slaveLatency;
    pCI->exist = true;
    pCI->ciUpdateTimes = 0;
    int_unlock(lock);

    if (!pContext->updateCITimerId)
    {
        pContext->updateCITimerId = osTimerCreate(osTimer(APP_BLE_UPDATE_CI_TIMEOUT), osTimerOnce, pContext);
    }

    osTimerStart(pContext->updateCITimerId, APP_BLE_UPDATE_CI_TIMEOUT_INTERVEL);
    return ret;
}

static void ble_set_actv_action(BLE_ACTV_ACTION_E actv_action, uint8_t actv_idx)
{
    LOG_I("%s %d -> %d idx 0x%02x", __func__, bleModeEnv.ble_actv_action, actv_action, actv_idx);
    bleModeEnv.ble_op_actv_idx = actv_idx;
    bleModeEnv.ble_actv_action = actv_action;
}

static bool ble_clear_actv_action(BLE_ACTV_ACTION_E actv_action, uint8_t actv_idx)
{
    LOG_I("actv_action now %d clear %d idx 0x%02x", bleModeEnv.ble_actv_action, actv_action, actv_idx);

    if (actv_action == bleModeEnv.ble_actv_action)
    {
        if (bleModeEnv.ble_actv_action == BLE_ACTV_ACTION_IDLE)
        {
            LOG_I("%s already idle now", __func__);
            return true;
        }

        if ((BLE_ACTV_ACTION_STOPPING_ADV == actv_action)
                && bleModeEnv.ble_op_actv_idx != actv_idx)
        {
            return false;
        }

        LOG_I("%s %d -> %d", __func__, bleModeEnv.ble_actv_action, BLE_ACTV_ACTION_IDLE);
        bleModeEnv.ble_op_actv_idx = 0xFF;
        bleModeEnv.ble_actv_action = BLE_ACTV_ACTION_IDLE;
        return true;
    }

    return false;
}

// ble advertisement used functions
static void ble_adv_config_param(BLE_ADV_ACTIVITY_USER_E actv_user)
{
    uint8_t i = 0;
    POSSIBLY_UNUSED int16_t avail_space = 0;
    ///set the adv_user_interval is BLE_ADV_INVALID_INTERVAL (default)
    uint32_t adv_user_interval = BLE_ADV_INVALID_INTERVAL;
    uint8_t empty_addr[BTIF_BD_ADDR_SIZE] = {0, 0, 0, 0, 0, 0};
    BLE_ADV_FILL_PARAM_T *p_ble_param = app_ble_param_get_ctx();

    // reset adv param info
    memset(&bleModeEnv.advParamInfo, 0, sizeof(bleModeEnv.advParamInfo));
    bleModeEnv.adv_user_enable[actv_user] = 0;
    bleModeEnv.advParamInfo.advType = bleModeEnv.advPendingType;
    bleModeEnv.advParamInfo.advMode = bleModeEnv.advertisingPendingMode;
    bleModeEnv.advParamInfo.discMode = bleModeEnv.advPendingDiscmode;
    bleModeEnv.advParamInfo.advTxPwr = app_ble_param_get_adv_tx_power_level(actv_user);
#ifdef IS_BLE_FLAGS_ADV_DATA_CONFIGURED_BY_APP_LAYER
    bleModeEnv.advParamInfo.isBleFlagsAdvDataConfiguredByAppLayer = true;
#else
    bleModeEnv.advParamInfo.isBleFlagsAdvDataConfiguredByAppLayer = false;
#endif
    if (memcmp(bleModeEnv.advPendingLocalAddr[actv_user], empty_addr, BTIF_BD_ADDR_SIZE))
    {
        memcpy(bleModeEnv.advParamInfo.localAddr, bleModeEnv.advPendingLocalAddr[actv_user], BTIF_BD_ADDR_SIZE);
    }
    else
    {
#ifdef CUSTOMER_DEFINE_ADV_DATA
        memcpy(bleModeEnv.advParamInfo.localAddr, bleModeEnv.advPendingLocalAddr[actv_user], BTIF_BD_ADDR_SIZE);
#else
        memcpy(bleModeEnv.advParamInfo.localAddr, bt_get_ble_local_address(), BTIF_BD_ADDR_SIZE);
#endif
    }

    if (app_ble_is_white_list_enable())
    {
        if (bleModeEnv.bleEnv->numberOfDevicesAddedToResolvingList != 0)
        {
            LOG_I("white list mode");
            bleModeEnv.advParamInfo.filter_pol = ADV_ALLOW_SCAN_WLST_CON_WLST;
        }
        else
        {
            LOG_I("Resolving list is empty, white list mode is disabled");
            bleModeEnv.advParamInfo.filter_pol = ADV_ALLOW_SCAN_ANY_CON_ANY;
        }
    }
    else
    {
        bleModeEnv.advParamInfo.filter_pol = ADV_ALLOW_SCAN_ANY_CON_ANY;
    }

    for (i = USER_INUSE; i < BLE_ADV_USER_NUM; i++)
    {
        bleModeEnv.advParamInfo.advUserInterval[i] = BLE_ADV_INVALID_INTERVAL;
    }

    // scenarios interval
    bleModeEnv.advParamInfo.advInterval = app_ble_param_get_adv_interval(actv_user);

#ifdef IS_BLE_FLAGS_ADV_DATA_CONFIGURED_BY_APP_LAYER
    bleModeEnv.advParamInfo.advData[0] = GAPM_ADV_AD_TYPE_FLAGS_INFO_LENGTH;
    bleModeEnv.advParamInfo.advData[1] = GAP_AD_TYPE_FLAGS;
    bleModeEnv.advParamInfo.advData[2] = GAP_LE_GEN_DISCOVERABLE_FLG_BIT | GAP_SIMUL_BR_EDR_LE_CONTROLLER_BIT;
    bleModeEnv.advParamInfo.advDataLen += GAPM_ADV_AD_TYPE_FLAGS_LENGTH;
#endif

    // fill adv user data
    for (i = USER_INUSE; i < BLE_ADV_USER_NUM; i++)
    {
        BLE_ADV_USER_E user = p_ble_param[actv_user].adv_user[i];
        if (USER_INUSE != user)
        {
            if (bleModeEnv.bleDataFillFunc[user])
            {
                bleModeEnv.bleDataFillFunc[user]((void *)&bleModeEnv.advParamInfo);

                // check if the adv/scan_rsp data length is legal
                if (bleModeEnv.advParamInfo.advMode == ADV_MODE_LEGACY)
                {
                    // check if the adv/scan_rsp data length is legal
                    if (bleModeEnv.advParamInfo.isBleFlagsAdvDataConfiguredByAppLayer)
                    {
                        ASSERT(BLE_ADV_DATA_WITHOUT_FLAG_LEN >= bleModeEnv.advParamInfo.advDataLen, "[BLE][ADV]adv data exceed");
                    }
                    else
                    {
                        ASSERT(BLE_ADV_DATA_WITH_FLAG_LEN >= bleModeEnv.advParamInfo.advDataLen, "[BLE][ADV]adv data exceed");
                    }
                    ASSERT(SCAN_RSP_DATA_LEN >= bleModeEnv.advParamInfo.scanRspDataLen, "[BLE][ADV]scan response data exceed");
                }
            }
        }
    }

    // adv param judge
    if (BLE_ADV_INVALID_INTERVAL == bleModeEnv.advParamInfo.advInterval)
    {
        // adv user interval
        for (i = USER_INUSE; i < BLE_ADV_USER_NUM; i++)
        {
            // adv interval
            adv_user_interval = bleModeEnv.advParamInfo.advUserInterval[i];
            if ((BLE_ADV_INVALID_INTERVAL != adv_user_interval) &&
                    ((BLE_ADV_INVALID_INTERVAL == bleModeEnv.advParamInfo.advInterval) ||
                     (adv_user_interval < bleModeEnv.advParamInfo.advInterval)))
            {
                bleModeEnv.advParamInfo.advInterval = adv_user_interval;
            }
        }
    }

    // adv interval judge
    if (BLE_ADV_INVALID_INTERVAL == bleModeEnv.advParamInfo.advInterval)
    {
        bleModeEnv.advParamInfo.advInterval = BLE_ADVERTISING_INTERVAL;
    }

    // adv type judge
    // connectable adv is not allowed if max connection reaches
    if (app_is_arrive_at_max_ble_connections() &&
            (ADV_TYPE_NON_CONN_SCAN != bleModeEnv.advParamInfo.advType) &&
            (ADV_TYPE_NON_CONN_NON_SCAN != bleModeEnv.advParamInfo.advType))
    {
        LOG_W("will change adv type to none-connectable because max ble connection reaches");
        bleModeEnv.advParamInfo.advType = ADV_TYPE_NON_CONN_SCAN;
    }

#if !defined (CUSTOMER_DEFINE_ADV_DATA) || defined (BLE_AUDIO_TEST_ENABLED)
    if (!bleModeEnv.advParamInfo.isBleFlagsAdvDataConfiguredByAppLayer)
    {
        if (ADV_MODE_LEGACY == bleModeEnv.advParamInfo.advMode)
        {
            avail_space = BLE_ADV_DATA_WITH_FLAG_LEN - bleModeEnv.advParamInfo.advDataLen - BLE_ADV_DATA_STRUCT_HEADER_LEN;
        }
        else
        {
            avail_space = EXT_ADV_DATA_LEN - BLE_ADV_FLAG_PART_LEN - bleModeEnv.advParamInfo.advDataLen - BLE_ADV_DATA_STRUCT_HEADER_LEN;
        }
    }
    else
    {
        if (ADV_MODE_LEGACY == bleModeEnv.advParamInfo.advMode)
        {
            avail_space = BLE_ADV_DATA_WITHOUT_FLAG_LEN - bleModeEnv.advParamInfo.advDataLen - BLE_ADV_DATA_STRUCT_HEADER_LEN;
        }
        else
        {
            avail_space = EXT_ADV_DATA_LEN - bleModeEnv.advParamInfo.advDataLen - BLE_ADV_DATA_STRUCT_HEADER_LEN;
        }
    }

    // Check if data can be added to the adv Data
    if (avail_space > BLE_ADV_DATA_STRUCT_HEADER_LEN && bleModeEnv.advParamInfo.disable_dev_name_fill == false)
    {
        avail_space = co_min(avail_space, bleModeEnv.bleEnv->dev_name_len);
        bleModeEnv.advParamInfo.advData[bleModeEnv.advParamInfo.advDataLen++] = avail_space;
        // Fill Device Name Flag
        bleModeEnv.advParamInfo.advData[bleModeEnv.advParamInfo.advDataLen++] =
            (avail_space == bleModeEnv.bleEnv->dev_name_len) ? '\x09' : '\x08';
        // Copy device name
        memcpy(&bleModeEnv.advParamInfo.advData[bleModeEnv.advParamInfo.advDataLen], bleModeEnv.bleEnv->dev_name, avail_space - 1);
        // Update adv Data Length
        bleModeEnv.advParamInfo.advDataLen += avail_space - 1;
    }
#endif
}

/*---------------------------------------------------------------------------
 *            ble_adv_is_allowed
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    check if BLE advertisment is allowed or not
 *
 * Parameters:
 *    void
 *
 * Return:
 *    true - if advertisement is allowed
 *    flase -  if adverstisement is not allowed
 */
static bool ble_adv_is_allowed(void)
{
    bool allowed_adv = true;
    if (!app_is_stack_ready())
    {
        LOG_I("reason: stack not ready");
        allowed_adv = false;
    }

    if (app_is_power_off_in_progress())
    {
        LOG_I("reason: in power off mode");
        allowed_adv = false;
    }

#ifndef CUSTOMER_DEFINE_ADV_DATA
    if (bleModeEnv.disablerBitmap)
    {
        LOG_I("adv disablerBitmap:0x%x", bleModeEnv.disablerBitmap);
        allowed_adv = false;
    }
#endif

#if ((BLE_AUDIO_ENABLED == 0) && (!defined SASS_ENABLED)) && defined(BT_HFP_SUPPORT)
    if (btapp_hfp_is_sco_active())
    {
        LOG_I("SCO ongoing");
        allowed_adv = false;
    }
#endif

    if (app_is_arrive_at_max_ble_connections())
    {
        LOG_I("arrive at max connection");
        allowed_adv = false;
    }

    return allowed_adv;
}

/*---------------------------------------------------------------------------
 *            ble_stop_advertising
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    stop advertising
 *
 * Parameters:
 *    actv_idx - activity index
 *
 * Return:
 *    void
 */
static void ble_stop_advertising(uint8_t actv_idx)
{
    ble_set_actv_action(BLE_ACTV_ACTION_STOPPING_ADV, actv_idx);
    appm_stop_advertising(actv_idx);
}

/*---------------------------------------------------------------------------
 *            ble_start_adv
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    start BLE advertisement
 *
 * Parameters:
 *    param - see @BLE_ADV_PARAM_T to get more info
 *
 * Return:
 *    void
 */
static void ble_start_adv(void *param)
{
    BLE_ADV_PARAM_T *pAdvParam = (BLE_ADV_PARAM_T *)param;
    uint8_t actv_user = pAdvParam->adv_actv_user;
    BLE_ADV_PARAM_T *pCurrentAdvParam = &bleModeEnv.advCurrentInfo[actv_user];
    if (!ble_adv_is_allowed())
    {
        LOG_I("[ADV] not allowed.");
        ble_stop_advertising(bleModeEnv.bleEnv->adv_actv_idx[pAdvParam->adv_actv_user]);
        return;
    }

    memcpy(pCurrentAdvParam, param, sizeof(BLE_ADV_PARAM_T));
    ble_set_actv_action(BLE_ACTV_ACTION_STARTING_ADV, 0xFF);
    appm_start_advertising(pCurrentAdvParam);
}

/*---------------------------------------------------------------------------
 *            ble_start_scan
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    start BLE scan api
 *
 * Parameters:
 *    param - see @BLE_SCAN_PARAM_T to get more info
 *
 * Return:
 *    void
 */
static void ble_start_scan(void)
{
    if (INVALID_BLE_ACTIVITY_INDEX != bleModeEnv.bleEnv->scan_actv_idx &&
            APP_SCAN_STATE_STARTED == bleModeEnv.bleEnv->state[bleModeEnv.bleEnv->scan_actv_idx] &&
            !memcmp(&bleModeEnv.scanCurrentInfo, &bleModeEnv.scanPendingInfo, sizeof(bleModeEnv.scanCurrentInfo)))
    {
        ble_execute_pending_op(BLE_ACTV_ACTION_IDLE, 0xFF);
        LOG_I("reason: scan param not changed");
        LOG_I("[SCAN] not allowed.");
        return;
    }
    memcpy(&bleModeEnv.scanCurrentInfo, &bleModeEnv.scanPendingInfo, sizeof(BLE_SCAN_PARAM_T));
    ble_set_actv_action(BLE_ACTV_ACTION_STARTING_SCAN, 0xFF);

    BLE_SCAN_PARAM_T scanCurrentInfo;

    scanCurrentInfo.scanType = bleModeEnv.scanCurrentInfo.scanType;
    scanCurrentInfo.scanFolicyType = bleModeEnv.scanCurrentInfo.scanFolicyType;
    scanCurrentInfo.scanWindowMs = bleModeEnv.scanCurrentInfo.scanWindowMs;
    scanCurrentInfo.scanIntervalMs = bleModeEnv.scanCurrentInfo.scanIntervalMs;
    scanCurrentInfo.scanDurationMs = bleModeEnv.scanCurrentInfo.scanDurationMs;
    appm_start_scanning(&scanCurrentInfo);
}

/*---------------------------------------------------------------------------
 *            ble_stop_scanning
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    stop BLE scanning
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
static void ble_stop_scanning(void)
{
    ble_set_actv_action(BLE_ACTV_ACTION_STOPPING_SCAN, 0xFF);
    appm_stop_scanning();
}

/*---------------------------------------------------------------------------
 *            ble_start_connect
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    start BLE connection
 *
 * Parameters:
 *    bleBdAddr - address of BLE device to connect
 *
 * Return:
 *    void
 */
static void ble_start_connect(void)
{
    uint8_t index = 0;

    for (index = 0; index < BLE_CONNECTION_MAX; index++)
    {
        if (bleModeEnv.bleToConnectInfo[index].pendingConnect)
        {
            break;
        }
    }
    ASSERT(index < BLE_CONNECTION_MAX, "%s has no ble to connect, index %d", __func__, index);

    bleModeEnv.bleToConnectInfo[index].pendingConnect = false;

    LOG_I("%s index %d", __func__, index);
    ble_set_actv_action(BLE_ACTV_ACTION_CONNECTING, 0xFF);
    appm_start_connecting(&bleModeEnv.bleToConnectInfo[index].init_param);
}

/*---------------------------------------------------------------------------
 *            ble_stop_connecting
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    stop BLE connecting
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
static void ble_stop_connecting(void)
{
    ble_set_actv_action(BLE_ACTV_ACTION_STOP_CONNECTING, 0xFF);
    appm_stop_connecting();
}

/*---------------------------------------------------------------------------
 *            ble_disconnect
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    BLE disconnect
 *
 * Parameters:
 *    idx - connection index
 *
 * Return:
 *    void
 */
static void ble_disconnect(uint8_t idx)
{
    ble_set_actv_action(BLE_ACTV_ACTION_DISCONNECTING, 0xFF);
    appm_disconnect(idx);
}

void app_ble_update_param_failed(uint8_t conidx, uint8_t errCode)
{
    bool isEnd = app_ble_update_ci_fail_handle(conidx, errCode);
    if (!isEnd)
    {
        return;
    }
    ble_event_t event;
    event.evt_type = BLE_CONN_PARAM_UPDATE_FAILED_EVENT;
    event.p.conn_param_update_failed_handled.conidx = conidx;
    event.p.conn_param_update_failed_handled.err_code = errCode;
    app_ble_core_global_handle(&event, NULL);
}

void app_ble_update_param_successful(uint8_t conidx, APP_BLE_CONN_PARAM_T *pConnParam)
{
    app_ble_update_ci_success_handle(conidx);

    ble_event_t event;
    event.evt_type = BLE_CONN_PARAM_UPDATE_SUCCESSFUL_EVENT;
    event.p.conn_param_update_successful_handled.conidx = conidx;
    event.p.conn_param_update_successful_handled.con_interval = pConnParam->con_interval;
    event.p.conn_param_update_successful_handled.con_latency = pConnParam->con_latency;
    event.p.conn_param_update_successful_handled.sup_to = pConnParam->sup_to;
    app_ble_core_global_handle(&event, NULL);
}

/**
 * @brief : callback function of BLE connect failed
 *
 */
static void app_ble_connecting_failed_handler(ble_bdaddr_t *peer_addr, uint8_t err_code)
{
    LOG_I("%s", __func__);

    ble_event_t event;
    event.evt_type = BLE_CONNECTING_FAILED_EVENT;
    event.p.connecting_failed_handled.actv_idx = 0;
    event.p.connecting_failed_handled.err_code = err_code;
    memcpy((uint8_t *)&event.p.connecting_failed_handled.peer_bdaddr, (uint8_t *)peer_addr, sizeof(ble_bdaddr_t));
    app_ble_core_global_handle(&event, NULL);
}

void app_connecting_failed(ble_bdaddr_t *peer_addr, uint8_t err_code)
{
    app_bt_start_custom_function_in_bt_thread((uint32_t)peer_addr,
                                              err_code,
                                              (uint32_t)app_ble_connecting_failed_handler);
}

void app_ble_on_bond_status_changed(uint8_t conidx, bool success, uint16_t reason)
{
    LOG_I("%s", __func__);

    ble_event_t event;

    event.evt_type = BLE_CONNECT_BOND_EVENT;
    event.p.connect_bond_handled.conidx = conidx;
    event.p.connect_bond_handled.success = success;
    event.p.connect_bond_handled.reason = reason;
    app_ble_core_global_handle(&event, NULL);
}

void app_ble_on_bond_failed(uint8_t conidx)
{
    LOG_I("%s", __func__);

    ble_event_t event;

    event.evt_type = BLE_CONNECT_BOND_FAIL_EVENT;
    event.p.connect_bond_handled.conidx = conidx;
    app_ble_core_global_handle(&event, NULL);
}

void app_ble_rpa_addr_parsed_success(uint8_t conidx)
{
    LOG_I("%s", __func__);

    ble_event_t event;

    event.evt_type = BLE_RPA_ADDR_PARSED_EVENT;
    event.p.connect_bond_handled.conidx = conidx;
    app_ble_core_global_handle(&event, NULL);
}

void app_ble_on_encrypt_success(uint8_t conidx, uint8_t pairing_lvl)
{
    LOG_I("%s", __func__);

    ble_event_t event;
    ble_bdaddr_t remote_addr = {{0}};

    event.evt_type = BLE_CONNECT_ENCRYPT_EVENT;
    event.p.connect_encrypt_handled.conidx = conidx;
    event.p.connect_encrypt_handled.pairing_lvl = pairing_lvl;
    app_ble_get_peer_solved_addr(conidx, &remote_addr);
    memcpy(event.p.connect_encrypt_handled.addr, remote_addr.addr, BTIF_BD_ADDR_SIZE);
    event.p.connect_encrypt_handled.addr_type = app_ble_get_peer_solved_addr_type(conidx);
    app_ble_core_global_handle(&event, NULL);
}

void app_ble_connected_evt_handler(uint8_t conidx, ble_bdaddr_t *pPeerBdAddress)
{
    ble_event_t event;

    app_ble_clear_update_ci_list(conidx);

    event.evt_type = BLE_LINK_CONNECTED_EVENT;
    event.p.connect_handled.conidx = conidx;
    memcpy(&event.p.connect_handled.peer_bdaddr, pPeerBdAddress, sizeof(ble_bdaddr_t));
    app_ble_core_global_handle(&event, NULL);
}

void app_ble_disconnected_evt_handler(uint8_t conidx, uint8_t errCode)
{
    ble_event_t event;

    app_ble_clear_update_ci_list(conidx);
    ble_execute_pending_op(BLE_ACTV_ACTION_DISCONNECTING, 0xFF);
    /// Ble conn is terminated, we can try to remove start adv fail flag
    bleModeEnv.isStartAdvFailbf = 0;

    event.evt_type = BLE_DISCONNECT_EVENT;
    event.p.disconnect_handled.conidx = conidx;
    event.p.disconnect_handled.errCode = errCode;
    app_ble_core_global_handle(&event, NULL);

    //if disconnected after set white list or rpa list, will break pending list cause busy
    app_ble_set_list();
}

void app_connecting_stopped(ble_bdaddr_t *peer_addr)
{
    LOG_I("%s peer addr:", __func__);
    DUMP8("%2x ", peer_addr->addr, BT_ADDR_OUTPUT_PRINT_NUM);

    bleModeEnv.ble_is_connecting = false;

    ble_event_t event;

    event.evt_type = BLE_CONNECTING_STOPPED_EVENT;
    memcpy(event.p.stopped_connecting_handled.peer_bdaddr, peer_addr->addr, BTIF_BD_ADDR_SIZE);
    app_ble_core_global_handle(&event, NULL);
}

static uint8_t app_ble_get_adv_user_from_activity_index(uint8_t actv_idx)
{
    uint8_t actv_user;
    for (actv_user = BLE_ADV_ACTIVITY_USER_0; actv_user < BLE_ADV_ACTIVITY_USER_NUM; actv_user++)
    {
        if (actv_idx == bleModeEnv.bleEnv->adv_actv_idx[actv_user])
        {
            return actv_user;
        }
    }

    return BLE_ADV_ACTIVITY_USER_NUM;
}

void app_advertising_started(uint8_t actv_idx)
{
    LOG_I("%s", __func__);

    uint8_t actv_user = app_ble_get_adv_user_from_activity_index(actv_idx);
    if (BLE_ADV_ACTIVITY_USER_NUM != actv_user)
    {
        if (bleModeEnv.advCurrentTxpwr[actv_user] != bleModeEnv.advPendingTxpwr[actv_user])
        {
            app_ble_set_adv_txpwr_by_actv_user(actv_user, bleModeEnv.advPendingTxpwr[actv_user]);
        }

        if (bleModeEnv.advCurrentInfo[actv_user].localAddrType == GAPM_GEN_RSLV_ADDR)
        {
            appm_read_rpa_addr();
        }
    }

    ble_event_t event;

    event.evt_type = BLE_ADV_STARTED_EVENT;
    event.p.adv_started_handled.actv_user = actv_user;
    app_ble_core_global_handle(&event, NULL);

    TRACE(0, "actv user %d started.", actv_user);

    app_ble_refresh_adv_state_generic(BLE_ADVERTISING_INTERVAL);
}

void app_advertising_stopped(uint8_t actv_idx)
{
    LOG_I("%s", __func__);

    uint8_t actv_user = app_ble_get_adv_user_from_activity_index(actv_idx);

    /// Check whether some adv is stop normally
    /// and we can try to remove adv fail flag
    if ((bleModeEnv.isStartAdvFailbf & CO_BIT(actv_user)) == false)
    {
        bleModeEnv.isStartAdvFailbf = 0;
    }

    if (BLE_ADV_ACTIVITY_USER_NUM != actv_user)
    {
        bleModeEnv.advCurrentTxpwr[actv_user] = BLE_ADV_INVALID_TXPWR;
#ifdef CUSTOMER_DEFINE_ADV_DATA
        if (!app_env.need_restart_adv)
        {
            app_ble_custom_adv_clear_enabled_flag(actv_user);
        }
#endif
    }

    ble_event_t event;

    event.evt_type = BLE_ADV_STOPPED_EVENT;
    event.p.adv_stopped_handled.actv_user = actv_user;
    app_ble_core_global_handle(&event, NULL);

    app_ble_refresh_adv_state_generic(BLE_ADVERTISING_INTERVAL);
}

static void app_advertising_starting_failed_handle(uint8_t actv_idx, uint8_t err_code)
{
    LOG_I("%s", __func__);

    uint8_t actv_user = app_ble_get_adv_user_from_activity_index(actv_idx);

    ble_event_t event;

    event.evt_type = BLE_ADV_STARTING_FAILED_EVENT;
    event.p.adv_starting_failed_handled.actv_user = actv_user;
    event.p.adv_starting_failed_handled.err_code = err_code;
    app_ble_core_global_handle(&event, NULL);
}

void app_advertising_starting_failed(uint8_t actv_idx, uint8_t err_code)
{
    /// Update ble_actv_action = BLE_ACTV_ACTION_STOPPING_ADV
    ble_set_actv_action(BLE_ACTV_ACTION_STOPPING_ADV, actv_idx);
    uint8_t actv_user = app_ble_get_adv_user_from_activity_index(actv_idx);
    ///Mark that adv start fail
    bleModeEnv.isStartAdvFailbf |= CO_BIT(actv_user);

    app_bt_start_custom_function_in_bt_thread(actv_idx,
                                              err_code,
                                              (uint32_t)app_advertising_starting_failed_handle);
}

// BLE APIs for external use
void app_ble_data_fill_enable(BLE_ADV_USER_E user, bool enable)
{
    ASSERT(user < BLE_ADV_USER_NUM, "%s user %d", __func__, user);
    LOG_I("%s user %d%s enable %d", __func__, user, ble_adv_user2str(user), enable);

    uint8_t actv_user = app_ble_param_get_actv_user_from_adv_user(user);

    if (actv_user == BLE_ADV_ACTIVITY_USER_NUM)
    {
        LOG_I("%s can't find actv user %d", __func__, actv_user);
        return;
    }

    if (enable)
    {
        bleModeEnv.adv_user_enable[actv_user] |= (1 << user);
    }
    else
    {
        bleModeEnv.adv_user_enable[actv_user] &= ~(1 << user);
    }
}

// BLE APIs for external use
void app_ble_register_data_fill_handle(BLE_ADV_USER_E user, BLE_DATA_FILL_FUNC_T func, bool enable)
{
    LOG_I("%s user %d", __func__, user);
    if (BLE_ADV_USER_NUM <= user)
    {
        LOG_W("invalid user");
    }
    else
    {
        if (func != bleModeEnv.bleDataFillFunc[user] &&
                NULL != func)
        {
            bleModeEnv.bleDataFillFunc[user] = func;
        }
    }

    bleModeEnv.adv_user_register |= (1 << user);
}

void app_ble_system_ready(void)
{
    app_ble_customif_init();
    app_notify_stack_ready(STACK_READY_BLE);

    ble_uart_cmd_init();
#ifdef IS_BLE_AUDIO_DEBUG_INFO_COLLECTOR_ENABLED
    app_dbg_ble_info_system_init();
#endif
    app_ble_add_devices_info_to_resolving();
}

void app_ble_add_dev_to_rpa_list_in_controller(const ble_bdaddr_t *ble_addr, const uint8_t *irk)
{
    uint8_t pIrk[16] = {0};
    ble_gap_ral_dev_info_t devicesInfo[1];

    nv_record_blerec_get_local_irk(&pIrk[0]);
    (devicesInfo + 0)->priv_mode = 0;
    memcpy((devicesInfo + 0)->local_irk, &pIrk[0], 16);
    (devicesInfo + 0)->addr.addr_type = (ble_addr->addr_type & 0x01);
    memcpy((devicesInfo + 0)->addr.addr, ble_addr->addr, 6);
    memcpy((devicesInfo + 0)->peer_irk, irk, 16);

    appm_add_multiple_devices_to_resolving_list_in_controller(devicesInfo, 1);
}

void app_ble_add_devices_info_to_resolving(void)
{
    ble_gap_ral_dev_info_t devicesInfo[RESOLVING_LIST_MAX_NUM];
    uint8_t devicesInfoNumber = appm_prepare_devices_info_added_to_resolving_list(devicesInfo);
    appm_add_multiple_devices_to_resolving_list_in_controller(devicesInfo, devicesInfoNumber);
}

void app_ble_set_resolving_list_complete(uint16_t status)
{
    ble_event_t event;

    LOG_I("%s", __func__);

    app_env.isNeedToAddDevicesToReslovingList = false;
    app_env.addingDevicesToReslovingList = false;
    ble_execute_pending_op(BLE_ACTV_ACTION_ADD_RESLO_LIST, 0xFF);
    if (app_env.isNeedToRestartScan)
    {
        app_env.isNeedToRestartScan = false;
        app_ble_start_scan(&app_env.ble_scan_param);
    }
    else
    {
        app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
    }

    event.evt_type = BLE_SET_RAL_CMP_EVENT;
    event.p.set_ral_cmp_handled.status = status;

    app_ble_core_global_handle(&event, NULL);
}

static void ble_adv_refreshing(void *param)
{
    BLE_ADV_PARAM_T *pAdvParam = (BLE_ADV_PARAM_T *)param;
    uint8_t actv_user = pAdvParam->adv_actv_user;
    uint8_t actv_idx = bleModeEnv.bleEnv->adv_actv_idx[actv_user];
    BLE_ADV_PARAM_T *pCurrentAdvParam = &bleModeEnv.advCurrentInfo[actv_user];
    // five conditions that we just need to update the ble adv data instead of restarting ble adv
    // 1. BLE advertising is on
    // 2. No on-going BLE operation
    // 3. BLE adv type is the same
    // 4. BLE adv interval is the same
    // 5. BLE adv mode type is the same
    // TODO: to be re-enabled when appm_update_adv_data is ready
    if ((INVALID_BLE_ACTIVITY_INDEX != actv_idx) && \
            (APP_ADV_STATE_STARTED == bleModeEnv.bleEnv->state[actv_idx]) && \
            pCurrentAdvParam->advType == pAdvParam->advType && \
            pCurrentAdvParam->advMode == pAdvParam->advMode && \
            pCurrentAdvParam->discMode == pAdvParam->discMode && \
            pCurrentAdvParam->advInterval == pAdvParam->advInterval && \
            pCurrentAdvParam->advTxPwr == pAdvParam->advTxPwr && \
            pCurrentAdvParam->filter_pol == pAdvParam->filter_pol && \
            pCurrentAdvParam->localAddrType == pAdvParam->localAddrType && \
            !memcmp(pCurrentAdvParam->localAddr, pAdvParam->localAddr, BTIF_BD_ADDR_SIZE))
    {
        memcpy(pCurrentAdvParam, param, sizeof(BLE_ADV_PARAM_T));
        ble_set_actv_action(BLE_ACTV_ACTION_STARTING_ADV, 0xFF);
        appm_update_adv_data(pCurrentAdvParam);
    }
    else
    {
        // otherwise, restart ble adv
        ble_start_adv(param);
    }
}

static bool app_ble_adv_param_is_different(BLE_ADV_PARAM_T *p_cur_adv, BLE_ADV_PARAM_T *p_dst_adv)
{
    uint8_t ret = 0;

    if ((p_cur_adv->isBleFlagsAdvDataConfiguredByAppLayer != p_dst_adv->isBleFlagsAdvDataConfiguredByAppLayer) ||
            (p_cur_adv->filter_pol != p_dst_adv->filter_pol) ||
            (p_cur_adv->advType != p_dst_adv->advType) ||
            (p_cur_adv->discMode != p_dst_adv->discMode) ||
            (p_cur_adv->advInterval != p_dst_adv->advInterval) ||
            (p_cur_adv->advTxPwr != p_dst_adv->advTxPwr) ||
            (p_cur_adv->advDataLen != p_dst_adv->advDataLen) ||
            (p_cur_adv->scanRspDataLen != p_dst_adv->scanRspDataLen))
    {
        ret = 1;
    }
    else if (memcmp(p_cur_adv->advData, p_dst_adv->advData, EXT_ADV_DATA_LEN))
    {
        ret = 2;
    }
    else if (memcmp(p_cur_adv->scanRspData, p_dst_adv->scanRspData, EXT_ADV_DATA_LEN))
    {
        ret = 3;
    }
    else if (((p_cur_adv->advType == ADV_TYPE_DIRECT_LDC) ||
              (p_cur_adv->advType == ADV_TYPE_DIRECT_HDC)) &&
             (memcmp((uint8_t *)&p_cur_adv->peerAddr, (uint8_t *)&p_dst_adv->peerAddr, sizeof(ble_bdaddr_t))))
    {
        ret = 4;
    }
    else if (memcmp(p_cur_adv->localAddr, p_dst_adv->localAddr, BTIF_BD_ADDR_SIZE))
    {
#ifdef CUSTOMER_DEFINE_ADV_DATA
        uint8_t addr[BTIF_BD_ADDR_SIZE] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
        if (memcmp(addr, p_dst_adv->localAddr, BTIF_BD_ADDR_SIZE))
        {
            ret = 5;
        }
#else
        ret = 5;
#endif
    }

    if (ret)
    {
        LOG_I("%s ret %d", __func__, ret);
        return true;
    }

    return false;
}

/*---------------------------------------------------------------------------
 *            app_ble_start_adv
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    start BLE advertisement
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
static bool app_ble_start_adv(void)
{
    uint32_t adv_user_enable = 0;
    uint8_t actv_idx = 0xFF;
    BLE_ADV_PARAM_T *pCurrentAdvParam;
    BLE_ADV_PARAM_T *pAdvParamInfo;
    //LOG_I("[ADV] interval:%d ca:%p", bleModeEnv.advPendingInterval, __builtin_return_address(0));

    if (!ble_adv_is_allowed())
    {
        LOG_I("[ADV] not allowed.");
        for (uint8_t actv_user = BLE_ADV_ACTIVITY_USER_0; actv_user < BLE_ADV_ACTIVITY_USER_NUM; actv_user++)
        {
            actv_idx = bleModeEnv.bleEnv->adv_actv_idx[actv_user];
            if (INVALID_BLE_ACTIVITY_INDEX != actv_idx)
            {
                ble_stop_advertising(actv_idx);
                return false;
            }
        }
        ble_execute_pending_op(BLE_ACTV_ACTION_IDLE, 0xFF);
        return false;
    }

    for (uint8_t actv_user = BLE_ADV_ACTIVITY_USER_0; actv_user < BLE_ADV_ACTIVITY_USER_NUM; actv_user++)
    {
        if (bleModeEnv.isStartAdvFailbf & CO_BIT(actv_user))
        {
            LOG_I("[WARNING] actv_user %d start adv fail before, cann't start adv now", actv_user);
            continue;
        }

        actv_idx = bleModeEnv.bleEnv->adv_actv_idx[actv_user];
        adv_user_enable = bleModeEnv.adv_user_enable[actv_user];

        ble_adv_config_param(actv_user);

        LOG_I("%s old_user_enable 0x%x new 0x%x %d %d", __func__, adv_user_enable, bleModeEnv.adv_user_enable[actv_user],
              actv_user, BLE_ADV_ACTIVITY_USER_NUM);
        if (!bleModeEnv.adv_user_enable[actv_user])
        {
            LOG_I("no adv user enable");

            if ((INVALID_BLE_ACTIVITY_INDEX != actv_idx) &&
                    (APP_ADV_STATE_STARTED == bleModeEnv.bleEnv->state[actv_idx]))
            {
                ble_stop_advertising(actv_idx);
                return false;
            }
            continue;
        }

        pCurrentAdvParam = &bleModeEnv.advCurrentInfo[actv_user];
        pAdvParamInfo = &bleModeEnv.advParamInfo;
        pAdvParamInfo->adv_actv_user = actv_user;

        // param of adv request is exactly same as current adv
        if ((INVALID_BLE_ACTIVITY_INDEX != actv_idx) &&
                (APP_ADV_STATE_STARTED == bleModeEnv.bleEnv->state[actv_idx]) &&
                (!app_ble_adv_param_is_different(pCurrentAdvParam, pAdvParamInfo)))
        {
            LOG_I("adv param not changed, do nothing");
        }
        else
        {
            LOG_I("[ADV_LEN] %d [DATA]:", bleModeEnv.advParamInfo.advDataLen);
            DUMP8("%02x ", bleModeEnv.advParamInfo.advData, bleModeEnv.advParamInfo.advDataLen);
            LOG_I("[SCAN_RSP_LEN] %d [DATA]:", bleModeEnv.advParamInfo.scanRspDataLen);
            DUMP8("%02x ", bleModeEnv.advParamInfo.scanRspData, bleModeEnv.advParamInfo.scanRspDataLen);

            ble_adv_refreshing(pAdvParamInfo);
            return true;
        }
    }

    ble_execute_pending_op(BLE_ACTV_ACTION_IDLE, 0xFF);
    return true;
}

static void app_ble_start_connectable_adv_generic(uint16_t advInterval)
{
    LOG_I("%s ble_is_busy %d", __func__, bleModeEnv.ble_is_busy);
    if (bleModeEnv.ble_is_busy)
    {
        bleModeEnv.adv_has_pending_op = true;
    }
    else
    {
        bleModeEnv.ble_is_busy = true;
        app_bt_start_custom_function_in_bt_thread(0, 0, (uint32_t)app_ble_start_adv);
    }
}

void app_ble_start_connectable_adv(uint16_t advInterval)
{
#ifndef CUSTOMER_DEFINE_ADV_DATA
    app_ble_start_connectable_adv_generic(advInterval);
#endif
}

void app_ble_start_connectable_adv_by_custom_adv(uint16_t advInterval)
{
    app_ble_start_connectable_adv_generic(advInterval);
}

static void app_ble_refresh_adv_state_generic(uint16_t advInterval)
{
    LOG_I("%s ble_is_busy %d", __func__, bleModeEnv.ble_is_busy);
    if (bleModeEnv.ble_is_busy)
    {
        bleModeEnv.adv_has_pending_op = true;
    }
    else
    {
        bleModeEnv.ble_is_busy = true;
        app_bt_start_custom_function_in_bt_thread(0, 0, (uint32_t)app_ble_start_adv);
    }
}

void app_ble_refresh_adv_state(uint16_t advInterval)
{
#ifndef CUSTOMER_DEFINE_ADV_DATA
    app_ble_refresh_adv_state_generic(advInterval);
#endif
}

void app_ble_refresh_adv_state_by_custom_adv(uint16_t advInterval)
{
    app_ble_refresh_adv_state_generic(advInterval);
}

void app_ble_set_adv_type(BLE_ADV_TYPE_E advType, ble_bdaddr_t *peer_addr)
{
    LOG_I("%s %d -> %d", __func__, bleModeEnv.advPendingType, advType);
    bleModeEnv.advPendingType = advType;
    if ((advType == ADV_TYPE_DIRECT_LDC) ||
            (advType == ADV_TYPE_DIRECT_HDC))
    {
        memcpy((uint8_t *)&bleModeEnv.advPendingPeerAddr, (uint8_t *)peer_addr, sizeof(ble_bdaddr_t));
    }

    app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
}

void app_ble_set_adv_disc_mode(ADV_DISC_MODE_E discMode)
{
    LOG_I("%s %d -> %d", __func__, bleModeEnv.advPendingDiscmode, discMode);
    bleModeEnv.advPendingDiscmode = discMode;
}

void app_ble_set_adv_txpwr_by_actv_user(BLE_ADV_ACTIVITY_USER_E actv_user, int8_t txpwr_dbm)
{
    ASSERT(actv_user < BLE_ADV_ACTIVITY_USER_NUM, "%s user %d", __func__, actv_user);

    uint8_t adv_hdl = bleModeEnv.bleEnv->adv_actv_idx[actv_user];
    uint8_t txpwr_level = app_ble_param_get_adv_tx_power_level(actv_user);
    LOG_I("%s %d hdl 0x%02x level %d dbm %d", __func__, actv_user, adv_hdl, txpwr_level, txpwr_dbm);

    bleModeEnv.advPendingTxpwr[actv_user] = txpwr_dbm;
    if (INVALID_BLE_ACTIVITY_INDEX != adv_hdl &&
            txpwr_dbm != bleModeEnv.advCurrentTxpwr[actv_user])
    {
        bleModeEnv.advCurrentTxpwr[actv_user] = txpwr_dbm;
        bt_drv_ble_adv_txpwr_via_advhdl(adv_hdl, txpwr_level, txpwr_dbm);
    }
}

void app_ble_set_adv_txpwr_by_adv_user(BLE_ADV_USER_E user, int8_t txpwr_dbm)
{
    ASSERT(user < BLE_ADV_USER_NUM, "%s %d", __func__, user);
    LOG_I("%s %d%s", __func__, user, ble_adv_user2str(user));

    uint8_t actv_user = app_ble_param_get_actv_user_from_adv_user(user);

    if (actv_user == BLE_ADV_ACTIVITY_USER_NUM)
    {
        LOG_I("%s can't find actv user %d", __func__, actv_user);
        return;
    }

    app_ble_set_adv_txpwr_by_actv_user(actv_user, txpwr_dbm);
}

void app_ble_set_all_adv_txpwr(int8_t txpwr_dbm)
{
    uint8_t actv_user;
    for (actv_user = BLE_ADV_ACTIVITY_USER_0; actv_user < BLE_ADV_ACTIVITY_USER_NUM; actv_user++)
    {
        app_ble_set_adv_txpwr_by_actv_user(actv_user, txpwr_dbm);
    }
}

void app_ble_set_adv_local_addr_by_actv_user(BLE_ADV_ACTIVITY_USER_E actv_user, uint8_t *addr)
{
    ASSERT(actv_user < BLE_ADV_ACTIVITY_USER_NUM, "%s user %d", __func__, actv_user);
    LOG_I("%s %d", __func__, actv_user);
    DUMP8("0x%02x ", addr, BT_ADDR_OUTPUT_PRINT_NUM);

    memcpy((uint8_t *)&bleModeEnv.advPendingLocalAddr[actv_user][0], addr, BTIF_BD_ADDR_SIZE);
}

void app_ble_set_adv_local_addr_by_adv_user(BLE_ADV_USER_E user, uint8_t *addr)
{
    ASSERT(user < BLE_ADV_USER_NUM, "%s %d", __func__, user);
    LOG_I("%s %d%s", __func__, user, ble_adv_user2str(user));

    uint8_t actv_user = app_ble_param_get_actv_user_from_adv_user(user);

    if (actv_user == BLE_ADV_ACTIVITY_USER_NUM)
    {
        LOG_I("%s can't find actv user %d", __func__, actv_user);
        return;
    }

    app_ble_set_adv_local_addr_by_actv_user(actv_user, addr);
}

void app_ble_start_scan(BLE_SCAN_PARAM_T *param)
{
    LOG_I("%s ble_is_busy %d", __func__, bleModeEnv.ble_is_busy);
    bleModeEnv.scanPendingInfo.scanType  = param->scanType;
    bleModeEnv.scanPendingInfo.scanFolicyType = param->scanFolicyType;
    bleModeEnv.scanPendingInfo.scanWindowMs = param->scanWindowMs;
    bleModeEnv.scanPendingInfo.scanIntervalMs = param->scanIntervalMs;
    bleModeEnv.scanPendingInfo.scanDurationMs = param->scanDurationMs;

    if (bleModeEnv.ble_is_busy)
    {
        bleModeEnv.scan_has_pending_op = true;
    }
    else
    {
        bleModeEnv.ble_is_busy = true;
        app_bt_start_custom_function_in_bt_thread(0, 0, (uint32_t)ble_start_scan);
    }
}

void app_ble_stop_scan(void)
{
    LOG_D("%s", __func__);
    if (bleModeEnv.ble_is_busy)
    {
        bleModeEnv.scan_has_pending_stop_op = true;
    }
    else
    {
        bleModeEnv.ble_is_busy = true;
        app_bt_start_custom_function_in_bt_thread(0, 0, (uint32_t)ble_stop_scanning);
    }
}

void app_scanning_started(void)
{
    LOG_I("%s", __func__);

    ble_event_t event;

    event.evt_type = BLE_SCAN_STARTED_EVENT;
    app_ble_core_global_handle(&event, NULL);
}

void app_scanning_stopped(void)
{
    LOG_I("%s", __func__);

    ble_event_t event;

    event.evt_type = BLE_SCAN_STOPPED_EVENT;
    app_ble_core_global_handle(&event, NULL);
}

/**
 * @brief : callback function of BLE scan starting failed
 *
 */
static void app_ble_scanning_starting_failed_handler(uint8_t actv_idx, uint8_t err_code)
{
    LOG_I("%s", __func__);

    ble_event_t event;
    event.evt_type = BLE_SCAN_STARTING_FAILED_EVENT;
    event.p.scan_starting_failed_handled.actv_idx = actv_idx;
    event.p.scan_starting_failed_handled.err_code = err_code;
    app_ble_core_global_handle(&event, NULL);
}

void app_scanning_starting_failed(uint8_t actv_idx, uint8_t err_code)
{
    app_bt_start_custom_function_in_bt_thread(actv_idx,
                                              err_code,
                                              (uint32_t)app_ble_scanning_starting_failed_handler);
}

void app_ble_credit_based_conn_request_ind_handler(
    bool isEnhanced, uint16_t spsm, uint16_t mtu, uint16_t mps, uint16_t initial_credits)
{
    LOG_I("%s", __func__);

    ble_event_t event;

    event.p.credit_based_conn_req_handled.isEnhanced = isEnhanced;
    event.p.credit_based_conn_req_handled.spsm = spsm;
    event.p.credit_based_conn_req_handled.mtu = mtu;
    event.p.credit_based_conn_req_handled.mps = mps;
    event.p.credit_based_conn_req_handled.initial_credits = initial_credits;

    event.evt_type = BLE_CREDIT_BASED_CONN_REQ_EVENT;
    app_ble_core_global_handle(&event, NULL);
}

/*---------------------------------------------------------------------------
 *            app_ble_pending_to_connect
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    check if BLE pending to connect
 *
 * Parameters:
 *    void
 *
 * Return:
 *    bool -- true means ble pending to connect
 */
static bool app_ble_pending_to_connect(void)
{
    uint8_t index = 0;
    for (index = 0; index < BLE_CONNECTION_MAX; index++)
    {
        if (bleModeEnv.bleToConnectInfo[index].pendingConnect)
        {
            return true;
        }
    }

    return false;
}

void app_ble_start_connect(const ble_bdaddr_t *peer_addr, uint8_t owner_addr_type)
{
    app_ble_start_connect_with_init_type(APP_BLE_INIT_TYPE_DIRECT_CONN_EST, (ble_bdaddr_t *)peer_addr, owner_addr_type, 0);
}

void app_ble_start_connect_with_white_list(ble_bdaddr_t *addr_list, uint8_t size, uint8_t owner_addr_type, uint16_t conn_to)
{
    app_ble_set_white_list(BLE_WHITE_LIST_USER_MOBILE, addr_list, size);
    app_ble_start_connect_with_init_type(APP_BLE_INIT_TYPE_AUTO_CONN_EST, NULL, owner_addr_type, conn_to);
}

void app_ble_start_connect_with_init_type(BLE_INIT_TYPE_E init_type, ble_bdaddr_t *peer_addr, uint8_t owner_addr_type, uint16_t conn_to)
{
    uint8_t index = 0;

    if (APP_BLE_INIT_TYPE_DIRECT_CONN_EST == init_type)
    {
        for (index = 0; index < BLE_CONNECTION_MAX; index++)
        {
            if (bleModeEnv.bleToConnectInfo[index].pendingConnect)
            {
                if (!memcmp(&bleModeEnv.bleToConnectInfo[index].init_param.peer_addr, peer_addr, sizeof(ble_bdaddr_t)))
                {
                    TRACE(0, "%s already record", __func__);
                    return;
                }
            }
        }
    }

    for (index = 0; index < BLE_CONNECTION_MAX; index++)
    {
        if (!bleModeEnv.bleToConnectInfo[index].pendingConnect)
        {
            bleModeEnv.bleToConnectInfo[index].pendingConnect = true;
            if (APP_BLE_INIT_TYPE_DIRECT_CONN_EST == init_type)
            {
                memcpy(&bleModeEnv.bleToConnectInfo[index].init_param.peer_addr, peer_addr, sizeof(ble_bdaddr_t));
            }
            bleModeEnv.bleToConnectInfo[index].init_param.gapm_init_type = init_type;
            bleModeEnv.bleToConnectInfo[index].init_param.own_addr_type  = owner_addr_type;
            bleModeEnv.bleToConnectInfo[index].init_param.conn_to = conn_to;
            break;
        }
    }
    ASSERT(index < BLE_CONNECTION_MAX, "%s has too many ble to connect, index %d", __func__, index);

    LOG_I("%s ble_is_busy %d ble_is_connecting %d", __func__, bleModeEnv.ble_is_busy, bleModeEnv.ble_is_connecting);

    if ((!bleModeEnv.ble_is_busy) && (!bleModeEnv.ble_is_connecting))
    {
        bleModeEnv.ble_is_busy = true;
        bleModeEnv.ble_is_connecting = true;
        app_bt_start_custom_function_in_bt_thread(0, 0, (uint32_t)ble_start_connect);
    }
}

void app_ble_cancel_connecting(void)
{
    LOG_I("%s ble_is_busy %d", __func__, bleModeEnv.ble_is_busy);

    if (bleModeEnv.ble_is_busy)
    {
        bleModeEnv.pending_stop_connecting = true;
    }
    else
    {
        bleModeEnv.ble_is_busy = true;
        app_bt_start_custom_function_in_bt_thread(0, 0, (uint32_t)ble_stop_connecting);
    }
}

bool app_ble_is_connection_on(uint8_t index)
{
    return app_ble_is_connection_on_by_index(index);
}

bool app_ble_is_any_connection_exist(void)
{
    bool ret = false;
    for (uint8_t i = 0; i < BLE_CONNECTION_MAX; i++)
    {
        if (app_ble_is_connection_on_by_index(i))
        {
            ret = true;
        }
    }

    return ret;
}

/*---------------------------------------------------------------------------
 *            app_ble_index_pending_to_disconnect
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    get the index of BLE pending to disconnect
 *
 * Parameters:
 *    void
 *
 * Return:
 *    uint8_t -- the index of ble pending to disconnect
 */
static uint8_t app_ble_index_pending_to_disconnect(void)
{
    uint8_t index = 0;
    for (index = 0; index < BLE_CONNECTION_MAX; index++)
    {
        if (bleModeEnv.pendingDisconnect[index])
        {
            return index;
        }
    }

    return BLE_CONNECTION_MAX;
}

void app_ble_pts_start_disconnect(uint8_t conIdx)
{
    uint16_t conhdl = app_ble_get_conhdl(conIdx);
    LOG_I("%s conidx %d", __func__, conIdx);

    if (GAP_INVALID_CONHDL != conhdl)
    {
        app_bt_start_custom_function_in_bt_thread((uint32_t)conIdx,
                                                  0,
                                                  (uint32_t)ble_disconnect);
    }
}

void app_ble_start_disconnect(uint8_t conIdx)
{
    LOG_I("%s conidx %d,busy = %d", __func__, conIdx, bleModeEnv.ble_is_busy);

    if (!bleModeEnv.ble_is_busy)
    {
        bleModeEnv.ble_is_busy = true;
        app_bt_start_custom_function_in_bt_thread((uint32_t)conIdx,
                                                  0,
                                                  (uint32_t)ble_disconnect);
    }
    else
    {
        bleModeEnv.pendingDisconnect[conIdx] = true;
    }
}

void app_ble_disconnect_all(void)
{
    uint8_t index = 0;

    LOG_I("%s %p", __func__,  __builtin_return_address(0));
    for (index = 0; index < BLE_CONNECTION_MAX; index++)
    {
        bleModeEnv.bleToConnectInfo[index].pendingConnect = false;
        app_ble_start_disconnect(index);
    }
}

void app_ble_force_switch_adv(enum BLE_ADV_SWITCH_USER_E user, bool isToEnableAdv)
{
    ASSERT(user < BLE_SWITCH_USER_NUM, "ble switch user exceed");

    if (!isToEnableAdv)
    {
        bleModeEnv.disablerBitmap |= (1 << user);
    }
    else
    {
        bleModeEnv.disablerBitmap &= ~(1 << user);
    }

    LOG_I("%s user %d isToDisableAdv %d disablerBitmap 0x%x", __func__,
          user, isToEnableAdv, bleModeEnv.disablerBitmap);

    app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
    LOG_I("%s user %d isToEnableAdv %d disablerBitmap 0x%x", __func__,
          user, isToEnableAdv, bleModeEnv.disablerBitmap);
}

bool app_ble_is_in_advertising_state(void)
{
    uint8_t actv_idx = 0xFF;

    for (uint8_t i = 0; i < BLE_ADV_ACTIVITY_USER_NUM; i++)
    {
        actv_idx = app_env.adv_actv_idx[i];
        if (actv_idx != INVALID_BLE_ACTIVITY_INDEX &&
                APP_ADV_STATE_STARTED == bleModeEnv.bleEnv->state[actv_idx])
        {
            return true;
        }
    }

    return false;
}

uint32_t POSSIBLY_UNUSED ble_get_manufacture_data_ptr(uint8_t *advData,
                                                      uint32_t dataLength,
                                                      uint8_t *manufactureData)
{
    uint8_t followingDataLengthOfSection;
    uint8_t rawContentDataLengthOfSection;
    uint8_t flag;
    while (dataLength > 0)
    {
        followingDataLengthOfSection = *advData++;
        dataLength--;
        if (dataLength < followingDataLengthOfSection)
        {
            return 0; // wrong adv data format
        }

        if (followingDataLengthOfSection > 0)
        {
            flag = *advData++;
            dataLength--;

            rawContentDataLengthOfSection = followingDataLengthOfSection - 1;
            if (BLE_ADV_MANU_FLAG == flag)
            {
                uint32_t lengthToCopy;
                if (dataLength < rawContentDataLengthOfSection)
                {
                    lengthToCopy = dataLength;
                }
                else
                {
                    lengthToCopy = rawContentDataLengthOfSection;
                }

                memcpy(manufactureData, advData - 2, lengthToCopy + 2);
                return lengthToCopy + 2;
            }
            else
            {
                advData += rawContentDataLengthOfSection;
                dataLength -= rawContentDataLengthOfSection;
            }
        }
    }

    return 0;
}

//received adv data
void app_adv_reported_scanned(struct gapm_ext_adv_report_ind *ptInd)
{
    ble_event_t event;
    event.evt_type = BLE_SCAN_DATA_REPORT_EVENT;
    memcpy(&event.p.scan_data_report_handled.trans_addr, &ptInd->trans_addr, sizeof(ble_bdaddr_t));
    event.p.scan_data_report_handled.rssi = ptInd->rssi;
    event.p.scan_data_report_handled.info = ptInd->info;
    event.p.scan_data_report_handled.length = (ptInd->length > EXT_ADV_DATA_MAX_LEN) ? EXT_ADV_DATA_MAX_LEN : ptInd->length;
    memcpy(event.p.scan_data_report_handled.data, ptInd->data, event.p.scan_data_report_handled.length);
    app_ble_core_global_handle(&event, NULL);
}

void app_ibrt_ui_disconnect_ble(void)
{
    app_ble_disconnect_all();
}

uint32_t app_ble_get_user_register(void)
{
    return bleModeEnv.adv_user_register;
}

void app_ble_get_runtime_adv_param(uint8_t *pAdvType, uint32_t *pAdvIntervalMs)
{
    *pAdvType = bleModeEnv.advParamInfo.advType;
    *pAdvIntervalMs = bleModeEnv.advParamInfo.advInterval;
}

void app_ble_set_white_list_complete(void)
{
    LOG_I("%s", __func__);

    app_env.need_set_white_list = false;
    app_env.setting_white_list = false;
    ble_execute_pending_op(BLE_ACTV_ACTION_SET_WHITE_LIST, 0xFF);
    if (app_env.isNeedToRestartScan)
    {
        app_env.isNeedToRestartScan = false;
        app_ble_start_scan(&app_env.ble_scan_param);
    }
    else
    {
        app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
    }
}

static void _app_ble_add_resloving_list(void)
{
    LOG_I("%s", __func__);
    uint8_t resloving_list_num = 0;
    ble_gap_ral_dev_info_t devicesInfoAddedToResolvingList[RESOLVING_LIST_MAX_NUM];

    bleModeEnv.pending_add_resloving_list = false;
    resloving_list_num = bleModeEnv.numberOfDevicesAddedToResolvingList;
    memcpy(&devicesInfoAddedToResolvingList[0], &bleModeEnv.devicesInfoAddedToResolvingList[0], sizeof(ble_gap_ral_dev_info_t)*resloving_list_num);

    ble_set_actv_action(BLE_ACTV_ACTION_ADD_RESLO_LIST, 0xFF);
    appm_set_rpa_list((ble_gap_ral_dev_info_t *)&devicesInfoAddedToResolvingList[0], resloving_list_num);
}

static void _app_ble_set_all_white_list(void)
{
    LOG_I("%s", __func__);
    uint8_t white_list_num = 0;
    uint8_t size = 0;
    ble_bdaddr_t bdaddr[8];

    bleModeEnv.pending_set_white_list = false;
    for (uint8_t i = 0; i < BLE_WHITE_LIST_USER_NUM; i++)
    {
        if (bleModeEnv.ble_white_list[i].enable)
        {
            size = bleModeEnv.ble_white_list[i].size;
            memcpy(&bdaddr[white_list_num], &bleModeEnv.ble_white_list[i].bdaddr[0], sizeof(ble_bdaddr_t)*size);
            white_list_num += size;
        }
    }

    ble_set_actv_action(BLE_ACTV_ACTION_SET_WHITE_LIST, 0xFF);
    appm_set_white_list((ble_bdaddr_t *)&bdaddr[0], white_list_num);
}

static void app_ble_set_all_white_list(void)
{
    TRACE(2, "%s ble_is_busy:%d, connection:%d", __func__,
          bleModeEnv.ble_is_busy, bleModeEnv.ble_is_connecting);
    if ((bleModeEnv.ble_is_busy) || (bleModeEnv.ble_is_connecting))
    {
        bleModeEnv.pending_set_white_list = true;
    }
    else
    {
        bleModeEnv.ble_is_busy = true;
        app_bt_start_custom_function_in_bt_thread(0, 0, (uint32_t)_app_ble_set_all_white_list);
    }
}

static void app_ble_add_all_resloving_list(void)
{
    TRACE(2, "%s ble_is_busy:%d, connection:%d", __func__,
          bleModeEnv.ble_is_busy, bleModeEnv.ble_is_connecting);
    if ((bleModeEnv.ble_is_busy) || (bleModeEnv.ble_is_connecting))
    {
        bleModeEnv.pending_add_resloving_list = true;
    }
    else
    {
        bleModeEnv.ble_is_busy = true;
        app_bt_start_custom_function_in_bt_thread(0, 0, (uint32_t)_app_ble_add_resloving_list);
    }
}

bool app_ble_is_white_list_enable(void)
{
    for (uint8_t i = 0; i < BLE_WHITE_LIST_USER_NUM; i++)
    {
        if (bleModeEnv.ble_white_list[i].enable)
        {
            return true;
        }
    }

    return false;
}

void app_ble_set_white_list(BLE_WHITE_LIST_USER_E user, ble_bdaddr_t *bdaddr, uint8_t size)
{
    LOG_I("%s user %d size %d", __func__, user, size);
    if (size > WHITE_LIST_MAX_NUM)
    {
        size = WHITE_LIST_MAX_NUM;
    }
    bleModeEnv.ble_white_list[user].enable = true;
    bleModeEnv.ble_white_list[user].size = size;

#ifdef CUSTOMER_DEFINE_ADV_DATA
    if (GAPM_STATIC_ADDR == bdaddr->addr_type)
    {
        bdaddr->addr_type = GAPM_GEN_RSLV_ADDR;
    }
#endif

    for (uint8_t index = 0; index < size; index++)
    {
        memcpy(&bleModeEnv.ble_white_list[user].bdaddr[index], &bdaddr[index], sizeof(ble_bdaddr_t));
    }

    app_ble_set_all_white_list();
}

uint8_t app_ble_add_all_paired_remote_dev_into_white_list(void)
{
    uint8_t addr_num = 0;
    ble_bdaddr_t addr_list[BLE_RECORD_NUM];

    addr_num = appm_get_all_paired_dev_addr_from_nv(addr_list);

    app_ble_set_white_list(BLE_WHITE_LIST_USER_MOBILE, addr_list, addr_num);

    return addr_num;
}

void app_ble_add_resloving_list(ble_gap_ral_dev_info_t *devicesInfoAddedToResolvingList, uint8_t numberOfDevicesAddedToResolvingList)
{
    TRACE(1, "%s", __func__);
    if (0 == numberOfDevicesAddedToResolvingList)
    {
        return;
    }

#if ((BLE_AUDIO_ENABLED == 0) && defined(IBRT))
    if (TWS_UI_SLAVE != app_ibrt_if_get_ui_role())
    {
        app_ble_sync_ble_info();
    }
#endif

    if (numberOfDevicesAddedToResolvingList > RESOLVING_LIST_MAX_NUM)
    {
        numberOfDevicesAddedToResolvingList = RESOLVING_LIST_MAX_NUM;
    }

    bleModeEnv.numberOfDevicesAddedToResolvingList = numberOfDevicesAddedToResolvingList;
    memcpy(&bleModeEnv.devicesInfoAddedToResolvingList[0], devicesInfoAddedToResolvingList, sizeof(ble_gap_ral_dev_info_t)*numberOfDevicesAddedToResolvingList);

    app_ble_add_all_resloving_list();
}

void app_ble_clear_white_list(BLE_WHITE_LIST_USER_E user)
{
    LOG_I("%s user %d", __func__, user);

    memset(&bleModeEnv.ble_white_list[user], 0, sizeof(BLE_WHITE_LIST_PARAM_T));

    app_ble_set_all_white_list();
}

void app_ble_clear_all_white_list(void)
{
    LOG_I("%s", __func__);

    for (uint8_t i = 0; i < BLE_WHITE_LIST_USER_NUM; i++)
    {
        memset(&bleModeEnv.ble_white_list[i], 0, sizeof(BLE_WHITE_LIST_PARAM_T));
    }

    app_ble_set_all_white_list();
}

void app_ble_refresh_irk(void)
{
    appm_refresh_ble_irk();
}

uint16_t app_ble_get_conhdl(uint8_t conidx)
{
    return gapc_get_conhdl(conidx);
}

uint8_t *app_ble_get_dev_name(void)
{
    return bleModeEnv.bleEnv->dev_name;
}

BLE_MODE_ENV_T *app_ble_get_mode_env(void)
{
    return &bleModeEnv;
}

uint8_t app_ble_get_user_activity_idx(BLE_ADV_ACTIVITY_USER_E actv_user)
{
    return bleModeEnv.bleEnv->adv_actv_idx[actv_user];
}

void app_ble_set_tx_rx_pref_phy(uint32_t tx_pref_phy, uint32_t rx_pref_phy)
{
    bleModeEnv.bleEnv->tx_pref_phy = tx_pref_phy;
    bleModeEnv.bleEnv->rx_pref_phy = rx_pref_phy;
    LOG_I("%s %04x %04x", __func__, bleModeEnv.bleEnv->tx_pref_phy,
          bleModeEnv.bleEnv->rx_pref_phy);
}

void ble_execute_pending_op(BLE_ACTV_ACTION_E finish_action, uint8_t actv_idx)
{
    LOG_I("%s finish_action %d", __func__, finish_action);
    uint8_t index_of_pending_disconnect = app_ble_index_pending_to_disconnect();
    if (false == ble_clear_actv_action(finish_action, actv_idx))
    {
        LOG_I("can't execute pending op");
        // BLE stack restricts connections to parallelism.
        // For the connection, need to check whether there is a pending connection,
        // because the bleModeEnv.ble_actv_action has been cleared when the activity started (BLE_ACTV_ACTION_CONNECTING),
        // so it will be executed here.
        if ((BLE_ACTV_ACTION_STOP_CONNECTING == finish_action) &&
                (bleModeEnv.ble_is_connecting))
        {
            bleModeEnv.ble_is_connecting = false;
            if (app_ble_pending_to_connect())
            {
                bleModeEnv.ble_is_connecting = true;
                app_bt_start_custom_function_in_bt_thread(0, 0, (uint32_t)ble_start_connect);
            }
        }

        return;
    }
    else
    {
        if ((BLE_ACTV_ACTION_CONNECTING == finish_action) &&
                (bleModeEnv.ble_is_connecting) && actv_idx == 0xFF)
        {
            bleModeEnv.ble_is_connecting = false;
        }
    }

    if (bleModeEnv.pending_stop_connecting)
    {
        bleModeEnv.pending_stop_connecting = false;
        app_bt_start_custom_function_in_bt_thread(0, 0,
                                                  (uint32_t)ble_stop_connecting);
    }
    else if (index_of_pending_disconnect != BLE_CONNECTION_MAX)
    {
        bleModeEnv.pendingDisconnect[index_of_pending_disconnect] = false;
        app_bt_start_custom_function_in_bt_thread((uint32_t)index_of_pending_disconnect,
                                                  0,
                                                  (uint32_t)ble_disconnect);
    }
    else if (bleModeEnv.pending_set_white_list)
    {
        app_bt_start_custom_function_in_bt_thread(0, 0,
                                                  (uint32_t)_app_ble_set_all_white_list);
    }
    else if (bleModeEnv.pending_add_resloving_list)
    {
        app_bt_start_custom_function_in_bt_thread(0, 0,
                                                  (uint32_t)_app_ble_add_resloving_list);
    }
    // BLE stack restricts connections to parallelism.
    else if (!bleModeEnv.ble_is_connecting && app_ble_pending_to_connect())
    {
        bleModeEnv.ble_is_connecting = true;
        app_bt_start_custom_function_in_bt_thread(0, 0,
                                                  (uint32_t)ble_start_connect);
    }
    else if (bleModeEnv.scan_has_pending_op)
    {
        bleModeEnv.scan_has_pending_op = false;
        app_bt_start_custom_function_in_bt_thread(0, 0,
                                                  (uint32_t)ble_start_scan);
    }
    else if (bleModeEnv.scan_has_pending_stop_op)
    {
        bleModeEnv.scan_has_pending_stop_op = false;
        app_bt_start_custom_function_in_bt_thread(0, 0,
                                                  (uint32_t)ble_stop_scanning);
    }
    else if (bleModeEnv.adv_has_pending_op)
    {
        bleModeEnv.adv_has_pending_op = false;
        app_bt_start_custom_function_in_bt_thread(0, 0,
                                                  (uint32_t)app_ble_start_adv);
    }
    else
    {
        bleModeEnv.ble_is_busy = false;
        app_ble_core_print_ble_state();
    }
}
