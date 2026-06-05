/* ==========================================================
   桌面智能灵动岛 — 数据层
   Mock 数据 + 状态管理 + LocalStorage 持久化
   ========================================================== */

// ===================== Mock 设备状态 =====================
const mockDevice = {
  online: true,
  battery: 85,
  state: 'idle',
  petMood: 'happy',
  pomodoroRemain: 0,
  city: '武汉',
  weather: '晴',
  temp: 26,
  wifiRssi: -45,
  uptime: '2h34m',
  name: '桌面灵动岛 #A3B2',
  version: 'v1.0.0',
};

// ===================== Mock 宠物状态映射 =====================
const petMoodMap = {
  idle:      { label: '✨ 开心中~', emoji: '🐕' },
  voice:     { label: '👂 听着呢~', emoji: '🐕' },
  thinking:  { label: '💭 想事情...', emoji: '🐕' },
  pomodoro:  { label: '🍅 陪着你~', emoji: '🐕' },
  charging:  { label: '⚡ 充充电~', emoji: '🐕' },
  sleeping:  { label: '😴 没电了...', emoji: '🐕' },
};

// ===================== Mock 任务数据 =====================
const mockTasks = [
  {
    id: 't1',
    title: '完成高数作业',
    priority: 1,
    deadline: new Date(Date.now() + 3.5 * 3600000).toISOString(),
    estimatedMinutes: 45,
    notes: '第3章课后习题1-20',
    done: false,
    createdAt: Date.now() - 86400000,
  },
  {
    id: 't2',
    title: '复习模电第三章',
    priority: 2,
    deadline: new Date(Date.now() + 18 * 3600000).toISOString(),
    estimatedMinutes: 60,
    notes: '重点：放大器频率响应',
    done: false,
    createdAt: Date.now() - 72000000,
  },
  {
    id: 't3',
    title: '预习实验报告',
    priority: 3,
    deadline: new Date(Date.now() + 4 * 86400000).toISOString(),
    estimatedMinutes: 30,
    notes: '',
    done: false,
    createdAt: Date.now() - 172800000,
  },
  {
    id: 't4',
    title: '背英语单词50个',
    priority: 2,
    deadline: new Date(Date.now() + 12 * 3600000).toISOString(),
    estimatedMinutes: 25,
    notes: 'CET-6 词汇表',
    done: true,
    createdAt: Date.now() - 259200000,
  },
  {
    id: 't5',
    title: '准备明天课堂展示PPT',
    priority: 1,
    deadline: new Date(Date.now() + 20 * 3600000).toISOString(),
    estimatedMinutes: 90,
    notes: '主题：嵌入式AI应用',
    done: false,
    createdAt: Date.now() - 43200000,
  },
];

// ===================== Mock 对话数据 =====================
const mockMessages = [
  {
    id: 'm1',
    role: 'assistant',
    content: '你好！我是桌面灵动岛的AI助手，可以帮你解答问题、查询天气、管理任务~',
    time: '09:00',
  },
  {
    id: 'm2',
    role: 'user',
    content: '今天天气怎么样？',
    time: '09:05',
  },
  {
    id: 'm3',
    role: 'assistant',
    content: '今天武汉天气晴朗，温度22~28°C，北风3级，湿度65%。适合出门走走~',
    time: '09:06',
  },
  {
    id: 'm4',
    role: 'user',
    content: '帮我规划一下今天的学习计划',
    time: '10:30',
  },
  {
    id: 'm5',
    role: 'assistant',
    content: '根据你的任务列表，建议今天优先完成：\n1. 高数作业（今天18:00截止）\n2. 准备课堂展示PPT\n3. 背英语单词\n\n加油！合理安排时间，注意休息 🍅',
    time: '10:31',
  },
];

// ===================== 预置 AI 回复 =====================
const aiReplies = [
  '好的，我来帮你处理~ 😊',
  '这是个好问题！让我想想... 根据你的情况，我建议先从最紧急的任务开始。',
  '没问题！已经帮你记住了。还有其他需要吗？',
  '今天表现很棒！继续加油~ 💪',
  '明白了！我会在合适的时间提醒你。',
  '让我查一下... 根据你的任务列表，接下来应该优先完成高数作业哦。',
];

