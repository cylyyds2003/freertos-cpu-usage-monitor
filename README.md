# FreeRTOS CPU Usage Task

一个轻量的 FreeRTOS CPU 占用率统计模块，支持：

- 系统总 CPU 占用率统计（差分法）
- 按任务句柄查询占用率
- 按任务名查询占用率
- 获取当前任务占用率快照列表
- 基于 DWT->CYCCNT 的运行时计数器实现

当前仓库包含两个核心文件：

- [cpu_task.c](cpu_task.c)
- [cpu_task.h](cpu_task.h)

## 1. 特性

- 低侵入：以独立任务周期采样，不改动业务任务逻辑
- 实时性好：采用相邻采样点差分，反映区间内真实占用变化
- 查询方便：支持句柄和任务名两种方式读取任务占用率
- 接口清晰：总占用率可直接取浮点百分比或 x100 定点值

## 2. 原理简介

在每个采样周期：

1. 调用 uxTaskGetSystemState 获取系统总运行计数与各任务累计运行计数。
2. 找到 Idle 任务累计运行计数。
3. 计算本周期增量：
   - delta_total = total_now - total_last
   - delta_idle = idle_now - idle_last
4. 系统总占用率：
   - cpu_usage = (delta_total - delta_idle) / delta_total
5. 各任务占用率：
   - task_usage = delta_task / delta_total

模块内部统一以 x100 保存百分比：

- 2534 表示 25.34%

## 3. 依赖与前置条件

需要使用 FreeRTOS，并正确开启运行时统计相关配置。

在 FreeRTOSConfig.h 中确保以下宏有效：

- configUSE_TRACE_FACILITY = 1
- configGENERATE_RUN_TIME_STATS = 1
- configUSE_STATS_FORMATTING_FUNCTIONS = 1
- INCLUDE_xTaskGetIdleTaskHandle = 1
- 定义 portCONFIGURE_TIMER_FOR_RUN_TIME_STATS
- 定义 portGET_RUN_TIME_COUNTER_VALUE

说明：本模块示例通过 DWT 周期计数器提供运行时计数值。

## 4. 快速接入

1. 将 [cpu_task.c](cpu_task.c) 和 [cpu_task.h](cpu_task.h) 加入工程。
2. 在系统启动阶段调用 cpu_task_runtime_counter_init() 初始化计数器。
3. 创建统计任务 cpu_task（建议周期 1000ms，可按需调整 CPU_TASK_PERIOD_MS）。
4. 在业务代码中调用查询接口读取占用率。

示例（伪代码）：

    cpu_task_runtime_counter_init();
    xTaskCreate(cpu_task, "cpu", stack, NULL, prio, NULL);

## 5. 对外接口

### 5.1 任务函数

- void cpu_task(void const *argument)
  - 周期采样并更新总占用率与任务占用率快照

### 5.2 总占用率

- float cpu_task_get_usage(void)
  - 返回浮点百分比，例如 25.34

- uint32_t cpu_task_get_usage_x100(void)
  - 返回 x100 定点值，例如 2534

### 5.3 任务占用率

- UBaseType_t cpu_task_get_task_usage_list(cpu_task_usage_item_t *list, UBaseType_t max_items)
  - 复制当前任务占用率快照到 list，返回实际拷贝数量

- uint32_t cpu_task_get_task_usage_x100(TaskHandle_t handle)
  - 按任务句柄查询 x100 占用率

- uint32_t cpu_task_get_task_usage_x100_by_name(const char *task_name)
  - 按任务名查询 x100 占用率

## 6. 参数建议

- CPU_TASK_PERIOD_MS：默认 1000ms
  - 更短周期：数据更灵敏，开销略增
  - 更长周期：数据更平滑，开销更低

- CPU_TASK_MAX_TRACKED_TASKS：默认 16
  - 需大于系统可能并发任务数，否则超出部分无法纳入快照

## 7. 已知限制

- 统计结果依赖运行时计数器精度与时钟配置。
- 任务频繁创建/删除时，单次差分中个别任务可能出现短时跳变。
- 当前实现面向单核 FreeRTOS 场景。

## 8. 版本信息

- V1.1.0
  - 新增总 CPU 占用率差分统计
  - 新增按任务占用率统计与快照接口
  - 新增 DWT 运行时间计数器初始化与读取

## 9. License

本项目基于 [MIT License](LICENSE) 开源。
