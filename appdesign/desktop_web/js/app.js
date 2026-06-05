/* ==========================================================
   桌面智能灵动岛 — 应用逻辑层
   渲染函数 + Tab 路由 + 事件处理 + 动画控制
   ========================================================== */

// ===================== 安全 DOM 获取 =====================
/** 获取元素，不存在时输出警告而非抛错 */
function $_(id) {
  const el = document.getElementById(id);
  if (!el) console.warn('[DesktopPet] 元素未找到: #' + id);
  return el;
}

// ===================== 开幕动画 =====================
function initOpening() {
  const overlay = $_('openingOverlay');
  const video = $_('openingVideo');
  const skipBtn = $_('openingSkip');
  const appMain = $_('appMain');
  if (!overlay || !appMain) return;

  // 检查是否已看过开幕（同会话跳过）
  if (sessionStorage.getItem('desktoppet_opened')) {
    overlay.style.display = 'none';
    appMain.style.display = '';
    return;
  }

  if (video) {
    video.play().catch(() => {});
    // 播完自动进入
    video.addEventListener('ended', () => finishOpening(overlay, appMain));
    // 3 秒最低播放时间
    video.addEventListener('timeupdate', function soon() {
      if (video.currentTime > 0.5) {
        video.removeEventListener('timeupdate', soon);
      }
    });
  }

  // 跳过按钮
  if (skipBtn) {
    skipBtn.addEventListener('click', () => finishOpening(overlay, appMain));
  }
}

function finishOpening(overlay, appMain) {
  if (overlay._done) return;
  overlay._done = true;
  overlay.classList.add('fade-out');
  const video = $_('openingVideo');
  if (video) video.pause();
  appMain.style.display = '';
  sessionStorage.setItem('desktoppet_opened', '1');
  // 开幕层淡出后移除
  setTimeout(() => { overlay.style.display = 'none'; }, 700);
}

// ===================== 宠物视频切换 =====================
let currentPetVideo = 'happy'; // happy | thinking | sleeping

function switchPetVideo(mood) {
  if (currentPetVideo === mood) return;
  currentPetVideo = mood;

  const video = $_('petVideo');
  if (!video) return;

  const srcMap = {
    happy: 'assets/pet_happy.mp4',
    thinking: 'assets/pet_thinking.mp4',
    sleeping: 'assets/pet_sleeping.mp4',
  };
  const src = srcMap[mood] || srcMap.happy;

  // 淡出 → 换源 → 淡入
  video.style.opacity = '0';
  setTimeout(() => {
    video.src = src;
    video.load();
    video.play().catch(() => {});
    video.style.opacity = '1';
  }, 250);
}

// ===================== 初始化入口 =====================
document.addEventListener('DOMContentLoaded', () => {
  AppState.init();
  DeviceStore.init();
  initOpening();
  renderAll();
  bindEvents();
  startDDLTimer();
  startBlinkTimer();
  initMqttUI();
  initMqttListeners();
});

/** 全量渲染 */
function renderAll() {
  renderSidebar();
  renderHomeTab();
  renderTasksTab();
  renderChatTab();
  renderSettingsTab();
}

// ===================== 左侧栏渲染 =====================
function renderSidebar() {
  const device = AppState.device;
  const mood = petMoodMap[device.petMood] || petMoodMap.idle;

  // 状态标签
  document.getElementById('petStatusLabel').textContent = mood.label;

  // 指标卡
  document.getElementById('indicatorTasks').textContent = AppState.getPendingCount();
  document.getElementById('indicatorBattery').textContent = device.battery + '%';
  document.getElementById('indicatorBattery').style.color =
    device.battery <= 20 ? 'var(--danger)' :
    device.battery <= 60 ? 'var(--warm-yellow)' : 'var(--success)';

  // 设备信息
  document.getElementById('deviceOnlineStatus').innerHTML =
    device.online ? '<span style="color:var(--success);">● 在线</span>' : '<span style="color:var(--text-tertiary);">● 离线</span>';
  document.getElementById('deviceUptime').textContent = '运行 ' + device.uptime;

  // 顶部栏状态
  const dot = document.getElementById('statusDot');
  const text = document.getElementById('statusText');
  if (device.online) {
    dot.className = 'topbar-status-dot online';
    text.textContent = '在线';
  } else {
    dot.className = 'topbar-status-dot offline';
    text.textContent = '离线';
  }
}

// ===================== 主页 Tab 渲染 =====================
function renderHomeTab() {
  const ddls = AppState.getUrgentDDLs();
  const ddlList = document.getElementById('ddlList');
  const ddlEmpty = document.getElementById('ddlEmpty');

  if (ddls.length === 0) {
    ddlList.innerHTML = '';
    ddlEmpty.style.display = 'block';
  } else {
    ddlEmpty.style.display = 'none';
    ddlList.innerHTML = ddls.map(task => {
      const pri = getPriorityConfig(task.priority);
      const dl = getDeadlineInfo(task.deadline);
      return `
        <div class="ddl-item" onclick="switchTab('tasks')">
          <div class="ddl-priority-bar ${pri.cls}"></div>
          <div class="ddl-priority-icon">${pri.icon}</div>
          <div class="ddl-body">
            <div class="ddl-title">${escapeHtml(task.title)}</div>
            <div class="ddl-meta ${dl.cls}">${dl.text}</div>
          </div>
          <span class="ddl-badge ${pri.cls}">${pri.label}优先</span>
        </div>
      `;
    }).join('');
  }
}

