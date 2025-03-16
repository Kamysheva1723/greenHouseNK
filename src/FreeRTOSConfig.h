/*
 * FreeRTOS V202111.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * Adjust these definitions for the specific hardware and application
 * requirements of the Greenhouse Fertilization System.
 * This project features a modular, FreeRTOS based design with tasks for sensor
 * data handling, UI updates, EEPROM storage, and cloud communications.
 *
 * See http://www.freertos.org/a00110.html for further details.
 *----------------------------------------------------------*/

/* Scheduler Related */
/* Enable preemptive scheduling to ensure high-priority tasks such as sensor and UI updates can interrupt lower priority tasks */
#define configUSE_PREEMPTION                    1

/* Disable tickless idle as we require a consistent 1 ms tick for precise timing,
   important for timed operations like valve control via one-shot timers */
#define configUSE_TICKLESS_IDLE                 0

/* Disable idle hook function since no extra processing is needed during idle time */
#define configUSE_IDLE_HOOK                     0

/* Disable tick hook function to keep ISR routines minimal */
#define configUSE_TICK_HOOK                     0

/* Set the tick rate to 1000Hz to provide 1 ms time resolution for accurate timing tasks 
   like sensor sampling and control loops. */
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 )

/* Define up to 32 task priority levels for fine-grained scheduling, critical for real-time operations */
#define configMAX_PRIORITIES                    32

/* Set minimal stack size for tasks; tasks must allocate a sufficient stack for processing sensor, UI, or network communications */
#define configMINIMAL_STACK_SIZE                ( configSTACK_DEPTH_TYPE ) 256

/* Use 32-bit ticks since the system requires fine resolution timing */
#define configUSE_16_BIT_TICKS                  0

/* Yield the idle task when other tasks of equal priority are ready, improving responsiveness */
#define configIDLE_SHOULD_YIELD                 1

/* No passive idle hook used in this application configuration */
#define configUSE_PASSIVE_IDLE_HOOK             0


/* Synchronization Related */
/* Enable mutexes to protect shared resources (e.g., sensor data, EEPROM storage, and UI updates) */
#define configUSE_MUTEXES                       1

/* Enable recursive mutexes for tasks that require multiple locks on the same resource */
#define configUSE_RECURSIVE_MUTEXES             1

/* Not using task tagging functionality */
#define configUSE_APPLICATION_TASK_TAG          0

/* Enable counting semaphores for synchronizing events between tasks, such as sensor data availability */
#define configUSE_COUNTING_SEMAPHORES           1

/* Enable queue registry to facilitate debugging of the inter-task communication queues */
#define configQUEUE_REGISTRY_SIZE               8

/* Enable support for queue sets which allow multiple queues to be managed together */
#define configUSE_QUEUE_SETS                    1

/* Enable time slicing to allow tasks of equal priority to share the CPU fairly */
#define configUSE_TIME_SLICING                  1

/* Enable reentrancy for newlib usage, important for multi-tasking scenarios */
#define configUSE_NEWLIB_REENTRANT              1

/* Defines compatibility for lwIP FreeRTOS sys_arch */
#define configENABLE_BACKWARD_COMPATIBILITY     1

/* Reserve 5 thread local storage pointers for tasks, useful for storing task specific data */
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 5


/* System */
/* Define the data type for stack depth; using uint32_t ensures compatibility with the RP2040 architecture */
#define configSTACK_DEPTH_TYPE                  uint32_t

/* Define type for message buffer lengths */
#define configMESSAGE_BUFFER_LENGTH_TYPE        size_t


/* Memory allocation related definitions. */
/* No static allocation is supported; tasks and objects are created dynamically. This allows flexible resource allocation at runtime */
#define configSUPPORT_STATIC_ALLOCATION         0

/* Enable dynamic memory allocation */
#define configSUPPORT_DYNAMIC_ALLOCATION        1

/* Set the total heap size to 64KB, providing plenty of RAM for dynamic object creation (tasks, queues, timers, etc.) */
#define configTOTAL_HEAP_SIZE                   (64*1024)

/* Do not use application allocated heap */
#define configAPPLICATION_ALLOCATED_HEAP        0


