/*
 * 文件名: main.c
 * 描述: 状态机事件通知加参数的示例
 * 平台: STM32F10x
 * 功能: 展示了状态机与消息队列结合使用，并支持事件参数传递的实现
 * 特点: 在状态机配合消息队列的基础上，增加了事件参数传递功能
 * 优点: 1. 事件产生和处理解耦，提高系统灵活性
 *       2. 支持事件缓存，避免事件丢失
 *       3. 支持事件参数传递，增强事件表达能力
 *       4. 便于实现多任务环境下的状态机
 *       5. 简化状态机的事件处理流程
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stm32f10x.h" // Device header
#include "Serial.h"

#include "fsm.h"
#include "queue.h"

/* 用户自定义事件枚举，从EVENT_USER开始定义 */
enum
{
    EVENT_GOTO_STATE1 = EVENT_USER, // 切换到状态1事件
    EVENT_GOTO_STATE2,              // 切换到状态2事件
    EVENT_GOTO_STATE3,              // 切换到状态3事件
    EVENT_RUN_CURRENT,              // 执行当前状态事件
    EVENT_RUN_STATE1_ONLY,          // 仅限状态1执行事件
    EVENT_RUN_STATE2_ONLY,          // 仅限状态2执行事件
    EVENT_RUN_STATE3_ONLY,          // 仅限状态3执行事件
};

/* 事件结构体定义，继承自基础事件类型并添加自定义参数 */
struct fsm_event
{
    event_t super;         /* 基础事件类型，包含事件类型等信息 */
    unsigned short value;   /* 自定义参数，用于传递额外数据 */
};

/* 全局状态机实例 */
static fsm_t g_fsm;
/* 全局事件队列，用于缓存事件 */
static queue_handle_t g_event_queue;

/* 状态处理函数声明 */
static status_t handle_state_init(fsm_t *self, event_t *event);
static status_t handle_state1(fsm_t *self, event_t *event);
static status_t handle_state2(fsm_t *self, event_t *event);
static status_t handle_state3(fsm_t *self, event_t *event);

/**
 * @brief 处理队列中的事件
 * 
 * 该函数从事件队列中取出所有事件，并逐一分发给状态机处理。
 * 这是状态机与消息队列结合使用的核心函数，实现了事件的生产和消费解耦。
 * 
 * 处理流程：
 * 1. 检查事件队列是否为空
 * 2. 如果不为空，取出队列头部的事件
 * 3. 将事件分发给状态机处理
 * 4. 重复上述过程直到队列为空
 * 
 * 注意事项：
 * - 该函数应该在主循环或适当的地方被调用
 * - 事件处理顺序与入队顺序一致(FIFO)
 * - 如果队列满，新事件将被丢弃
 * - 事件参数通过事件结构体传递
 */
static void process_events(void)
{
    /* 定义事件结构体变量，用于存储从队列中取出的事件 */
    struct fsm_event event;

    /* 处理队列中的所有事件，直到队列为空 */
    while (!queue_is_empty(g_event_queue))
    {
        /* 从队列中取出事件 */
        if (queue_receive(g_event_queue, &event))
        {
            /* 将事件分发给状态机处理，传递事件结构体指针 */
            fsm_dispatch(&g_fsm, (event_t *)&event);
        }
    }
}

/**
 * @brief 添加事件到队列
 * 
 * 该函数将事件添加到事件队列中，供后续处理。
 * 这是事件生产者调用的接口，实现了事件产生和处理的解耦。
 * 
 * @param event_type 要添加的事件类型
 * 
 * 使用场景：
 * - 中断服务程序中添加事件
 * - 其他任务或模块中添加事件
 * - 定时器回调中添加事件
 * 
 * 注意事项：
 * - 如果队列满，事件将被丢弃
 * - 该函数可以被多个调用者调用
 * - 事件处理顺序与添加顺序一致(FIFO)
 * - 每次调用会自动递增事件参数值
 */
static void add_event_to_queue(unsigned short event_type)
{
    /* 定义静态事件结构体变量，保持值在函数调用间不丢失 */
    static struct fsm_event event = {0, 0};
    
    /* 设置事件类型 */
    event.super.type = event_type;
    /* 递增事件参数值，模拟不同事件的不同参数 */
    event.value++;

    /* 将事件添加到队列尾部 */
    queue_send(g_event_queue, (event_t *)&event);
}

/**
 * @brief 主函数
 * 
 * 程序入口函数，负责初始化硬件、状态机和消息队列，
 * 并演示状态机与消息队列结合使用，以及事件参数传递的效果。
 * 
 * 初始化流程：
 * 1. 硬件初始化
 * 2. 创建事件队列
 * 3. 构造状态机
 * 4. 初始化状态机
 * 
 * 测试场景：
 * 1. 场景1：测试在状态1下的各种事件
 * 2. 场景2：测试从状态1切换到状态2
 * 3. 场景3：测试从状态2切换到状态3
 * 
 * @return int 程序退出码(嵌入式系统通常不返回)
 */