// ===================== 应用状态管理 =====================
const AppState = {
  device: { ...mockDevice },
  tasks: [],
  messages: [],
  activeTab: 'home',
  activeTaskFilter: 'all',
  chatMode: 'cloud',
  editingTaskId: null,

  // ---- 初始化：优先读 LocalStorage ----
  init() {
    const savedTasks = loadFromStorage('desktoppet_tasks');
    const savedMessages = loadFromStorage('desktoppet_messages');
    const savedDevice = loadFromStorage('desktoppet_device');
    const savedChatMode = localStorage.getItem('desktoppet_chat_mode');

    this.tasks = savedTasks || mockTasks.map(t => ({ ...t }));
    this.messages = savedMessages || mockMessages.map(m => ({ ...m }));
    if (savedDevice) this.device = { ...mockDevice, ...savedDevice };
    if (savedChatMode) this.chatMode = savedChatMode;
  },

  // ---- 持久化 ----
  save() {
    saveToStorage('desktoppet_tasks', this.tasks);
    saveToStorage('desktoppet_messages', this.messages);
    saveToStorage('desktoppet_device', this.device);
    localStorage.setItem('desktoppet_chat_mode', this.chatMode);
  },

  // ---- 任务操作 ----
  addTask(taskData) {
    const newTask = {
      id: 't' + Date.now(),
      title: taskData.title,
      priority: taskData.priority || 2,
      deadline: taskData.deadline || new Date(Date.now() + 86400000).toISOString(),
      estimatedMinutes: taskData.estimatedMinutes || null,
      notes: taskData.notes || '',
      done: false,
      createdAt: Date.now(),
    };
    this.tasks.unshift(newTask);
    this.save();
    return newTask;
  },

  updateTask(id, taskData) {
    const idx = this.tasks.findIndex(t => t.id === id);
    if (idx === -1) return;
    this.tasks[idx] = { ...this.tasks[idx], ...taskData };
    this.save();
  },

  toggleTask(id) {
    const task = this.tasks.find(t => t.id === id);
    if (task) {
      task.done = !task.done;
      this.save();
    }
  },

  deleteTask(id) {
    this.tasks = this.tasks.filter(t => t.id !== id);
    this.save();
  },

  // ---- 任务查询 ----
  getFilteredTasks() {
    let filtered = [...this.tasks];
    switch (this.activeTaskFilter) {
      case 'pending':
        filtered = filtered.filter(t => !t.done);
        break;
      case 'done':
        filtered = filtered.filter(t => t.done);
        break;
      case 'high':
        filtered = filtered.filter(t => t.priority === 1 && !t.done);
        break;
    }
    // 按优先级 + 截止时间排序
    filtered.sort((a, b) => {
      if (a.priority !== b.priority) return a.priority - b.priority;
      return new Date(a.deadline) - new Date(b.deadline);
    });
    return filtered;
  },

  getPendingCount() {
    return this.tasks.filter(t => !t.done).length;
  },

  getDoneCount() {
    return this.tasks.filter(t => t.done).length;
  },

  getUrgentDDLs() {
    return this.tasks
      .filter(t => !t.done && t.deadline)
      .sort((a, b) => new Date(a.deadline) - new Date(b.deadline))
      .slice(0, 5);
  },

  // ---- 对话操作 ----
  sendMessage(text) {
    const userMsg = {
      id: 'mu' + Date.now(),
      role: 'user',
      content: text,
      time: formatTime(new Date()),
    };
    this.messages.push(userMsg);

    // 模拟 AI 回复（1~2秒延迟，真实场景替换为 API 调用）
    const delay = 800 + Math.random() * 1200;
    const reply = aiReplies[Math.floor(Math.random() * aiReplies.length)];
    const aiMsg = {
      id: 'ma' + Date.now(),
      role: 'assistant',
      content: reply,
      time: formatTime(new Date(Date.now() + delay)),
    };

    // 用 setTimeout 实现异步回复（外部处理渲染）
    this.save();
    return { userMsg, aiMsg, delay };
  },

  // ---- 宠物状态 ----
  setPetMood(mood) {
    this.device.petMood = mood;
    this.save();
  },
};