/* Hook function related definitions. */
/* Do not check for stack overflow at runtime in this configuration */
#define configCHECK_FOR_STACK_OVERFLOW          0

/* No malloc failed hook is provided */
#define configUSE_MALLOC_FAILED_HOOK            0

/* No daemon task startup hook is defined */
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0


/* Run time and task stats gathering related definitions. */
/* Enable generation of run time statistics for profiling system performance (e.g., task execution times) */
#define configGENERATE_RUN_TIME_STATS           1

/* Enable trace facility for better debugging and insight into task behavior */
#define configUSE_TRACE_FACILITY                1

/* Enable functions to format run time statistics for display or logging */
#define configUSE_STATS_FORMATTING_FUNCTIONS    1

/* Record the highest address of the stack to assist in debugging and stack usage optimisation */
#define configRECORD_STACK_HIGH_ADDRESS         1

#ifdef __cplusplus
extern "C" {
#endif

/* Prototype for a function to read the runtime counter, used by the run time stats gathering */
uint32_t read_runtime_ctr(void);

#ifdef __cplusplus
}
#endif

/* Macro definitions to interface with platform-specific timer for runtime statistics*/
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()  {}
#define portGET_RUN_TIME_COUNTER_VALUE() read_runtime_ctr()
//#define configRESET_STACK_POINTER 0


/* Co-routine related definitions. */
/* Co-routines are not used in this application */
#define configUSE_CO_ROUTINES                   0
#define configMAX_CO_ROUTINE_PRIORITIES         1


/* Software timer related definitions. */
/* Enable software timers that are essential for tasks like automatically closing the CO₂ valve after a fixed time */
#define configUSE_TIMERS                        1

/* Assign the highest priority (one less than the max) to the timer task to ensure timely callback execution */
#define configTIMER_TASK_PRIORITY               ( configMAX_PRIORITIES - 1 )

/* Set the timer queue length to accommodate multiple timer requests (eg, for sensor control and actuator timing) */
#define configTIMER_QUEUE_LENGTH                10

/* Define the stack depth for the timer task; a depth of 1024 ensures sufficient space for timer callback routines */
#define configTIMER_TASK_STACK_DEPTH            1024


/* Interrupt nesting behaviour configuration. */
/* Processor specific values must be defined based on the target hardware, omitted here for brevity. */
/*
#define configKERNEL_INTERRUPT_PRIORITY         [dependent of processor]
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    [dependent on processor and application]
#define configMAX_API_CALL_INTERRUPT_PRIORITY   [dependent on processor and application]
*/


#if FREE_RTOS_KERNEL_SMP // set by the RP2040 SMP port of FreeRTOS
    /* SMP port only: For RP2040, define the number of cores used.
       Here, configuration is for a single core operation as per this project's requirements. */
    #define configNUMBER_OF_CORES                   1

    /* Ticking is performed on core 0 */
    #define configTICK_CORE                         0

    /* Allow multiple priorities to run concurrently in an SMP environment */
    #define configRUN_MULTIPLE_PRIORITIES           1

    /* Disable explicit core affinity for tasks */
    #define configUSE_CORE_AFFINITY                 0
#endif


/* RP2040 specific configuration */
/* Enable Pico specific interop for synchronizing with the Pico SDK APIs (e.g., for I²C or Wi-Fi) */
#define configSUPPORT_PICO_SYNC_INTEROP         1

/* Enable Pico specific time interoperability */
#define configSUPPORT_PICO_TIME_INTEROP         1

#include <assert.h>
/* Define assertion macro to trap errors during development */
#define configASSERT(x)                         assert(x)


/* Set the following definitions to 1 to include the API function, or zero
   to exclude the API function. Here, most API functions are included to provide
   a full set of features for mission-critical modules such as the Controller, UI, and Cloud tasks. */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTaskGetIdleTaskHandle          1
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_xTaskAbortDelay                 1
#define INCLUDE_xTaskGetHandle                  1
#define INCLUDE_xTaskResumeFromISR              1
#define INCLUDE_xQueueGetMutexHolder            1

/* Optionally, include a header for trace macros to assist in runtime debugging and profiling */

#endif /* FREERTOS_CONFIG_H */