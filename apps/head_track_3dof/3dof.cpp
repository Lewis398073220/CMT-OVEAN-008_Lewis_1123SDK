/***************************************************************************
 *
 * Copyright 2015-2023 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 * Application layer function of head tracking 3dof with lsm6dsl sensor
 ****************************************************************************/
#ifdef HEAD_TRACK_ENABLE
#include "hal_trace.h"
#include "hal_i2c.h"
#include "hal_timer.h"
#include "hal_spi.h"
#include "string.h"
#include "lsm6dsl_reg.h"
#include "stdlib.h"

#ifdef RTOS
#include "cmsis_os.h"
#ifdef KERNEL_RTX
#include "rt_Time.h"
#endif
#endif

#include "3dof.h"
#include "head_track_3dof.h"
#include "window_node.h"

#if defined(__VIRTUAL_SURROUND__) || defined(__VIRTUAL_SURROUND___) || defined(__VIRTUAL_SURROUND_STEREO__)
#include "app_bt_stream.h"
extern "C" int32_t stereo_surround_set_yaw(float direction);
extern "C" int32_t stereo_surround_set_pitch(float direction);
#endif

#ifdef HEAD_TRACK_TOTA_TEST
#include "app_tota_cmd_code.h"
#include "app_tota.h"
static uint16_t resData[6]={0};
extern int app_bt_call_func_in_bt_thread(
    uint32_t param0, uint32_t param1, uint32_t param2, uint32_t param3, uint32_t funcPtr);
#endif

#define HEAD_TRACK_SIGNAL_ALGO          (0x01)
#define HEAD_TRACK_TIMER_INTERVAL       (1)
#define SLIDE_WINDOW_SIZE               (3)
#define OP_TOTA_HEAD_TRACK_POSE_CMD     (0x6900)


static int head_track_inited = 0;
static int head_track_algo_inited = 0;
static int head_track_send_inited = 0;
static bool execute_track = true;
static IMU_DATA_FETCHER_FUNC imu_data_fetcher = NULL;

static osMutexId head_track_sensor_mutex_id = NULL;
osMutexDef(head_track_sensor_mutex);

static osMutexId head_track_pose_mutex_id = NULL;
osMutexDef(head_track_pose_mutex);

static osSemaphoreId sensor_signal = NULL;
osSemaphoreDef(sensor_signal);

static struct WindowNode* head = NULL;

void head_track_window_link_init(void)
{
    for (int i = 0; i < SLIDE_WINDOW_SIZE; i++) {
        insertNode(&head, 0.0);
    }
}

void head_track_window_link_release(void)
{
    struct WindowNode* current = head;
    while (current != NULL) {
        WindowNode* temp = current;
        current = current->next;
        free(temp);
    }
}

void head_track_resouce_init(void)
{
    if(head_track_sensor_mutex_id == NULL) {
        head_track_sensor_mutex_id = osMutexCreate(osMutex(head_track_sensor_mutex));
        if (head_track_sensor_mutex_id == NULL) {
            TRACE(1, "Failed to Create head_track_sensor_mutex_id");
        }
    }

    if (head_track_pose_mutex_id == NULL) {
        head_track_pose_mutex_id = osMutexCreate(osMutex(head_track_pose_mutex));
        if (head_track_pose_mutex_id == NULL) {
            TRACE(1, "Failed to Create head_track_pose_mutex_id");
        }
    }

    if (sensor_signal == NULL) {
        sensor_signal = osSemaphoreCreate(osSemaphore(sensor_signal), 1);
        if (sensor_signal == NULL) {
            TRACE(1, "Failed to Create sensor_signal");
        }
    }

    if(head == NULL) {
        head_track_window_link_init();
        if(head == NULL) {
            TRACE(1, "Failed to Create head track window LinkList");
        }
    }else{
        /* release, then init again */
        head_track_window_link_release();
        head_track_window_link_init();
        if(head == NULL) {
            TRACE(1, "Failed to Create head track window LinkList");
        }
    }

}