// ===================== 任务 Tab 渲染 =====================
function renderTasksTab() {
  const tasks = AppState.getFilteredTasks();
  const list = document.getElementById('taskList');
  const empty = document.getElementById('taskEmpty');
  const badge = $_('taskBadge');
  const pendingCount = AppState.getPendingCount();

  // 更新任务角标
  if (badge) {
    badge.textContent = pendingCount;
    badge.style.display = pendingCount > 0 ? 'flex' : 'none';
  }

  if (tasks.length === 0) {
    list.innerHTML = '';
    empty.style.display = 'block';
    document.getElementById('taskEmptyText').textContent =
      AppState.activeTaskFilter === 'done' ? '没有已完成任务' :
      AppState.activeTaskFilter === 'high' ? '没有高优先任务' : '暂无任务，点击 + 创建';
  } else {
    empty.style.display = 'none';
    list.innerHTML = tasks.map(task => {
      const pri = getPriorityConfig(task.priority);
      const deadline = task.deadline ? new Date(task.deadline) : null;
      const deadlineStr = deadline
        ? `${deadline.getFullYear()}-${(deadline.getMonth()+1).toString().padStart(2,'0')}-${deadline.getDate().toString().padStart(2,'0')} ${formatTime(deadline)}`
        : '';
      const dl = task.deadline ? getDeadlineInfo(task.deadline) : null;

      return `
        <div class="task-item ${task.done ? 'done' : ''}" data-id="${task.id}">
          <div class="task-checkbox" onclick="animateToggleTask('${task.id}');" title="标记完成/取消">
            ${task.done ? '✓' : ''}
          </div>
          <div class="task-body">
            <div class="task-title">${escapeHtml(task.title)}</div>
            <div class="task-detail">
              <span class="task-priority-tag ${pri.cls}">${pri.icon} ${pri.label}优先</span>
              ${deadlineStr ? `<span>📅 ${deadlineStr}</span>` : ''}
              ${dl ? `<span class="ddl-meta ${dl.cls}">${dl.remaining}</span>` : ''}
              ${task.estimatedMinutes ? `<span>⏱ ${task.estimatedMinutes}分钟</span>` : ''}
            </div>
          </div>
          <div class="task-actions">
            <button class="task-action-btn edit" onclick="event.stopPropagation(); openTaskModal('${task.id}');" title="编辑">✎</button>
            <button class="task-action-btn" onclick="event.stopPropagation(); animateDeleteTask('${task.id}');" title="删除">🗑</button>
          </div>
        </div>
      `;
    }).join('');
  }

  // 更新进度条
  renderTaskProgress();
}

function renderTaskProgress() {
  const total = AppState.tasks.length;
  const done = AppState.getDoneCount();
  document.getElementById('progressDone').textContent = done;
  document.getElementById('progressTotal').textContent = total;
  const pct = total > 0 ? Math.round((done / total) * 100) : 0;
  document.getElementById('progressFill').style.width = pct + '%';
}

// ===================== 对话 Tab 渲染 =====================
function renderChatTab() {
  const container = document.getElementById('chatMessages');
  const messages = AppState.messages;

  container.innerHTML = messages.map(msg => {
    const isUser = msg.role === 'user';
    const pushBtn = !isUser
      ? `<button class="msg-push-btn" onclick="pushToDevice('${msg.id}')">📤 推送到设备</button>`
      : '';
    return `
      <div class="chat-msg ${msg.role}">
        <div class="msg-bubble">${escapeHtml(msg.content).replace(/\n/g, '<br>')}</div>
        <div class="msg-time">${msg.time}</div>
        ${pushBtn}
      </div>
    `;
  }).join('');

  // 滚动到底部
  setTimeout(() => {
    container.scrollTop = container.scrollHeight;
  }, 50);
}

function renderSettingsTab() {
  // 设置页目前大多是静态内容，更新设备卡片即可
  const device = AppState.device;
  const sdCard = document.querySelector('.settings-device-card');
  if (sdCard) {
    const statusEl = sdCard.querySelector('.sd-status');
    const uptimeEl = sdCard.querySelector('.sd-uptime');
    if (statusEl) {
      statusEl.innerHTML = device.online
        ? '<span style="color:var(--success);">● 在线</span> · 电量 ' + device.battery + '%'
        : '<span style="color:var(--text-tertiary);">● 离线</span>';
    }
    if (uptimeEl) {
      uptimeEl.textContent = '运行 ' + device.uptime + ' · WiFi 信号良好';
    }
  }
}

// ===================== Tab 切换 =====================
function switchTab(tabName) {
  const prevActive = document.querySelector('.tab-page.active');
  const nextActive = document.querySelector('#tab-' + tabName);

  // 相同 tab，忽略
  if (prevActive === nextActive) return;

  AppState.activeTab = tabName;

  // 更新 Tab 按钮状态
  document.querySelectorAll('.tab-btn').forEach(btn => {
    btn.classList.toggle('active', btn.dataset.tab === tabName);
  });

  if (prevActive && nextActive) {
    // 旧页退场动画
    prevActive.classList.add('exiting');
    prevActive.addEventListener('animationend', function handler() {
      prevActive.removeEventListener('animationend', handler);
      prevActive.classList.remove('active', 'exiting');
    }, { once: true });
    // 等旧页退场一半时切入新页，视觉更连贯
    setTimeout(() => activateTab(nextActive, tabName), 80);
  } else {
    // 首次加载（无 prevActive）
    activateTab(nextActive, tabName);
  }
}