int main(void)
{
    /* 硬件初始化 */
    Serial_Init();

    /* 创建事件队列，大小为事件结构体的大小 */
    g_event_queue = queue_create(sizeof(struct fsm_event));
    /* 构造状态机，设置初始状态处理函数 */
    fsm_ctor(&g_fsm, handle_state_init);
    /* 初始化状态机，触发状态初始化流程 */
    fsm_init(&g_fsm, (event_t *)0);

    /* 打印状态机实现方式标题 */
    printf("=== 状态机事件通知加参数 ===\r\n");

    /* 场景1: 当前在状态1 */
    printf("--- 场景1: 当前在状态1 ---\r\n");
    /* 添加各种事件到队列 */
    add_event_to_queue(EVENT_RUN_STATE1_ONLY); // 应该成功
    add_event_to_queue(EVENT_RUN_STATE2_ONLY); // 应该失败
    add_event_to_queue(EVENT_RUN_STATE3_ONLY); // 应该失败
    add_event_to_queue(EVENT_RUN_CURRENT);     // 应该成功

    /* 处理队列中的所有事件 */
    process_events();
    printf("\r\n");

    /* 场景2: 切换到状态2 */
    printf("--- 场景2: 切换到状态2 ---\r\n");
    /* 添加状态切换事件和其他测试事件 */
    add_event_to_queue(EVENT_GOTO_STATE2);
    add_event_to_queue(EVENT_RUN_STATE1_ONLY); // 应该失败
    add_event_to_queue(EVENT_RUN_STATE2_ONLY); // 应该成功
    add_event_to_queue(EVENT_RUN_STATE3_ONLY); // 应该失败
    add_event_to_queue(EVENT_RUN_CURRENT);     // 应该成功

    /* 处理队列中的所有事件 */
    process_events();
    printf("\r\n");

    /* 场景3: 切换到状态3 */
    printf("--- 场景3: 切换到状态3 ---\r\n");
    /* 添加状态切换事件和其他测试事件 */
    add_event_to_queue(EVENT_GOTO_STATE3);
    add_event_to_queue(EVENT_RUN_STATE1_ONLY); // 应该失败
    add_event_to_queue(EVENT_RUN_STATE2_ONLY); // 应该失败
    add_event_to_queue(EVENT_RUN_STATE3_ONLY); // 应该成功
    add_event_to_queue(EVENT_RUN_CURRENT);     // 应该成功

    /* 处理队列中的所有事件 */
    process_events();
    printf("\r\n");

    /* 主循环 */
    while (1)
    {

    }
}

/**
 * @brief 状态机的初始化函数
 * 
 * 该函数是状态机的初始状态处理函数，负责将状态机转换到状态1。
 * 这是状态机启动时调用的第一个状态处理函数。
 * 
 * @param self 状态机对象指针
 * @param event 触发的事件(此处为NULL)
 * @return status_t 处理结果，总是返回状态转换标志
 * 
 * 设计思路：
 * - 简单直接，只负责将状态机转换到状态1
 * - 使用TRAN_TO宏简化状态转换代码
 * - 不处理任何其他事件，保持简洁
 */
static status_t handle_state_init(fsm_t *self, event_t *event)
{
    /* 转换到状态1 */
    return TRAN_TO(handle_state1);
}

/**
 * @brief 状态1的处理函数
 * 
 * 该函数处理状态1下的所有事件，包括状态进入/退出、状态切换和业务逻辑执行。
 * 与前一个实现相比，增加了事件参数的处理和显示。
 * 
 * @param self 状态机对象指针
 * @param event 触发的事件(包含事件类型和参数)
 * @return status_t 处理结果，可能是状态转换标志或事件已处理标志
 * 
 * 事件处理：
 * - EVENT_STATE_ENTER: 状态进入时执行初始化操作
 * - EVENT_STATE_EXIT: 状态退出时执行清理操作
 * - EVENT_GOTO_STATE1: 已经是状态1，无需切换
 * - EVENT_GOTO_STATE2: 切换到状态2
 * - EVENT_GOTO_STATE3: 切换到状态3
 * - EVENT_RUN_CURRENT: 执行当前状态的业务逻辑
 * - EVENT_RUN_STATE1_ONLY: 执行状态1专属操作
 * - EVENT_RUN_STATE2_ONLY/EVENT_RUN_STATE3_ONLY: 在状态1下执行其他状态专属操作，失败
 * 
 * 新增功能：
 * - 显示事件类型和参数值
 * - 通过事件参数传递额外信息
 */
