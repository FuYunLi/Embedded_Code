/*
 * 文件名: main.c
 * 描述: 使用函数地址作为状态的状态机示例
 * 平台: STM32F10x
 * 功能: 展示了使用函数地址直接作为状态实现的有限状态机
 * 特点: 直接使用函数地址作为状态，省去了状态枚举和状态到函数的映射表
 * 优点: 1. 实现更加简洁，代码量更少
 *       2. 状态切换效率更高，直接通过函数指针切换
 *       3. 保持了面向对象封装的思想
 *       4. 状态转换更加可控，支持状态初始化和清理操作
 * 缺点: 1. 调试时状态显示为函数地址，不够直观
 *       2. 状态数量受限于函数指针数量
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stm32f10x.h" // Device header
#include "Serial.h"

/* 事件枚举定义 */
enum
{
    EVENT_STATE_INIT,      // 初始化状态事件，用于状态机初始化
    EVENT_STATE_ENTER,     // 进入状态事件，状态进入时触发
    EVENT_STATE_EXIT,      // 退出状态事件，状态退出时触发
    EVENT_GOTO_STATE1,     // 切换到状态1事件
    EVENT_GOTO_STATE2,     // 切换到状态2事件
    EVENT_GOTO_STATE3,     // 切换到状态3事件
    EVENT_RUN_CURRENT,     // 执行当前状态逻辑事件
    EVENT_RUN_STATE1_ONLY, // 仅限状态1执行事件
    EVENT_RUN_STATE2_ONLY, // 仅限状态2执行事件
    EVENT_RUN_STATE3_ONLY  // 仅限状态3执行事件
};

/* 状态处理结果状态枚举 */
enum
{
    STATUS_TRAN,    // 状态转换状态，表示需要执行状态转换
    STATUS_HANDLED, // 处理完成状态，表示事件已处理但无需状态转换
};

/* 类型定义 */
typedef unsigned short event_t; // 事件类型
typedef unsigned char status_t; // 状态处理结果类型

/* 状态名称字符串数组，用于调试输出 */
static const char *const state_name[] = {"STATE_1", "STATE_2", "STATE_3"};
/* 事件名称字符串数组，用于调试输出 */
static const char *const event_name[] = {"EVENT_STATE_INIT", "EVENT_STATE_ENTER", "EVENT_STATE_EXIT",
                                         "EVENT_GOTO_STATE1", "EVENT_GOTO_STATE2", "EVENT_GOTO_STATE3", "EVENT_RUN_CURRENT",
                                         "EVENT_RUN_STATE1_ONLY", "EVENT_RUN_STATE2_ONLY", "EVENT_RUN_STATE3_ONLY"};

/* 前向声明状态机结构体 */
typedef struct fsm fsm_t;

/* 状态处理函数指针类型定义，每个状态处理函数接收状态机实例指针和事件参数 */
typedef status_t (*state_handler_t)(fsm_t *self, event_t event);

/* 状态机结构体定义，模拟面向对象中的类 */
struct fsm
{
    state_handler_t state; // 状态处理函数指针，直接使用函数地址作为状态
};

/* 状态处理函数声明 */
static status_t handle_state_init(fsm_t *self, event_t event);
static status_t handle_state1(fsm_t *self, event_t event);
static status_t handle_state2(fsm_t *self, event_t event);
static status_t handle_state3(fsm_t *self, event_t event);

/* 全局状态机实例 */
fsm_t fsm;

/*
 * 函数名: fsm_ctor
 * 描述: 状态机构造函数，初始化状态机实例
 * 参数: self - 状态机实例指针
 *       initial - 初始状态处理函数
 * 返回: 无
 * 特点: 直接使用函数指针作为初始状态
 */
static void fsm_ctor(fsm_t *self, state_handler_t initial)
{
    /* 直接将状态设置为初始状态处理函数 */
    self->state = initial;
}

/*
 * 函数名: fsm_init
 * 描述: 状态机初始化函数，执行状态机的初始化流程
 * 参数: self - 状态机实例指针
 *       event - 初始化事件参数
 * 返回: 无
 * 流程: 1. 调用当前状态的初始化处理函数
 *       2. 调用当前状态的进入处理函数
 * 特点: 直接通过函数指针调用状态处理函数
 */