function activateTab(page, tabName) {
  page.classList.add('active');

  // 切换到对话页时滚动到底部
  if (tabName === 'chat') {
    setTimeout(() => {
      const container = document.getElementById('chatMessages');
      if (container) container.scrollTop = container.scrollHeight;
    }, 150);
  }

  // 进入 Agent 设置时渲染
  if (tabName === 'agent-settings') {
    initAgentSettingsPage();
    return;
  }

  // 切换任务页时重置筛选
  if (tabName === 'tasks') {
    AppState.activeTaskFilter = 'all';
    document.querySelectorAll('.filter-btn').forEach(b => b.classList.remove('active'));
    const allBtn = document.querySelector('.filter-btn[data-filter="all"]');
    if (allBtn) allBtn.classList.add('active');
    renderTasksTab();
  }
}

// ===================== 事件绑定 =====================
function bindEvents() {
  // Tab 切换
  document.querySelectorAll('.tab-btn').forEach(btn => {
    btn.addEventListener('click', () => switchTab(btn.dataset.tab));
  });

  // 任务筛选按钮
  document.querySelectorAll('.filter-btn').forEach(btn => {
    btn.addEventListener('click', () => {
      document.querySelectorAll('.filter-btn').forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
      AppState.activeTaskFilter = btn.dataset.filter;
      renderTasksTab();
    });
  });

  // 对话模式切换
  document.querySelectorAll('.chat-mode-btn').forEach(btn => {
    btn.addEventListener('click', () => {
      document.querySelectorAll('.chat-mode-btn').forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
      AppState.chatMode = btn.dataset.mode;
      AppState.save();
    });
  });

  // 语音输入
  const voiceBtn = $_('chatVoiceBtn');
  if (voiceBtn) voiceBtn.addEventListener('click', () => VoiceInput.toggle());

  // 对话发送
  const sendBtn = $_('chatSendBtn');
  const chatInput = $_('chatInput');
  if (sendBtn) sendBtn.addEventListener('click', () => handleChatSend());
  if (chatInput) {
    chatInput.addEventListener('keydown', (e) => {
      if (e.key === 'Enter' && !e.shiftKey) {
        e.preventDefault();
        handleChatSend();
      }
    });
  }

  // 宠物点击交互（视频版）
  const petContainer = $_('petContainer');
  if (petContainer) {
    petContainer.addEventListener('click', () => {
      // 弹跳动画
      petContainer.style.animation = 'none';
      petContainer.offsetHeight;
      petContainer.style.animation = 'pet-container-bounce 0.4s ease';

      const reactions = ['汪！🐾', '嗷呜~ ✨', '汪汪！💕', '呜~ 🎵', '汪！⏰'];
      const reaction = reactions[Math.floor(Math.random() * reactions.length)];
      toast(reaction, 'info');
    });
  }

  // 番茄钟按钮
  const pomoBtn = $_('pomodoroBtn');
  if (pomoBtn) {
    pomoBtn.addEventListener('click', () => {
      toast('🍅 番茄钟功能即将上线，敬请期待！', 'info');
    });
  }

  // 弹窗关闭（点击遮罩）
  const taskModal = $_('taskModal');
  if (taskModal) {
    taskModal.addEventListener('click', (e) => {
      if (e.target === e.currentTarget) closeTaskModal();
    });
  }

  // ESC 关闭弹窗
  document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') closeTaskModal();
  });

  // 优先级选择器
  document.querySelectorAll('.priority-option').forEach(btn => {
    btn.addEventListener('click', () => {
      document.querySelectorAll('.priority-option').forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
    });
  });
}

// ===================== 对话发送逻辑（接入真实 Agent） =====================
let _lastChatSendTime = 0;
const CHAT_COOLDOWN_MS = 3000;