static status_t handle_state1(fsm_t *self, event_t *event)
{
    /* 默认返回事件已处理标志 */
    status_t ret = STATUS_HANDLED;

    /* 将事件指针转换为事件结构体指针 */
    struct fsm_event *evt = (struct fsm_event *)event;
    /* 显示事件类型和参数值 */
    printf("evt >>> type:%d - value:%d\r\n", evt->super.type, evt->value);

    switch (event->type)
    {
    case EVENT_STATE_ENTER:
        /* 处理进入状态1事件，执行状态初始化操作 */
        printf("   [进入] 正在进入状态1...\r\n");
        break;

    case EVENT_STATE_EXIT:
        /* 处理退出状态1事件，执行状态清理操作 */
        printf("   [退出] 正在退出状态1...\r\n");
        break;

    case EVENT_GOTO_STATE1:
        /* 已经是状态1，无需切换 */
        printf("切换到状态1 (已经是状态1)\r\n");
        break;

    case EVENT_GOTO_STATE2:
        /* 切换到状态2 */
        printf("切换到状态2\r\n");
        /* 使用TRAN_TO宏进行状态转换 */
        ret = TRAN_TO(handle_state2);
        break;

    case EVENT_GOTO_STATE3:
        /* 切换到状态3 */
        printf("切换到状态3\r\n");
        /* 使用TRAN_TO宏进行状态转换 */
        ret = TRAN_TO(handle_state3);
        break;

    case EVENT_RUN_CURRENT:
        /* 执行当前状态(状态1)的业务逻辑 */
        printf("执行当前状态\r\n");
        printf("   [执行] 正在运行状态 1 的逻辑...\r\n");
        break;

    case EVENT_RUN_STATE1_ONLY:
        /* 执行状态1专属操作 */
        printf("尝试执行[仅限状态1]指令 -> 成功！\r\n");
        printf("   [状态1专属] 只有在状态1才能看到这句话！\r\n");
        break;

    case EVENT_RUN_STATE2_ONLY:
    case EVENT_RUN_STATE3_ONLY:
        /* 在状态1下尝试执行其他状态的专属操作，失败 */
        printf("尝试执行[仅限其他状态]指令 -> 失败！当前是状态 1。\r\n");
        break;
    }

    return ret;
}

/**
 * @brief 状态2的处理函数
 * 
 * 该函数处理状态2下的所有事件，包括状态进入/退出、状态切换和业务逻辑执行。
 * 与前一个实现相比，增加了事件参数的处理和显示。
 * 
 * @param self 状态机对象指针
 * @param event 触发的事件(包含事件类型和参数)
 * @return status_t 处理结果，可能是状态转换标志或事件已处理标志
 * 
 * 事件处理：
 * - EVENT_STATE_ENTER: 状态进入时执行初始化操作
 * - EVENT_STATE_EXIT: 状态退出时执行清理操作
 * - EVENT_GOTO_STATE1: 切换到状态1
 * - EVENT_GOTO_STATE2: 已经是状态2，无需切换
 * - EVENT_GOTO_STATE3: 切换到状态3
 * - EVENT_RUN_CURRENT: 执行当前状态的业务逻辑
 * - EVENT_RUN_STATE2_ONLY: 执行状态2专属操作
 * - EVENT_RUN_STATE1_ONLY/EVENT_RUN_STATE3_ONLY: 在状态2下执行其他状态专属操作，失败
 * 
 * 新增功能：
 * - 显示事件类型和参数值
 * - 通过事件参数传递额外信息
 */
