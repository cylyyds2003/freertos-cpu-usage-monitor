/**
  ****************************(C) COPYRIGHT 2026 C10****************************
  * @file       cpu_task.c
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


#include "cpu_task.h"
#include "main.h"
#include <string.h>

// 总CPU占用率，放大100倍保存 例如2534表示25.34%
static volatile uint32_t cpu_usage_x100 = 0U;
// 每个任务的占用率快照表
static cpu_task_usage_item_t cpu_task_usage_table[CPU_TASK_MAX_TRACKED_TASKS];
static UBaseType_t cpu_task_usage_count = 0U;

static uint32_t cpu_task_get_prev_counter(const TaskStatus_t *prev_status,
                                          UBaseType_t prev_count,
                                          TaskHandle_t handle,
                                          uint8_t *found);

void cpu_task(void const *argument)
{
  TaskHandle_t idle_task_handle = xTaskGetIdleTaskHandle();
  TaskStatus_t task_status[CPU_TASK_MAX_TRACKED_TASKS];
  TaskStatus_t prev_task_status[CPU_TASK_MAX_TRACKED_TASKS];
  uint32_t total_run_time = 0U;
  uint32_t last_total_run_time = 0U;
  uint32_t idle_run_time = 0U;
  uint32_t last_idle_run_time = 0U;
  UBaseType_t prev_task_count = 0U;

  while (1)
  {
    // 读取当前所有任务状态和累计运行时间
    UBaseType_t task_count = uxTaskGetSystemState(task_status,
                                                  (UBaseType_t)CPU_TASK_MAX_TRACKED_TASKS,
                                                  &total_run_time);

    idle_run_time = 0U;
    for (UBaseType_t i = 0; i < task_count; i++)
    {
      if (task_status[i].xHandle == idle_task_handle)
      {
        idle_run_time = task_status[i].ulRunTimeCounter;
        break;
      }
    }

    {
      uint32_t delta_total = total_run_time - last_total_run_time;
      uint32_t delta_idle = idle_run_time - last_idle_run_time;

      if ((delta_total > 0U) && (delta_idle <= delta_total))
      {
        // 总占用率 = (总增量-空闲增量)/总增量
        uint64_t busy = (uint64_t)(delta_total - delta_idle) * 10000ULL;
        cpu_usage_x100 = (uint32_t)(busy / delta_total);

        taskENTER_CRITICAL();
        cpu_task_usage_count = task_count;
        for (UBaseType_t i = 0; i < task_count; i++)
        {
          uint8_t found = 0U;
          uint32_t prev_counter = cpu_task_get_prev_counter(prev_task_status,
                                                            prev_task_count,
                                                            task_status[i].xHandle,
                                                            &found);
          uint32_t delta_task = 0U;

          if ((found == 1U) && (task_status[i].ulRunTimeCounter >= prev_counter))
          {
            // 单任务占用率使用相邻两次采样差分计算
            delta_task = task_status[i].ulRunTimeCounter - prev_counter;
          }

          cpu_task_usage_table[i].handle = task_status[i].xHandle;
          if (task_status[i].pcTaskName != NULL)
          {
            (void)strncpy(cpu_task_usage_table[i].name,
                          (const char *)task_status[i].pcTaskName,
                          configMAX_TASK_NAME_LEN - 1U);
            cpu_task_usage_table[i].name[configMAX_TASK_NAME_LEN - 1U] = '\0';
          }
          else
          {
            cpu_task_usage_table[i].name[0] = '\0';
          }

          cpu_task_usage_table[i].usage_x100 = (uint32_t)(((uint64_t)delta_task * 10000ULL) / delta_total);
        }
        taskEXIT_CRITICAL();
      }
    }

    // 保存本次采样结果，供下一个周期做差分
    last_total_run_time = total_run_time;
    last_idle_run_time = idle_run_time;
    if (task_count <= (UBaseType_t)CPU_TASK_MAX_TRACKED_TASKS)
    {
      (void)memcpy(prev_task_status, task_status, sizeof(TaskStatus_t) * task_count);
      prev_task_count = task_count;
    }

    vTaskDelay(pdMS_TO_TICKS(CPU_TASK_PERIOD_MS));
  }
}

// 在上一次采样结果中按任务句柄查找累计运行计数
static uint32_t cpu_task_get_prev_counter(const TaskStatus_t *prev_status,
                                          UBaseType_t prev_count,
                                          TaskHandle_t handle,
                                          uint8_t *found)
{
  for (UBaseType_t i = 0; i < prev_count; i++)
  {
    if (prev_status[i].xHandle == handle)
    {
      *found = 1U;
      return prev_status[i].ulRunTimeCounter;
    }
  }

  *found = 0U;
  return 0U;
}

void cpu_task_runtime_counter_init(void)
{
  // 先开Trace总开关，再启用DWT周期计数器
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0U;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

uint32_t cpu_task_runtime_counter_get(void)
{
  return DWT->CYCCNT;
}

float cpu_task_get_usage(void)
{
  // 把x100格式转成百分比浮点数
  return ((float)cpu_usage_x100) * 0.01f;
}

uint32_t cpu_task_get_usage_x100(void)
{
  return cpu_usage_x100;
}

UBaseType_t cpu_task_get_task_usage_list(cpu_task_usage_item_t *list, UBaseType_t max_items)
{
  UBaseType_t copy_count;

  // 在临界区内复制快照，避免与统计任务并发写冲突
  taskENTER_CRITICAL();
  copy_count = (cpu_task_usage_count < max_items) ? cpu_task_usage_count : max_items;
  if ((list != NULL) && (copy_count > 0U))
  {
    (void)memcpy(list, cpu_task_usage_table, sizeof(cpu_task_usage_item_t) * copy_count);
  }
  taskEXIT_CRITICAL();

  return copy_count;
}

uint32_t cpu_task_get_task_usage_x100(TaskHandle_t handle)
{
  uint32_t usage = 0U;

  // 按任务句柄查询占用率
  taskENTER_CRITICAL();
  for (UBaseType_t i = 0; i < cpu_task_usage_count; i++)
  {
    if (cpu_task_usage_table[i].handle == handle)
    {
      usage = cpu_task_usage_table[i].usage_x100;
      break;
    }
  }
  taskEXIT_CRITICAL();

  return usage;
}

uint32_t cpu_task_get_task_usage_x100_by_name(const char *task_name)
{
  uint32_t usage = 0U;

  if (task_name == NULL)
  {
    return 0U;
  }

  // 按任务名查询占用率
  taskENTER_CRITICAL();
  for (UBaseType_t i = 0; i < cpu_task_usage_count; i++)
  {
    if (strncmp(cpu_task_usage_table[i].name, task_name, configMAX_TASK_NAME_LEN) == 0)
    {
      usage = cpu_task_usage_table[i].usage_x100;
      break;
    }
  }
  taskEXIT_CRITICAL();

  return usage;
}