static void fsm_init(fsm_t *self, event_t event)
{
    /* 调用当前状态的初始化处理函数 */
    (*self->state)(self, event);
    /* 调用当前状态的进入处理函数 */
    (*self->state)(self, EVENT_STATE_ENTER);
}

/*
 * 函数名: handle_state_init
 * 描述: 处理初始状态的初始化事件
 * 参数: self - 状态机实例指针
 *       event - 触发的事件
 * 返回: status_t - 状态处理结果
 * 功能: 将状态机从初始状态转换到状态1
 * 特点: 直接设置状态为状态1的处理函数
 */
static status_t handle_state_init(fsm_t *self, event_t event)
{
    /* 直接将状态设置为状态1的处理函数 */
    self->state = handle_state1;
    /* 返回状态转换状态，表示需要执行状态转换 */
    return STATUS_TRAN;
}

/*
 * 函数名: handle_state1
 * 描述: 处理状态1的事件响应
 * 参数: self - 状态机实例指针
 *       event - 触发的事件
 * 返回: status_t - 状态处理结果
 * 特点: 状态转换时直接设置状态为对应的状态处理函数
 */
static status_t handle_state1(fsm_t *self, event_t event)
{
    /* 默认返回处理完成状态 */
    status_t ret = STATUS_HANDLED;
    switch (event)
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
        /* 切换到状态2，直接设置状态为状态2的处理函数 */
        printf("切换到状态2\r\n");
        self->state = handle_state2;
        /* 返回状态转换状态，表示需要执行状态转换 */
        ret = STATUS_TRAN;
        break;
    case EVENT_GOTO_STATE3:
        /* 切换到状态3，直接设置状态为状态3的处理函数 */
        printf("切换到状态3\r\n");
        self->state = handle_state3;
        /* 返回状态转换状态，表示需要执行状态转换 */
        ret = STATUS_TRAN;
        break;
    case EVENT_RUN_CURRENT:
        /* 执行当前状态(状态1)的逻辑 */
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

/*
 * 函数名: handle_state2
 * 描述: 处理状态2的事件响应
 * 参数: self - 状态机实例指针
 *       event - 触发的事件
 * 返回: status_t - 状态处理结果
 * 特点: 状态转换时直接设置状态为对应的状态处理函数
 */
static status_t handle_state2(fsm_t *self, event_t event)
{
    /* 默认返回处理完成状态 */
    status_t ret = STATUS_HANDLED;
    switch (event)
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
        /* 切换到状态1，直接设置状态为状态1的处理函数 */
        printf("切换到状态1\r\n");
        self->state = handle_state1;
        /* 返回状态转换状态，表示需要执行状态转换 */
        ret = STATUS_TRAN;
        break;
    case EVENT_GOTO_STATE2:
        /* 已经是状态2，无需切换 */
        printf("切换到状态2 (已经是状态2)\r\n");
        break;
    case EVENT_GOTO_STATE3:
        /* 切换到状态3，直接设置状态为状态3的处理函数 */
        printf("切换到状态3\r\n");
        self->state = handle_state3;
        /* 返回状态转换状态，表示需要执行状态转换 */
        ret = STATUS_TRAN;
        break;
    case EVENT_RUN_CURRENT:
        /* 执行当前状态(状态2)的逻辑 */
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

/*
 * 函数名: handle_state3
 * 描述: 处理状态3的事件响应
 * 参数: self - 状态机实例指针
 *       event - 触发的事件
 * 返回: status_t - 状态处理结果
 * 特点: 状态转换时直接设置状态为对应的状态处理函数
 */
static status_t handle_state3(fsm_t *self, event_t event)
{
    /* 默认返回处理完成状态 */
    status_t ret = STATUS_HANDLED;
    switch (event)
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
        /* 切换到状态1，直接设置状态为状态1的处理函数 */
        printf("切换到状态1\r\n");
        self->state = handle_state1;
        /* 返回状态转换状态，表示需要执行状态转换 */
        ret = STATUS_TRAN;
        break;
    case EVENT_GOTO_STATE2:
        /* 切换到状态2，直接设置状态为状态2的处理函数 */
        printf("切换到状态2\r\n");
        self->state = handle_state2;
        /* 返回状态转换状态，表示需要执行状态转换 */
        ret = STATUS_TRAN;
        break;
    case EVENT_GOTO_STATE3:
        /* 已经是状态3，无需切换 */
        printf("切换到状态3 (已经是状态3)\r\n");
        break;
    case EVENT_RUN_CURRENT:
        /* 执行当前状态(状态3)的逻辑 */
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

/*
 * 函数名: fsm_dispatch
 * 描述: 状态机事件分发函数，处理事件并执行状态转换
 * 参数: self - 状态机实例指针
 *       event - 触发的事件
 * 返回: 无
 * 流程: 1. 调用当前状态的处理函数处理事件
 *       2. 如果需要状态转换，则调用前一状态的退出处理函数和新状态的进入处理函数
 * 特点: 直接通过函数指针调用状态处理函数，状态显示为函数地址
 */
void fsm_dispatch(fsm_t *self, event_t event)
{
    /* 打印当前状态(函数地址)和触发的事件，用于调试 */
    printf("\r\n>>> state:%X - event:%s\r\n", (unsigned int)self->state, event_name[event]);

    /* 保存当前状态，用于状态转换时的退出处理 */
    state_handler_t prev_state = self->state;
    /* 调用当前状态的处理函数 */
    status_t status = (*self->state)(self, event);

    /* 如果返回状态转换状态，执行状态进入和退出处理 */
    if (status == STATUS_TRAN)
    {
        /* 调用前一状态的退出处理 */
        (*prev_state)(self, EVENT_STATE_EXIT);
        /* 调用新状态的进入处理 */
        (*self->state)(self, EVENT_STATE_ENTER);
    }
}

/*
 * 函数名: main
 * 描述: 主函数，程序入口
 * 参数: 无
 * 返回: int
 */
int main()
{
    /* 硬件初始化 */
    Serial_Init();

    /* 初始化状态机 */
    /* 1. 构造状态机实例，设置初始状态为初始化状态处理函数 */
    fsm_ctor(&fsm, handle_state_init);
    /* 2. 初始化状态机，执行初始化流程 */
    fsm_init(&fsm, 0);

    /* 打印状态机实现方式标题 */
    printf("=== 使用函数地址作为状态 ===\r\n");

    /* 场景1：测试在状态1下的各种事件 */
    printf("--- 场景1: 当前在状态1 ---\r\n");
    fsm_dispatch(&fsm, EVENT_RUN_STATE1_ONLY); // 应该成功
    fsm_dispatch(&fsm, EVENT_RUN_STATE2_ONLY); // 应该失败
    fsm_dispatch(&fsm, EVENT_RUN_STATE3_ONLY); // 应该失败
    fsm_dispatch(&fsm, EVENT_RUN_CURRENT);     // 应该成功
    printf("\r\n");

    /* 场景2：切换到状态2并测试各种事件 */
    printf("--- 场景2: 切换到状态2 ---\r\n");
    fsm_dispatch(&fsm, EVENT_GOTO_STATE2);
    fsm_dispatch(&fsm, EVENT_RUN_STATE1_ONLY); // 应该失败
    fsm_dispatch(&fsm, EVENT_RUN_STATE2_ONLY); // 应该成功
    fsm_dispatch(&fsm, EVENT_RUN_STATE3_ONLY); // 应该失败
    fsm_dispatch(&fsm, EVENT_RUN_CURRENT);     // 应该成功
    printf("\r\n");

    /* 场景3：切换到状态3并测试各种事件 */
    printf("--- 场景3: 切换到状态3 ---\r\n");
    fsm_dispatch(&fsm, EVENT_GOTO_STATE3);
    fsm_dispatch(&fsm, EVENT_RUN_STATE1_ONLY); // 应该失败
    fsm_dispatch(&fsm, EVENT_RUN_STATE2_ONLY); // 应该失败
    fsm_dispatch(&fsm, EVENT_RUN_STATE3_ONLY); // 应该成功
    fsm_dispatch(&fsm, EVENT_RUN_CURRENT);     // 应该成功
    printf("\r\n");

    /* 主循环，保持程序运行 */
    while (1)
    {
    }
}