static status_t handle_state2(fsm_t *self, event_t *event)
{
    /* 默认返回事件已处理标志 */
    status_t ret = STATUS_HANDLED;

    /* 将事件指针转换为事件结构体指针 */
    struct fsm_event *evt = (struct fsm_event *)event;
    /* 显示事件类型和参数值 */
    printf("evt >>> type:%d - value:%d\r\n", evt->super.type, evt->value);

    switch (event->type)
    {
    case EVENT_STATE_ENTER:
        /* 处理进入状态2事件，执行状态初始化操作 */
        printf("   [进入] 正在进入状态2...\r\n");
        break;

    case EVENT_STATE_EXIT:
        /* 处理退出状态2事件，执行状态清理操作 */
        printf("   [退出] 正在退出状态2...\r\n");
        break;

    case EVENT_GOTO_STATE1:
        /* 切换到状态1 */
        printf("切换到状态1\r\n");
        /* 使用TRAN_TO宏进行状态转换 */
        ret = TRAN_TO(handle_state1);
        break;

    case EVENT_GOTO_STATE2:
        /* 已经是状态2，无需切换 */
        printf("切换到状态2 (已经是状态2)\r\n");
        break;

    case EVENT_GOTO_STATE3:
        /* 切换到状态3 */
        printf("切换到状态3\r\n");
        /* 使用TRAN_TO宏进行状态转换 */
        ret = TRAN_TO(handle_state3);
        break;

    case EVENT_RUN_CURRENT:
        /* 执行当前状态(状态2)的业务逻辑 */
        printf("执行当前状态\r\n");
        printf("   [执行] 正在运行状态 2 的逻辑...\r\n");
        break;

    case EVENT_RUN_STATE2_ONLY:
        /* 执行状态2专属操作 */
        printf("尝试执行[仅限状态2]指令 -> 成功！\r\n");
        printf("   [状态2专属] 只有在状态2才能看到这句话！\r\n");
        break;

    case EVENT_RUN_STATE1_ONLY:
    case EVENT_RUN_STATE3_ONLY:
        /* 在状态2下尝试执行其他状态的专属操作，失败 */
        printf("尝试执行[仅限其他状态]指令 -> 失败！当前是状态 2。\r\n");
        break;
    }

    return ret;
}

/**
 * @brief 状态3的处理函数
 * 
 * 该函数处理状态3下的所有事件，包括状态进入/退出、状态切换和业务逻辑执行。
 * 与前一个实现相比，增加了事件参数的处理和显示。
 * 
 * @param self 状态机对象指针
 * @param event 触发的事件(包含事件类型和参数)
 * @return status_t 处理结果，可能是状态转换标志或事件已处理标志
 * 
 * 事件处理：
 * - EVENT_STATE_ENTER: 状态进入时执行初始化操作
 * - EVENT_STATE_EXIT: 状态退出时执行清理操作
 * - EVENT_GOTO_STATE1: 切换到状态1
 * - EVENT_GOTO_STATE2: 切换到状态2
 * - EVENT_GOTO_STATE3: 已经是状态3，无需切换
 * - EVENT_RUN_CURRENT: 执行当前状态的业务逻辑
 * - EVENT_RUN_STATE3_ONLY: 执行状态3专属操作
 * - EVENT_RUN_STATE1_ONLY/EVENT_RUN_STATE2_ONLY: 在状态3下执行其他状态专属操作，失败
 * 
 * 新增功能：
 * - 显示事件类型和参数值
 * - 通过事件参数传递额外信息
 */
static status_t handle_state3(fsm_t *self, event_t *event)
{
    /* 默认返回事件已处理标志 */
    status_t ret = STATUS_HANDLED;

    /* 将事件指针转换为事件结构体指针 */
    struct fsm_event *evt = (struct fsm_event *)event;
    /* 显示事件类型和参数值 */
    printf("evt >>> type:%d - value:%d\r\n", evt->super.type, evt->value);

    switch (event->type)
    {
    case EVENT_STATE_ENTER:
        /* 处理进入状态3事件，执行状态初始化操作 */
        printf("   [进入] 正在进入状态3...\r\n");
        break;

    case EVENT_STATE_EXIT:
        /* 处理退出状态3事件，执行状态清理操作 */
        printf("   [退出] 正在退出状态3...\r\n");
        break;

    case EVENT_GOTO_STATE1:
        /* 切换到状态1 */
        printf("切换到状态1\r\n");
        /* 使用TRAN_TO宏进行状态转换 */
        ret = TRAN_TO(handle_state1);
        break;

    case EVENT_GOTO_STATE2:
        /* 切换到状态2 */
        printf("切换到状态2\r\n");
        /* 使用TRAN_TO宏进行状态转换 */
        ret = TRAN_TO(handle_state2);
        break;

    case EVENT_GOTO_STATE3:
        /* 已经是状态3，无需切换 */
        printf("切换到状态3 (已经是状态3)\r\n");
        break;

    case EVENT_RUN_CURRENT:
        /* 执行当前状态(状态3)的业务逻辑 */
        printf("执行当前状态\r\n");
        printf("   [执行] 正在运行状态 3 的逻辑...\r\n");
        break;

    case EVENT_RUN_STATE3_ONLY:
        /* 执行状态3专属操作 */
        printf("尝试执行[仅限状态3]指令 -> 成功！\r\n");
        printf("   [状态3专属] 只有在状态3才能看到这句话！\r\n");
        break;

    case EVENT_RUN_STATE1_ONLY:
    case EVENT_RUN_STATE2_ONLY:
        /* 在状态3下尝试执行其他状态的专属操作，失败 */
        printf("尝试执行[仅限其他状态]指令 -> 失败！当前是状态 3。\r\n");
        break;
    }

    return ret;
}
