/**
 * @brief  按键驱动核心函数，驱动状态机运转
 * @param  handle: 按键句柄结构体
 * @retval None
 */
static void button_handler(Button *handle) {
  uint8_t read_gpio_level = button_read_level(handle);

  // 非空闲状态下递增计时器（带饱和保护）
  if (handle->state > BTN_STATE_IDLE) {
    if (handle->ticks < UINT16_MAX) {
      handle->ticks++;
    }
  }

  /* 按键消抖处理 */
  if (read_gpio_level != handle->button_level) {
    // 新电平与历史不同，累加消抖计数
    if (++(handle->debounce_cnt) >= DEBOUNCE_TICKS) {
      handle->button_level = read_gpio_level;
      handle->debounce_cnt = 0;
    }
  } else {
    // 电平未变化，一票否决，计数器归零
    handle->debounce_cnt = 0;
  }

  /* 状态机 */
  switch (handle->state) {
  case BTN_STATE_IDLE:
    if (handle->button_level == handle->active_level) {
      // 检测到有效按下
      handle->event = (uint8_t)BTN_PRESS_DOWN;
      EVENT_CB(BTN_PRESS_DOWN);
      handle->ticks = 0;
      handle->repeat = 1;
      handle->state = BTN_STATE_PRESS;
    } else {
      handle->event = (uint8_t)BTN_NONE_PRESS;
    }
    break;

  case BTN_STATE_PRESS:
    if (handle->button_level != handle->active_level) {
      // 松手，进入释放等待期
      handle->event = (uint8_t)BTN_PRESS_UP;
      EVENT_CB(BTN_PRESS_UP);
      handle->ticks = 0;
      handle->state = BTN_STATE_RELEASE;
    } else if (handle->ticks > LONG_TICKS) {
      // 超时未松手，判定为长按
      handle->event = (uint8_t)BTN_LONG_PRESS_START;
      EVENT_CB(BTN_LONG_PRESS_START);
      handle->state = BTN_STATE_LONG_HOLD;
    }
    break;

  case BTN_STATE_RELEASE:
    if (handle->button_level == handle->active_level) {
      // 等待期内再次按下，进入连击
      handle->event = (uint8_t)BTN_PRESS_DOWN;
      EVENT_CB(BTN_PRESS_DOWN);
      if (handle->repeat < PRESS_REPEAT_MAX_NUM) {
        handle->repeat++;
      }
      handle->event = (uint8_t)BTN_PRESS_REPEAT;
      EVENT_CB(BTN_PRESS_REPEAT);
      handle->ticks = 0;
      handle->state = BTN_STATE_REPEAT;
    } else if (handle->ticks > SHORT_TICKS) {
      // 超时未再按，结算点击类型
      if (handle->repeat == 1) {
        handle->event = (uint8_t)BTN_SINGLE_CLICK;
        EVENT_CB(BTN_SINGLE_CLICK);
      } else if (handle->repeat == 2) {
        handle->event = (uint8_t)BTN_DOUBLE_CLICK;
        EVENT_CB(BTN_DOUBLE_CLICK);
      }
      handle->state = BTN_STATE_IDLE;
    }
    break;

  case BTN_STATE_REPEAT:
    if (handle->button_level != handle->active_level) {
      // 连击中松手
      handle->event = (uint8_t)BTN_PRESS_UP;
      EVENT_CB(BTN_PRESS_UP);
      if (handle->ticks < SHORT_TICKS) {
        handle->ticks = 0;
        handle->state = BTN_STATE_RELEASE; // 节奏内松手，继续等待更多连击
      } else {
        handle->state = BTN_STATE_IDLE; // 超时松手，连击序列终止
      }
    } else if (handle->ticks > SHORT_TICKS) {
      // 连击中按住不放，降级为新一次普通按下
      handle->ticks = 0;  // 清零计时，重新计算长按
      handle->repeat = 0; // 清除连击计数，开启新的按压周期
      handle->state = BTN_STATE_PRESS;
    }
    break;

  case BTN_STATE_LONG_HOLD:
    if (handle->button_level == handle->active_level) {
      // 持续按住，每 tick 派发长按保持事件
      handle->event = (uint8_t)BTN_LONG_PRESS_HOLD;
      EVENT_CB(BTN_LONG_PRESS_HOLD);
    } else {
      // 长按后松手
      handle->event = (uint8_t)BTN_PRESS_UP;
      EVENT_CB(BTN_PRESS_UP);
      handle->state = BTN_STATE_IDLE;
    }
    break;

  default:
    // 非法状态，强制复位
    handle->state = BTN_STATE_IDLE;
    break;
  }
}

/**
 * @brief  启动按键工作，将句柄挂载到工作链表（头插法）
 * @param  handle: 目标按键句柄结构体
 * @retval 0: 成功, -1: 已存在, -2: 无效参数
 */
int button_start(Button *handle) {
  if (!handle)
    return -2; // 无效参数

  MULTIBUTTON_LOCK();
  Button *target = head_handle;
  while (target) {
    if (target == handle) {
      MULTIBUTTON_UNLOCK();
      return -1; // 已存在于链表，防止成环
    }
    target = target->next;
  }

  // 头插法：先牵手，再移交
  handle->next = head_handle;
  head_handle = handle;
  MULTIBUTTON_UNLOCK();
  return 0;
}

/**
 * @brief  停止按键工作，将句柄从工作链表中摘除
 * @param  handle: 目标按键句柄结构体
 * @retval None
 */
void button_stop(Button *handle) {
  if (!handle)
    return; // 参数校验

  MULTIBUTTON_LOCK();
  Button **curr;
  for (curr = &head_handle; *curr;) {
    Button *entry = *curr;
    if (entry == handle) {
      *curr = entry->next;
      entry->next = NULL; // 清除后继指针，断开与链表的联系
      MULTIBUTTON_UNLOCK();
      return;
    } else {
      curr = &entry->next;
    }
  }
  MULTIBUTTON_UNLOCK();
}

/**
 * @brief  后台心跳驱动，定时器以 5ms 间隔调用
 *         回调在锁外执行，可安全调用 button_start()/button_stop() 而无死锁风险
 *         预读取 next 指针，防止回调中 stop 当前节点导致野指针
 * @param  None
 * @retval None
 */
void button_ticks(void) {
  Button *target;
  Button *next;

  MULTIBUTTON_LOCK();
  target = head_handle;
  MULTIBUTTON_UNLOCK();

  while (target) {
    MULTIBUTTON_LOCK();
    next = target->next; // 预读取：先记住下一站，再执行当前节点
    MULTIBUTTON_UNLOCK();

    button_handler(target); // 锁外执行回调，避免死锁
    target = next;
  }
}