void adjustStep(uint8_t* s, struct SENSOR_IMU* sensor, float* cur, float* pre)
{
    float windowAvg;
    *cur = (float)sqrt(pow(sensor->g_x,2.0) +  pow(sensor->g_y, 2.0) + pow(sensor->g_z, 2.0));
    writeDataToLinkedList(&head, abs(*cur - *pre));
    *pre = *cur;
    windowAvg = avgList(head);
    if(windowAvg > 2.5){
        /* freq needs to be arised to 1k Hz */
        *s = 1;
    }else{
        /* lower freq is enough, e.g. 100Hz */
        *s = 10;
    }
}

#define HEAD_TRACK_THREAD_STACK_SIZE    2048
static void head_track_task_thread(void const* argument);
static osThreadId head_track_thread_id;
static osThreadDef(head_track_task_thread, osPriorityNormal, 1, HEAD_TRACK_THREAD_STACK_SIZE, "head_track_thread");

#define POSE_SIZE (sizeof(struct POSE_S))
#define SENSOR_SIZE (sizeof(struct SENSOR_IMU))

static struct POSE_S pose_s;
static struct SENSOR_IMU sensor;
static uint8_t step;
static void head_track_task_thread(void const*argument)
{
    osEvent evt;
    uint32_t signals = 0;
    struct SENSOR_IMU thread_sensor;

    uint8_t stepInteval = 0;
    float preAngleVel = 0.0;
    float curAngleVel = 0.0;

    while (1) {
        evt = osSignalWait(0x0, osWaitForever);
        signals = evt.value.signals;

        if (evt.status == osEventSignal) {
            if (signals & HEAD_TRACK_SIGNAL_ALGO & execute_track) {
                /* get current imu sensor data */
                thread_sensor = imu_data_fetcher();

                adjustStep(&step, &thread_sensor, &curAngleVel, &preAngleVel);

                if(stepInteval % step == 0) {
                    osMutexWait(head_track_sensor_mutex_id, osWaitForever);
                    memcpy(&sensor, &thread_sensor, SENSOR_SIZE);
                    osSemaphoreRelease(sensor_signal);
                    osMutexRelease(head_track_sensor_mutex_id);
                    stepInteval = 0;
                }
                stepInteval++;

            }
        }
    }
}

#define HEAD_TRACK_ALGO_THREAD_STACK_SIZE    4096
static void head_track_algo_task_thread(void const* argument);
static osThreadId head_track_algo_thread_id;
static osThreadDef(head_track_algo_task_thread, osPriorityNormal, 1, HEAD_TRACK_ALGO_THREAD_STACK_SIZE, "head_track_algo_thread");

static void head_track_algo_task_thread(void const*argument)
{
    struct POSE_S algoPose;
    struct SENSOR_IMU localSensor;

    uint32_t algoCount=0;

    while (1)
    {
        osSemaphoreWait(sensor_signal, osWaitForever);
        osMutexWait(head_track_sensor_mutex_id, osWaitForever);
        memcpy(&localSensor, &sensor, SENSOR_SIZE);
        sensor.rflag = 0;
        osMutexRelease(head_track_sensor_mutex_id);

        if(localSensor.rflag == 1) {
            // algoPose = algo_head_track(&localSensor, &adjParam);
            head_track_3dof_algo_with_fix_param(&algoPose, &localSensor);
        }

        algoCount++;

        if((algoCount * step) % 10 == 0){
            osMutexWait(head_track_pose_mutex_id, osWaitForever);
            memcpy(&pose_s, &algoPose, POSE_SIZE);
            osMutexRelease(head_track_pose_mutex_id);
        }
    }
}

#define HEAD_TRACK_SEND_THREAD_STACK_SIZE    1024
static void head_track_send_task_thread(void const* argument);
static osThreadId head_track_send_thread_id;
static osThreadDef(head_track_send_task_thread, osPriorityNormal, 1, HEAD_TRACK_SEND_THREAD_STACK_SIZE, "head_track_send_thread");