async function handleChatSend() {
  // 速率限制：两次发送间隔 >= 3 秒
  const now = Date.now();
  const sinceLast = now - _lastChatSendTime;
  if (sinceLast < CHAT_COOLDOWN_MS) {
    const waitSec = Math.ceil((CHAT_COOLDOWN_MS - sinceLast) / 1000);
    toast(`请稍等 ${waitSec} 秒再发送`, 'error');
    return;
  }
  _lastChatSendTime = now;

  const input = $_('chatInput');
  if (!input) return;
  const text = input.value.trim();
  if (!text) return;
  input.value = '';

  // 添加用户消息
  const userMsg = {
    id: 'mu' + Date.now(),
    role: 'user',
    content: text,
    time: formatTime(new Date()),
  };
  AppState.messages.push(userMsg);
  AppState.save();
  renderChatTab();

  // 检查 API Key（getter 属性，不是函数）
  if (!AgentSettings.apiKey) {
    const tipMsg = {
      id: 'ma' + Date.now(),
      role: 'assistant',
      content: '⚠️ 还没设置 DeepSeek API Key 哦～请到 **设置 → AI 助手设置** 中填入 Key，我才能变聪明！',
      time: formatTime(new Date()),
    };
    AppState.messages.push(tipMsg);
    AppState.save();
    renderChatTab();
    return;
  }

  // 显示思考中
  const container = $_('chatMessages');
  const thinkingEl = document.createElement('div');
  thinkingEl.className = 'chat-msg assistant';
  thinkingEl.innerHTML = '<div class="msg-bubble thinking-dots">🤔 思考中<span>.</span><span>.</span><span>.</span></div>';
  thinkingEl.id = 'thinking-indicator';
  if (container) { container.appendChild(thinkingEl); container.scrollTop = container.scrollHeight; }

  // 切换宠物到思考态
  switchPetVideo('thinking');

  try {
    const results = await Agent.run(text);

    // 移除思考指示器
    const indicator = $_('thinking-indicator');
    if (indicator) indicator.remove();

    // 处理结果
    let finalText = '';
    const toolCalls = [];

    for (const r of results) {
      if (r.type === 'text') {
        finalText += r.content;
      } else if (r.type === 'tool_call') {
        toolCalls.push(r.name);
      } else if (r.type === 'compact_ready') {
        finalText += '\n\n💡 *对话已达 ' + r.rounds + ' 轮，你可以选择清空上下文或存档继续。*';
      }
    }

    if (!finalText && toolCalls.length > 0) {
      finalText = '已完成：' + toolCalls.map(t => toolLabel(t)).join('、');
    }

    const aiMsg = {
      id: 'ma' + Date.now(),
      role: 'assistant',
      content: finalText || '嗯...让我想想 🤔',
      time: formatTime(new Date()),
    };
    AppState.messages.push(aiMsg);
    AppState.save();
    renderChatTab();

    // 宠物切回开心
    switchPetVideo('happy');

  } catch (e) {
    const indicator = $_('thinking-indicator');
    if (indicator) indicator.remove();

    let errText;
    if (e.message === 'NO_API_KEY') {
      errText = '⚠️ 请先在设置中填入 DeepSeek API Key';
    } else if (e.message?.includes('timeout') || e.message?.includes('AbortError')) {
      errText = '⏱ 请求超时，请检查网络或稍后重试';
    } else {
      errText = '❌ ' + (e.message || '未知错误');
    }

    const errMsg = {
      id: 'ma' + Date.now(),
      role: 'assistant',
      content: errText,
      time: formatTime(new Date()),
    };
    AppState.messages.push(errMsg);
    AppState.save();
    renderChatTab();
    switchPetVideo('happy');
  }
}

function toolLabel(name) {
  const map = {
    createTask: '创建任务', updateTask: '修改任务', deleteTask: '删除任务',
    listTasks: '查询任务', toggleTask: '切换状态', getDDLSummary: '查看DDL',
    getWeather: '查天气', webSearch: '联网搜索',
  };
  return map[name] || name;
}

/** 推送到设备 */
function pushToDevice(msgId) {
  const msg = AppState.messages.find(m => m.id === msgId);
  if (msg) {
    toast('📤 已推送到桌面灵动岛！', 'success');
  }
}

// ===================== 动效辅助函数 =====================

/** 带退场动画的任务删除 */
function animateDeleteTask(taskId) {
  const el = document.querySelector('.task-item[data-id="' + taskId + '"]');
  if (el) {
    el.classList.add('task-leave');
    el.addEventListener('animationend', function () {
      AppState.deleteTask(taskId);
      renderAll();
      toast('任务已删除', 'info');
    }, { once: true });
  } else {
    AppState.deleteTask(taskId);
    renderAll();
    toast('任务已删除', 'info');
  }
}

/** 带弹跳动画的任务完成/取消 */
function animateToggleTask(taskId) {
  AppState.toggleTask(taskId);
  renderAll();
  setTimeout(() => {
    const refreshed = document.querySelector('.task-item[data-id="' + taskId + '"]');
    if (refreshed) refreshed.classList.add('task-toggle');
  }, 30);
}

// ===================== 任务弹窗 =====================
function openTaskModal(taskId) {
  const modal = document.getElementById('taskModal');
  const formTitle = document.getElementById('formTitle');
  const formDeadline = document.getElementById('formDeadline');
  const formDuration = document.getElementById('formDuration');
  const formNotes = document.getElementById('formNotes');
  const modalTitle = document.getElementById('modalTitle');
  const editId = document.getElementById('editTaskId');

  if (taskId) {
    // 编辑模式
    const task = AppState.tasks.find(t => t.id === taskId);
    if (!task) return;
    modalTitle.textContent = '编辑任务';
    editId.value = taskId;
    formTitle.value = task.title;
    formDuration.value = task.estimatedMinutes || '';
    formNotes.value = task.notes || '';
    // 设置 deadline
    if (task.deadline) {
      const d = new Date(task.deadline);
      formDeadline.value = d.getFullYear() + '-' +
        (d.getMonth() + 1).toString().padStart(2, '0') + '-' +
        d.getDate().toString().padStart(2, '0') + 'T' +
        d.getHours().toString().padStart(2, '0') + ':' +
        d.getMinutes().toString().padStart(2, '0');
    } else {
      formDeadline.value = '';
    }
    // 设置优先级
    document.querySelectorAll('.priority-option').forEach(b => {
      b.classList.toggle('active', parseInt(b.dataset.priority) === task.priority);
    });
  } else {
    // 新建模式
    modalTitle.textContent = '新建任务';
    editId.value = '';
    formTitle.value = '';
    formDuration.value = '';
    formNotes.value = '';
    formDeadline.value = '';
    document.querySelectorAll('.priority-option').forEach((b, i) => {
      b.classList.toggle('active', i === 1); // 默认中优先
    });
  }

  modal.classList.add('open');
  setTimeout(() => formTitle.focus(), 150);
}

