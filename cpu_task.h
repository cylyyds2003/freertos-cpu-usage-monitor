/**
  ****************************(C) COPYRIGHT 2026 C10****************************
  * @file       cpu_task.h
  * @brief      CPU占用率统计任务
  *             基于FreeRTOS Run Time Stats统计总CPU占用率与任务占用率
  * @note       1. 依赖configGENERATE_RUN_TIME_STATS等宏配置开启
  *             2. 运行时计数器使用DWT->CYCCNT
  *             3. 对外提供按任务句柄/任务名查询占用率接口
  * @history
  *  Version    Date            Author          Modification
  *  V1.0.0     2026-04-23      C10             1.创建模块并预留基础接口
  *  V1.1.0     2026-04-23      C10             1.新增总CPU占用率差分统计
  *                                             2.新增按任务占用率统计与快照接口
  *                                             3.新增DWT运行时间计数器初始化与读取
  @verbatim
  ==============================================================================
  * 主要接口:
  * 1. cpu_task(): 周期统计总占用率与任务占用率
  * 2. cpu_task_get_usage(): 返回总CPU占用率(%)
  * 3. cpu_task_get_task_usage_x100(): 按句柄返回任务占用率(x100)
  * 4. cpu_task_get_task_usage_x100_by_name(): 按任务名返回任务占用率(x100)
  * 5. cpu_task_get_task_usage_list(): 返回任务占用率快照
  @endverbatim
  ****************************(C) COPYRIGHT 2026 C10****************************
  */


#ifndef CPU_TASK_H
#define CPU_TASK_H

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

#ifndef configUSE_TRACE_FACILITY
#error "cpu_task requires configUSE_TRACE_FACILITY to be defined and set to 1."
#elif (configUSE_TRACE_FACILITY != 1)
#error "cpu_task requires configUSE_TRACE_FACILITY == 1."
#endif

#ifndef configGENERATE_RUN_TIME_STATS
#error "cpu_task requires configGENERATE_RUN_TIME_STATS to be defined and set to 1."
#elif (configGENERATE_RUN_TIME_STATS != 1)
#error "cpu_task requires configGENERATE_RUN_TIME_STATS == 1."
#endif

#ifndef configUSE_STATS_FORMATTING_FUNCTIONS
#error "cpu_task requires configUSE_STATS_FORMATTING_FUNCTIONS to be defined and set to 1."
#elif (configUSE_STATS_FORMATTING_FUNCTIONS != 1)
#error "cpu_task requires configUSE_STATS_FORMATTING_FUNCTIONS == 1."
#endif

#ifndef INCLUDE_xTaskGetIdleTaskHandle
#error "cpu_task requires INCLUDE_xTaskGetIdleTaskHandle to be defined and set to 1."
#elif (INCLUDE_xTaskGetIdleTaskHandle != 1)
#error "cpu_task requires INCLUDE_xTaskGetIdleTaskHandle == 1."
#endif

#ifndef portCONFIGURE_TIMER_FOR_RUN_TIME_STATS
#error "cpu_task requires portCONFIGURE_TIMER_FOR_RUN_TIME_STATS to be defined in FreeRTOSConfig.h."
#endif

#ifndef portGET_RUN_TIME_COUNTER_VALUE
#error "cpu_task requires portGET_RUN_TIME_COUNTER_VALUE to be defined in FreeRTOSConfig.h."
#endif

#define CPU_TASK_PERIOD_MS            1000U
#define CPU_TASK_MAX_TRACKED_TASKS    16U

typedef struct
{
	TaskHandle_t handle;
	char name[configMAX_TASK_NAME_LEN];
	uint32_t usage_x100;
} cpu_task_usage_item_t;

void cpu_task(void const *argument);

float cpu_task_get_usage(void);
uint32_t cpu_task_get_usage_x100(void);
UBaseType_t cpu_task_get_task_usage_list(cpu_task_usage_item_t *list, UBaseType_t max_items);
uint32_t cpu_task_get_task_usage_x100(TaskHandle_t handle);
uint32_t cpu_task_get_task_usage_x100_by_name(const char *task_name);

void cpu_task_runtime_counter_init(void);
uint32_t cpu_task_runtime_counter_get(void);

#endif