// ===================== LocalStorage 工具函数 =====================
function loadFromStorage(key) {
  try {
    const raw = localStorage.getItem(key);
    if (!raw) return null;
    return JSON.parse(raw);
  } catch (e) {
    console.warn('localStorage 读取失败:', key, e);
    return null;
  }
}

function saveToStorage(key, data) {
  try {
    localStorage.setItem(key, JSON.stringify(data));
  } catch (e) {
    console.warn('localStorage 写入失败:', key, e);
  }
}

// ===================== 工具函数 =====================
function formatTime(date) {
  const h = date.getHours().toString().padStart(2, '0');
  const m = date.getMinutes().toString().padStart(2, '0');
  return h + ':' + m;
}

/** 计算截止时间的中文描述和剩余时间 */
function getDeadlineInfo(deadlineISO) {
  const now = Date.now();
  const dl = new Date(deadlineISO).getTime();
  const remainMs = dl - now;

  if (remainMs <= 0) {
    return { text: '已过期！', cls: 'urgent', remaining: '已过期' };
  }

  const remainMin = Math.floor(remainMs / 60000);
  const remainHour = Math.floor(remainMs / 3600000);
  const remainDay = Math.floor(remainMs / 86400000);

  if (remainMin < 60) {
    return {
      text: `${remainMin} 分钟后截止`,
      cls: 'urgent',
      remaining: `剩余 ${remainMin} 分钟`,
    };
  }
  if (remainHour < 6) {
    return {
      text: `${remainHour} 小时 ${remainMin % 60} 分钟后截止`,
      cls: 'urgent',
      remaining: `剩余 ${remainHour}h${remainMin % 60}m`,
    };
  }
  if (remainHour < 24) {
    return {
      text: `今天 ${formatTime(new Date(dl))} 截止`,
      cls: 'soon',
      remaining: `剩余 ${remainHour} 小时`,
    };
  }
  if (remainDay < 3) {
    return {
      text: `${remainDay} 天后截止`,
      cls: 'soon',
      remaining: `剩余 ${remainDay} 天`,
    };
  }
  return {
    text: `${remainDay} 天后截止`,
    cls: 'normal',
    remaining: `剩余 ${remainDay} 天`,
  };
}

/** 获取优先级配置 */
function getPriorityConfig(priority) {
  switch (priority) {
    case 1: return { icon: '🔥', label: '高', cls: 'high' };
    case 2: return { icon: '⚡', label: '中', cls: 'medium' };
    case 3: return { icon: '•', label: '低', cls: 'low' };
    default: return { icon: '•', label: '低', cls: 'low' };
  }
}

/** 生成唯一消息 ID */
function genId() {
  return Date.now().toString(36) + Math.random().toString(36).slice(2, 6);
}

// ===================== MQTT & 设备发现状态 =====================
const DeviceStore = {
  /** 已发现的设备列表 */
  discovered: [],

  /** 当前绑定的设备（null = 未绑定） */
  boundDevice: null,

  /** 是否正在扫描 */
  isScanning: false,

  /** MQTT Broker 配置 */
  brokerHost: 'broker.emqx.io',
  brokerPort: 8083,  // WebSocket 端口

  /** 从 localStorage 恢复 */
  init() {
    const saved = loadFromStorage('desktoppet_bound_device');
    if (saved) {
      this.boundDevice = saved;
    }
    const host = localStorage.getItem('desktoppet_mqtt_host');
    const port = localStorage.getItem('desktoppet_mqtt_port');
    if (host) this.brokerHost = host;
    if (port) this.brokerPort = parseInt(port) || 8083;
  },

  /** 保存绑定设备 */
  saveBoundDevice() {
    saveToStorage('desktoppet_bound_device', this.boundDevice);
  },

  /** 保存 Broker 配置 */
  saveBrokerConfig() {
    localStorage.setItem('desktoppet_mqtt_host', this.brokerHost);
    localStorage.setItem('desktoppet_mqtt_port', String(this.brokerPort));
  },

  /** 绑定设备 */
  bindDevice(device) {
    this.boundDevice = device;
    this.saveBoundDevice();
  },

  /** 解绑设备 */
  unbindDevice() {
    this.boundDevice = null;
    saveToStorage('desktoppet_bound_device', null);
  },
};