function closeTaskModal() {
  document.getElementById('taskModal').classList.remove('open');
  document.getElementById('editTaskId').value = '';
}

function saveTask() {
  const editId = document.getElementById('editTaskId').value;
  const title = document.getElementById('formTitle').value.trim();
  const deadline = document.getElementById('formDeadline').value;
  const duration = document.getElementById('formDuration').value;
  const notes = document.getElementById('formNotes').value.trim();
  const priorityBtn = document.querySelector('.priority-option.active');
  const priority = priorityBtn ? parseInt(priorityBtn.dataset.priority) : 2;

  if (!title) {
    toast('请输入任务标题', 'error');
    return;
  }

  const taskData = {
    title,
    priority,
    deadline: deadline ? new Date(deadline).toISOString() : null,
    estimatedMinutes: duration ? parseInt(duration) : null,
    notes,
  };

  let newTaskId = null;
  if (editId) {
    AppState.updateTask(editId, taskData);
    toast('任务已更新', 'success');
  } else {
    const newTask = AppState.addTask(taskData);
    newTaskId = newTask.id;
    toast('任务已创建', 'success');
  }

  closeTaskModal();
  renderAll();

  // 新建任务执行入场动画
  if (newTaskId) {
    setTimeout(() => {
      const el = document.querySelector('.task-item[data-id="' + newTaskId + '"]');
      if (el) el.classList.add('task-enter');
    }, 50);
  }
}

// ===================== DDL 倒计时更新 =====================
let ddlTimer = null;
function startDDLTimer() {
  if (ddlTimer) clearInterval(ddlTimer);
  ddlTimer = setInterval(() => {
    // 只在主页活跃时刷新 DDL 显示
    if (AppState.activeTab === 'home') {
      renderHomeTab();
    }
    // 任务页也刷新剩余时间
    if (AppState.activeTab === 'tasks') {
      renderTasksTab();
    }
  }, 30000); // 每30秒刷新一次
}

// ===================== Toast 提示 =====================
function toast(message, type) {
  type = type || 'info';
  const container = document.getElementById('toastContainer');
  const el = document.createElement('div');
  el.className = 'toast ' + type;
  el.textContent = message;
  container.appendChild(el);

  // 3秒后自动移除
  setTimeout(() => {
    if (el.parentNode) el.parentNode.removeChild(el);
  }, 3000);
}

// ===================== HTML 转义 =====================
/** 纯字符串替换，避免反复创建 DOM 节点 */
function escapeHtml(str) {
  if (str == null) return '';
  return String(str)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#39;');
}

// ===================== 宠物弹跳 + 眨眼 =====================
const petAnimStyle = document.createElement('style');
petAnimStyle.textContent = `
  @keyframes pet-container-bounce {
    0%   { transform: scale(1); }
    30%  { transform: scale(1.08); }
    50%  { transform: scale(0.95); }
    70%  { transform: scale(1.03); }
    100% { transform: scale(1); }
  }
`;
document.head.appendChild(petAnimStyle);

// 眨眼计时器：每 3~5 秒眨眼一次
let blinkTimer = null;
function startBlinkTimer() {
  blinkTimer = setInterval(() => {
    triggerBlink();
  }, 3000 + Math.random() * 2000);
}

function triggerBlink() {
  const video = $_('petVideo');
  if (!video) return;
  // 瞬间 scaleY 缩小模拟眨眼
  video.style.transition = 'transform 0.06s ease';
  video.style.transform = 'scaleY(0.08)';
  setTimeout(() => {
    video.style.transform = 'scaleY(1)';
  }, 80);
}

// ===================== Agent 设置页 =====================
function initAgentSettingsPage() {
  const soulEditor = $_('agentSoulEditor');
  if (soulEditor) soulEditor.value = AgentSettings.soul;

  const compactEl = $_('agentCompactAt');
  if (compactEl) compactEl.value = AgentSettings.compactAt;

  const roundsEl = $_('agentMaxRounds');
  if (roundsEl) roundsEl.value = AgentSettings.maxRounds;

  const searchEl = $_('agentWebSearch');
  if (searchEl) searchEl.checked = AgentSettings.webSearch;

  const apiKeyEl = $_('agentApiKey');
  if (apiKeyEl) apiKeyEl.value = AgentSettings.apiKey;

  const modelEl = $_('agentModel');
  if (modelEl) modelEl.value = AgentSettings.model;

  const workerUrlEl = $_('agentWorkerUrl');
  if (workerUrlEl) workerUrlEl.value = AgentSettings.workerUrl;

  // 更新 API Key 状态指示
  const statusEl = $_('apiKeyStatus');
  if (statusEl) {
    const hasKey = AgentSettings.apiKey;
    statusEl.textContent = hasKey ? '✅ 已配置' : '⚠️ 未配置';
    statusEl.style.color = hasKey ? 'var(--success)' : 'var(--danger)';
  }

  renderMemoryList();
}

function resetAgentSoul() {
  const editor = $_('agentSoulEditor');
  if (editor && confirm('恢复默认 Soul？当前内容将丢失。')) {
    editor.value = DEFAULT_SOUL;
    AgentSettings.save({ soul: DEFAULT_SOUL });
    toast('已恢复默认 Soul', 'success');
  }
}