static void head_track_send_task_thread(void const*argument)
{
    struct POSE_S send_pose;
    while (1)
    {
        osMutexWait(head_track_pose_mutex_id, osWaitForever);
        memcpy(&send_pose, &pose_s, POSE_SIZE);
        osMutexRelease(head_track_pose_mutex_id);
        /*
            set spatial audio with 3dof angle if necessary,
            the spatial audio APIs might be different,
            here is just a demo usage.
        */
#if defined(__VIRTUAL_SURROUND__) || defined(__VIRTUAL_SURROUND___) || defined(__VIRTUAL_SURROUND_STEREO__)
        stereo_surround_set_yaw((float)pose_s.yaw);
        stereo_surround_set_pitch((float)pose_s.pitch);
#endif

#ifdef HEAD_TRACK_TOTA_TEST
    /*
     * pose data send to phone app demo via tota_v2 protocal
     * make sure  TOTA_ENCODE = 0
     */
    resData[0] = (int)(pose_s.yaw * (float)57.3); // rad to deg
    resData[1] = (int)(pose_s.pitch * (float)57.3);
    resData[2] = (int)(pose_s.roll * (float)57.3);
    resData[3] = (int)(pose_s.vx);
    resData[4] = (int)(pose_s.vy);
    resData[5] = (int)(pose_s.vz);
    app_bt_call_func_in_bt_thread(OP_TOTA_HEAD_TRACK_POSE_CMD,
                                    TOTA_NO_ERROR,
                                    (uint32_t)resData,
                                    6 * sizeof(uint16_t),
                                    (uint32_t)app_tota_send_rsp);
#endif
        osDelay(10);
    }
}


static int head_track_task_init(void)
{
    TRACE(0, "%s(true),%d \n\r", __FUNCTION__, __LINE__);

    if (!head_track_inited) {
        head_track_thread_id = osThreadCreate(osThread(head_track_task_thread), NULL);
        if (head_track_thread_id == NULL) {
            TRACE(0, "create head_track_thread failed");
            return -1;
        }
        head_track_inited = 1;
    }
    return 0;
}

static int head_track_algo_task_init(void)
{
    TRACE(0, "%s(true),%d \n\r", __FUNCTION__, __LINE__);

    if(!head_track_algo_inited)
    {
        head_track_algo_thread_id = osThreadCreate(osThread(head_track_algo_task_thread), NULL);
        if (head_track_algo_thread_id == NULL) {
            TRACE(0, "create head_track_algo_thread failed");
            return -1;
        }

        head_track_algo_inited = 1;
    }
    return 0;
}

static int head_track_send_task_init(void)
{
    TRACE(0, "%s(true),%d \n\r", __FUNCTION__, __LINE__);

    if(!head_track_send_inited)
    {
        head_track_send_thread_id = osThreadCreate(osThread(head_track_send_task_thread), NULL);
        if (head_track_send_thread_id == NULL) {
            TRACE(0, "create head_track_send_thread failed");
            return -1;
        }

        head_track_send_inited = 1;
    }
    return 0;
}

void imu_timer_handler(void const *param);
osTimerDef (imu_timer, imu_timer_handler);
osTimerId imu_timer_id = NULL;

void imu_timer_handler(void const *param)
{
    osSignalSet(head_track_thread_id, HEAD_TRACK_SIGNAL_ALGO);
}

void head_angle_reset(void)
{
    head_track_3dof_reset();
}

void imusensor_init(IMU_DATA_FETCHER_FUNC imu_data_fetcher_func)
{
    imu_data_fetcher = imu_data_fetcher_func;

    /* Init head track task  */
    head_track_resouce_init();

    /* Init head track task  */
    head_track_task_init();
    head_track_algo_task_init();
    head_track_send_task_init();

    /* Create imu timer*/
    imu_timer_id = osTimerCreate(osTimer(imu_timer), osTimerPeriodic, NULL);

    /* start head track timer to release signals */
    if (imu_timer_id != NULL) {
        osTimerStart(imu_timer_id, HEAD_TRACK_TIMER_INTERVAL);
    }
    return;
}

void imusensor_reset(void)
{
    head_track_3dof_reset();
}

void head_track_algo_start(void)
{
    execute_track = true;
}

void head_track_algo_stop(void)
{
    execute_track = false;
}

#endif