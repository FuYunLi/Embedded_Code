/*
 * 文件名: main.c
 * 描述: 使用函数指针数组实现的状态机示例
 * 平台: STM32F10x
 * 功能: 展示了使用函数指针数组实现的三状态有限状态机
 * 特点: 相比前两种方式，使用函数指针数组更加灵活，便于扩展
 * 优点: 代码结构清晰，易于维护，状态切换效率高
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stm32f10x.h" // Device header
#include "Serial.h"

/* 状态枚举定义 */
enum
{
    STATE_1, // 状态1
    STATE_2, // 状态2
    STATE_3  // 状态3
};

/* 事件枚举定义 */
enum
{
    EVENT_GOTO_STATE1,     // 切换到状态1事件
    EVENT_GOTO_STATE2,     // 切换到状态2事件
    EVENT_GOTO_STATE3,     // 切换到状态3事件
    EVENT_RUN_CURRENT,     // 执行当前状态逻辑事件
    EVENT_RUN_STATE1_ONLY, // 仅限状态1执行事件
    EVENT_RUN_STATE2_ONLY, // 仅限状态2执行事件
    EVENT_RUN_STATE3_ONLY  // 仅限状态3执行事件
};

/* 类型定义 */
typedef unsigned char state_t;  // 状态类型
typedef unsigned short event_t; // 事件类型

/* 状态名称字符串数组，用于调试输出 */
static const char *const state_name[] = {"STATE_1", "STATE_2", "STATE_3"};
/* 事件名称字符串数组，用于调试输出 */
static const char *const event_name[] = {"EVENT_GOTO_STATE1", "EVENT_GOTO_STATE2", "EVENT_GOTO_STATE3", "EVENT_RUN_CURRENT",
                                         "EVENT_RUN_STATE1_ONLY", "EVENT_RUN_STATE2_ONLY", "EVENT_RUN_STATE3_ONLY"};

/* 状态处理函数指针类型定义 */
typedef void (*state_handler_t)(event_t event);

/* 状态处理函数声明 */
static void handle_state1(event_t event);
static void handle_state2(event_t event);
static void handle_state3(event_t event);

/* 状态处理函数指针数组，将状态枚举值映射到对应的状态处理函数 */
static state_handler_t state_handler[] = {handle_state1, handle_state2, handle_state3};

/* 当前状态变量，初始为STATE_1 */
state_t current_state = STATE_1;

/*
 * 函数名: handle_state1
 * 描述: 处理状态1的事件响应
 * 参数: event - 触发的事件
 * 返回: 无
 */
static void handle_state1(event_t event)
{
    switch (event)
    {
    case EVENT_GOTO_STATE1:
        /* 已经是状态1，无需切换 */
        printf("切换到状态1 (已经是状态1)\r\n");
        break;
    case EVENT_GOTO_STATE2:
        /* 切换到状态2 */
        printf("切换到状态2\r\n");
        current_state = STATE_2;
        break;
    case EVENT_GOTO_STATE3:
        /* 切换到状态3 */
        printf("切换到状态3\r\n");
        current_state = STATE_3;
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
}

/*
 * 函数名: handle_state2
 * 描述: 处理状态2的事件响应
 * 参数: event - 触发的事件
 * 返回: 无
 */
static void handle_state2(event_t event)
{
    switch (event)
    {
    case EVENT_GOTO_STATE1:
        /* 切换到状态1 */
        printf("切换到状态1\r\n");
        current_state = STATE_1;
        break;
    case EVENT_GOTO_STATE2:
        /* 已经是状态2，无需切换 */
        printf("切换到状态2 (已经是状态2)\r\n");
        break;
    case EVENT_GOTO_STATE3:
        /* 切换到状态3 */
        printf("切换到状态3\r\n");
        current_state = STATE_3;
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
}

/*
 * 函数名: handle_state3
 * 描述: 处理状态3的事件响应
 * 参数: event - 触发的事件
 * 返回: 无
 */
static void handle_state3(event_t event)
{
    switch (event)
    {
    case EVENT_GOTO_STATE1:
        /* 切换到状态1 */
        printf("切换到状态1\r\n");
        current_state = STATE_1;
        break;
    case EVENT_GOTO_STATE2:
        /* 切换到状态2 */
        printf("切换到状态2\r\n");
        current_state = STATE_2;
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
}

/*
 * 函数名: state_machine_handler
 * 描述: 状态机事件处理函数，使用函数指针数组调用对应的状态处理函数
 * 参数: event - 触发的事件
 * 返回: 无
 * 优点: 通过函数指针数组直接调用，避免了switch-case结构，提高执行效率
 */
void state_machine_handler(event_t event)
{
    /* 打印当前状态和触发的事件，用于调试 */
    printf("\r\n>>> state:%s - event:%s\r\n", state_name[current_state], event_name[event]);

    /* 通过函数指针数组调用当前状态的处理函数 */
    (*state_handler[current_state])(event);
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

    /* 打印状态机实现方式标题 */
    printf("=== 函数指针数组映射调用 ===\r\n");

    /* 场景1：测试在状态1下的各种事件 */
    printf("--- 场景1: 当前在状态1 ---\r\n");
    state_machine_handler(EVENT_RUN_STATE1_ONLY); // 应该成功
    state_machine_handler(EVENT_RUN_STATE2_ONLY); // 应该失败
    state_machine_handler(EVENT_RUN_STATE3_ONLY); // 应该失败
    state_machine_handler(EVENT_RUN_CURRENT);     // 应该成功
    printf("\r\n");

    /* 场景2：切换到状态2并测试各种事件 */
    printf("--- 场景2: 切换到状态2 ---\r\n");
    state_machine_handler(EVENT_GOTO_STATE2);
    state_machine_handler(EVENT_RUN_STATE1_ONLY); // 应该失败
    state_machine_handler(EVENT_RUN_STATE2_ONLY); // 应该成功
    state_machine_handler(EVENT_RUN_STATE3_ONLY); // 应该失败
    state_machine_handler(EVENT_RUN_CURRENT);     // 应该成功
    printf("\r\n");

    /* 场景3：切换到状态3并测试各种事件 */
    printf("--- 场景3: 切换到状态3 ---\r\n");
    state_machine_handler(EVENT_GOTO_STATE3);
    state_machine_handler(EVENT_RUN_STATE1_ONLY); // 应该失败
    state_machine_handler(EVENT_RUN_STATE2_ONLY); // 应该失败
    state_machine_handler(EVENT_RUN_STATE3_ONLY); // 应该成功
    state_machine_handler(EVENT_RUN_CURRENT);     // 应该成功
    printf("\r\n");

    /* 主循环，保持程序运行 */
    while (1)
    {
    }
}