function saveAgentSoul() {
  const editor = $_('agentSoulEditor');
  if (editor) {
    AgentSettings.save({ soul: editor.value });
    toast('Soul 已保存 ✓', 'success');
  }
}

function saveAgentApiKey() {
  const apiKeyEl = $_('agentApiKey');
  const modelEl = $_('agentModel');
  const apiKey = apiKeyEl?.value?.trim() || '';
  const model = modelEl?.value || 'deepseek-chat';
  const workerUrlEl = $_('agentWorkerUrl');
  const workerUrl = workerUrlEl?.value?.trim() || '';
  if (!apiKey) {
    toast('请输入 API Key', 'error');
    return;
  }
  AgentSettings.save({ apiKey, model, workerUrl });
  // 重置对话上下文，让新 Key 立即生效
  Agent.reset();
  // 更新状态指示
  const statusEl = $_('apiKeyStatus');
  if (statusEl) {
    statusEl.textContent = '✅ 已配置';
    statusEl.style.color = 'var(--success)';
  }
  toast('✅ API 设置已保存', 'success');
}

function saveAgentCompactSetting() {
  const compactAt = parseInt($_('agentCompactAt')?.value) || 8;
  const maxRounds = parseInt($_('agentMaxRounds')?.value) || 5;
  const webSearch = $_('agentWebSearch')?.checked !== false;
  AgentSettings.save({ compactAt, maxRounds, webSearch });
  toast('设置已保存 ✓', 'success');
}

function renderMemoryList() {
  const list = $_('memoryList');
  const empty = $_('memoryEmpty');
  if (!list) return;
  const memories = MemoryStore.getAll();
  if (memories.length === 0) {
    list.innerHTML = '<div class="empty-state"><div class="empty-icon">📭</div><p>暂无存档</p></div>';
    return;
  }
  list.innerHTML = memories.map(m => `
    <div class="memory-item">
      <div class="memory-item-info">
        <div class="memory-item-title">${escapeHtml(m.title)}</div>
        <div class="memory-item-meta">${new Date(m.createdAt).toLocaleString('zh-CN')} · ${Math.round(m.content.length / 100) / 10}k 字</div>
      </div>
      <div class="memory-item-actions">
        <button class="task-action-btn edit" onclick="loadMemoryToChat('${m.id}')" title="加载到对话">⇧</button>
        <button class="task-action-btn" onclick="MemoryStore.delete('${m.id}'); renderMemoryList(); toast('已删除存档', 'info');" title="删除">🗑</button>
      </div>
    </div>
  `).join('');
}

function loadMemoryToChat(id) {
  if (Agent.loadMemory(id)) {
    toast('已加载存档到当前对话', 'success');
    switchTab('chat');
    const msg = {
      id: 'ms' + Date.now(),
      role: 'assistant',
      content: '📋 已加载存档内容，继续聊吧~',
      time: formatTime(new Date()),
    };
    AppState.messages.push(msg);
    renderChatTab();
  }
}

function archiveCurrentConversation() {
  const title = prompt('给这段对话起个名字：', new Date().toLocaleString('zh-CN') + ' 对话');
  if (!title) return;
  const mem = Agent.saveToMemory(title);
  toast('已存档「' + title + '」', 'success');
  renderMemoryList();
}

function compactOrResetContext() {
  const rounds = Agent.roundCount;
  if (rounds === 0) {
    toast('当前没有对话需要处理', 'info');
    return;
  }
  const choice = confirm(
    `当前对话 ${rounds} 轮。\n\n` +
    `「确定」= 压缩上下文（保留最近4轮，其余摘要）\n` +
    `「取消」= 先存档再清空`
  );
  if (choice) {
    Agent.compact();
    toast('上下文已压缩', 'success');
  } else {
    archiveCurrentConversation();
    Agent.reset();
    AppState.messages = [];
    AppState.save();
    renderChatTab();
  }
}

// ===================== 语音输入模块 =====================
const VoiceInput = {
  recognition: null,
  listening: false,

  /** 初始化 SpeechRecognition 实例 */
  _init() {
    if (this.recognition) return true;
    const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition;
    if (!SpeechRecognition) {
      toast('⚠ 您的浏览器不支持语音输入，请使用 Chrome', 'error');
      return false;
    }
    const rec = new SpeechRecognition();
    rec.lang = 'zh-CN';
    rec.interimResults = true;
    rec.continuous = false;
    rec.maxAlternatives = 1;

    rec.onresult = (e) => {
      let transcript = '';
      for (let i = e.resultIndex; i < e.results.length; i++) {
        transcript += e.results[i][0].transcript;
      }
      const chatInput = $_('chatInput');
      if (chatInput) {
        const old = chatInput.value.trim();
        chatInput.value = old ? old + ' ' + transcript : transcript;
        chatInput.focus();
      }
      // 最终结果：高亮提示
      if (e.results[e.results.length - 1].isFinal) {
        toast('🗣 ' + transcript.slice(0, 40) + (transcript.length > 40 ? '...' : ''), 'success');
        this._stopUI();
        this.listening = false;
      }
    };

    rec.onerror = (e) => {
      console.warn('SpeechRecognition error:', e.error);
      this._stopUI();
      this.listening = false;
      if (e.error === 'not-allowed') {
        toast('⚠ 麦克风权限被拒绝，请在浏览器设置中允许', 'error');
      } else if (e.error !== 'aborted') {
        toast('🎤 语音识别出错，请重试', 'error');
      }
    };

    rec.onend = () => {
      this._stopUI();
      this.listening = false;
    };

    rec.onstart = () => {
      this.listening = true;
      const chatInput = $_('chatInput');
      // 保留已有文字，追加模式
    };

    this.recognition = rec;
    return true;
  },

  /** 切换录音状态 */
  toggle() {
    if (!this._init()) return;
    if (this.listening) {
      this.stop();
    } else {
      this.start();
    }
  },

  start() {
    if (!this._init()) return;
    try {
      this.recognition.start();
      this._startUI();
    } catch (e) {
      console.warn('start failed:', e);
    }
  },

  stop() {
    if (this.recognition) {
      try { this.recognition.stop(); } catch (e) {}
    }
    this._stopUI();
    this.listening = false;
  },

  /** 录音中 UI 反馈 */
  _startUI() {
    const btn = $_('chatVoiceBtn');
    if (btn) {
      btn.classList.add('listening');
      btn.textContent = '🔴';
    }
    const input = $_('chatInput');
    if (input) input.placeholder = '正在聆听...';
  },

  _stopUI() {
    const btn = $_('chatVoiceBtn');
    if (btn) {
      btn.classList.remove('listening');
      btn.textContent = '🎤';
    }
    const input = $_('chatInput');
    if (input) input.placeholder = '输入消息...';
  },
};

// ===================== MQTT + 设备发现 =====================

/** 初始化 MQTT 相关的 UI 控件 */
function initMqttUI() {
  // 回填 Broker 配置
  const hostEl = $_('mqttBrokerHost');
  const portEl = $_('mqttBrokerPort');
  if (hostEl) hostEl.value = DeviceStore.brokerHost;
  if (portEl) portEl.value = DeviceStore.brokerPort;

  // 刷新状态显示
  updateMqttStatusUI();

  // 如果已绑定设备，显示设备信息
  if (DeviceStore.boundDevice) {
    updateBoundDeviceUI();
  }
}

/** 监听 MQTT 状态变化（从 mqtt.js 发出的全局事件） */
function initMqttListeners() {
  window.addEventListener('mqtt-state', (e) => {
    updateMqttStatusUI();
  });

  // 监听 MQTT 消息：设备状态上报 → 更新 AppState
  window.addEventListener('mqtt-message', (e) => {
    const { topic, payload } = e.detail;
    if (DeviceStore.boundDevice && topic === `device/${DeviceStore.boundDevice.id}/status`) {
      // 实时更新设备状态
      if (payload.online !== undefined) AppState.device.online = payload.online;
      if (payload.battery !== undefined) AppState.device.battery = payload.battery;
      if (payload.pet_mood !== undefined) AppState.device.petMood = payload.pet_mood;
      if (payload.state !== undefined) AppState.device.state = payload.state;
      AppState.save();

      // 更新宠物视频
      if (payload.pet_mood) {
        switchPetVideo(payload.pet_mood);
      }

      // 刷新 UI
      renderSidebar();
      renderSettingsTab();
    }
  });
}

/** 连接/断开 MQTT Broker */
async function MqttConnect() {
  const hostEl = $_('mqttBrokerHost');
  const portEl = $_('mqttBrokerPort');

  const host = (hostEl && hostEl.value.trim()) ? hostEl.value.trim() : 'broker.emqx.io';
  const port = (portEl && parseInt(portEl.value)) ? parseInt(portEl.value) : 8083;

  // 保存配置
  DeviceStore.brokerHost = host;
  DeviceStore.brokerPort = port;
  DeviceStore.saveBrokerConfig();

  // 如果已连接 → 断开
  if (MqttClient.connected) {
    await MqttClient.disconnect();
    updateMqttStatusUI();
    toast('MQTT 已断开', 'info');
    return;
  }

  toast('正在连接 MQTT...', 'info');
  const ok = await MqttClient.connect({
    host: host,
    port: port,
    deviceId: DeviceStore.boundDevice ? DeviceStore.boundDevice.id : '',
  });

  updateMqttStatusUI();

  if (ok) {
    toast('MQTT 已连接 ✅', 'success');

    // 如果已绑定设备，重新订阅主题
    if (DeviceStore.boundDevice) {
      MqttClient.setDeviceId(DeviceStore.boundDevice.id);
      // 订阅设备状态
      MqttClient.subscribe(`device/${DeviceStore.boundDevice.id}/status`);
    }

    // 始终订阅发现应答
    MqttClient.subscribe('device/+/discover/ack');
  } else {
    toast('MQTT 连接失败，请检查 Broker 地址', 'error');
  }
}

/** 刷新 MQTT 状态文字 */
function updateMqttStatusUI() {
  const el = $_('mqttStatusDisplay');
  const btn = $_('btnScanDevice');
  if (!el) return;

  if (MqttClient.connected) {
    el.innerHTML = '<span style="color:var(--success);">🟢 已连接</span>';
    el.className = 'si-value success';
  } else {
    el.innerHTML = '<span style="color:var(--text-tertiary);">⚪ 未连接</span>';
    el.className = 'si-value';
    // 未连接时扫描按钮仍可用（会提示先连接）
  }
}

/** 开始设备扫描 */
function startDeviceScan() {
  if (!MqttClient.connected) {
    toast('请先连接 MQTT Broker', 'error');
    return;
  }

  const btn = $_('btnScanDevice');
  if (DeviceStore.isScanning) {
    // 停止扫描
    MqttClient.stopDiscovery();
    DeviceStore.isScanning = false;
    if (btn) { btn.textContent = '🔍 扫描'; btn.disabled = false; }
    toast('扫描已停止', 'info');
    return;
  }

  // 清空之前的设备
  DeviceStore.discovered = [];
  renderDiscoveredDevices();

  DeviceStore.isScanning = true;
  if (btn) { btn.textContent = '⏳ 扫描中...'; btn.disabled = true; }

  toast('🔍 正在搜索机器狗...', 'info');

  MqttClient.startDiscovery((devices) => {
    DeviceStore.discovered = devices;
    renderDiscoveredDevices();
  });

  // 5 秒后自动停止（与 mqtt.js 中的 timeout 对应）
  setTimeout(() => {
    if (DeviceStore.isScanning) {
      DeviceStore.isScanning = false;
      if (btn) { btn.textContent = '🔍 扫描'; btn.disabled = false; }
      const count = DeviceStore.discovered.length;
      toast(count > 0 ? `发现 ${count} 台设备 🐕` : '未发现设备，请确保机器狗已开机并联网', count > 0 ? 'success' : 'info');
    }
  }, 5500);
}

/** 渲染已发现的设备列表 */
function renderDiscoveredDevices() {
  const container = $_('discoveredDeviceList');
  if (!container) return;

  const devices = DeviceStore.discovered;

  if (devices.length === 0) {
    container.style.display = 'none';
    return;
  }

  container.style.display = 'block';
  container.innerHTML = devices.map(d => {
    const isBound = DeviceStore.boundDevice && DeviceStore.boundDevice.id === d.id;
    const batteryColor = d.battery >= 80 ? 'var(--success)' :
                         d.battery >= 40 ? 'var(--warm-yellow)' : 'var(--danger)';
    const moodMap = {
      happy: '😊', listening: '👂', thinking: '🤔', sleepy: '😴', idle: '✨',
    };
    const moodEmoji = moodMap[d.petMood] || '🐕';

    return `
      <div class="discovered-device-card ${isBound ? 'bound' : ''}" onclick="bindToDevice('${d.id}')">
        <div class="dd-icon">${moodEmoji}</div>
        <div class="dd-info">
          <div class="dd-name">${escapeHtml(d.name)} <span class="dd-id">#${escapeHtml(d.id)}</span></div>
          <div class="dd-meta">
            <span>🔋 <b style="color:${batteryColor};">${d.battery}%</b></span>
            <span>📡 ${d.ip || '?'}</span>
            <span>${d.online ? '🟢 在线' : '⚫ 离线'}</span>
          </div>
        </div>
        <div class="dd-action">
          ${isBound ? '<span class="dd-bound-badge">已绑定</span>' : '<button class="btn btn-primary btn-sm" style="padding:4px 12px;font-size:11px;">绑定</button>'}
        </div>
      </div>
    `;
  }).join('');
}

/** 绑定设备 */
async function bindToDevice(deviceId) {
  const device = DeviceStore.discovered.find(d => d.id === deviceId);
  if (!device) return;

  DeviceStore.bindDevice(device);
  updateBoundDeviceUI();
  renderDiscoveredDevices();

  // 订阅该设备的 MQTT 主题
  if (MqttClient.connected) {
    MqttClient.setDeviceId(deviceId);
    MqttClient.subscribe(`device/${deviceId}/status`);
    MqttClient.subscribe(`device/${deviceId}/task/list`);
    MqttClient.subscribe(`device/${deviceId}/task/done`);
    MqttClient.subscribe(`device/${deviceId}/pomodoro/status`);
  }

  // 更新设备名
  AppState.device.name = device.name;
  AppState.save();

  toast(`已绑定 ${device.name} 🐕`, 'success');
  renderSidebar();
  renderSettingsTab();
}

/** 刷新绑定设备 UI */
function updateBoundDeviceUI() {
  const el = $_('boundDeviceDisplay');
  if (!el) return;

  const device = DeviceStore.boundDevice;
  if (device) {
    el.innerHTML = `${device.name} <span style="font-size:11px;">#${device.id}</span>
      <button class="btn-text danger" onclick="unbindDevice()" style="margin-left:8px;font-size:11px;">解绑</button>`;
    el.style.color = 'var(--text)';
  } else {
    el.innerHTML = '未绑定';
    el.style.color = 'var(--text-tertiary)';
  }
}

/** 解绑设备 */
function unbindDevice() {
  if (!confirm('确定要解绑当前设备吗？')) return;
  DeviceStore.unbindDevice();
  updateBoundDeviceUI();
  renderDiscoveredDevices();
  toast('已解绑设备', 'info');
}

/** 加速页面加载时自动连接 MQTT（如果已配置且有绑定设备） */
async function autoConnectMqtt() {
  if (!DeviceStore.boundDevice) return; // 没绑定设备就不自动连
  if (MqttClient.connected) return;

  const ok = await MqttClient.connect({
    host: DeviceStore.brokerHost,
    port: DeviceStore.brokerPort,
    deviceId: DeviceStore.boundDevice.id,
  });

  if (ok) {
    updateMqttStatusUI();
    MqttClient.subscribe(`device/${DeviceStore.boundDevice.id}/status`);
    MqttClient.subscribe('device/+/discover/ack');
  }
}

// 页面加载后 2 秒自动尝试连接
setTimeout(() => { autoConnectMqtt(); }, 2000);
