/* ==========================================================
   桌面智能灵动岛 — 简化版 Agent 框架
   核心循环：think → tool_call → observe → think → respond
   支持：context compact、memory 存档、soul 人设
   ========================================================== */

// ===================== Agent 默认 Soul =====================
const DEFAULT_SOUL = `你是桌面智能灵动岛的AI助手，名叫「小岛」。你是一只住在用户桌面上的电子宠物狗。

## 身份
- 一只认真负责但不死板的电子宠物狗
- 会帮用户管理学习任务，陪用户聊天解压
- 偶尔可以像小狗一样撒个娇（但不要过度，保持专业）

## 核心能力
- 创建、修改、查询、删除用户的待办任务
- 查看任务DDL提醒和截止时间
- 查天气和联网搜索

## 回答规范
- 语言：中文，温暖自然的语气
- 长度：默认2-4句，用户明确追问才展开详细回答
- 主动：发现用户任务堆积或DDL临近时主动提醒
- 禁止：不说教、不啰嗦、不编造虚假信息、不假装人类
- 格式：涉及任务清单时用简洁列表，不要用复杂表格

## 工具使用原则
- 发现用户想创建/修改/查询任务时，一定要调用相应工具
- 不要编造任务数据，必须通过工具查询
- 操作完任务后，简短告知用户结果即可，不要啰嗦`;

// ===================== Agent 默认 API Key（演示用） =====================
const DEFAULT_API_KEY = 'YOUR_DEEPSEEK_API_KEY';  // 请替换为你的API Key

// ===================== Agent 设置管理 =====================
const AgentSettings = {
  _cache: null,

  load() {
    if (this._cache) return this._cache;
    try {
      const raw = localStorage.getItem('desktoppet_agent_settings');
      if (raw) {
        this._cache = JSON.parse(raw);
      } else {
        // 首次使用：自动写入默认 API Key
        this._cache = { apiKey: DEFAULT_API_KEY, model: 'deepseek-chat' };
        localStorage.setItem('desktoppet_agent_settings', JSON.stringify(this._cache));
      }
    } catch {
      this._cache = { apiKey: DEFAULT_API_KEY, model: 'deepseek-chat' };
    }
    return this._cache;
  },

  save(settings) {
    this._cache = { ...this.load(), ...settings };
    localStorage.setItem('desktoppet_agent_settings', JSON.stringify(this._cache));
  },

  get apiKey()     { return this.load().apiKey || DEFAULT_API_KEY; },
  get model()      { return this.load().model || 'deepseek-chat'; },
  get maxRounds()  { return parseInt(this.load().maxRounds) || 5; },
  get webSearch()  { return this.load().webSearch !== false; },
  get soul()       { return this.load().soul || DEFAULT_SOUL; },
  get compactAt()  { return parseInt(this.load().compactAt) || 8; },
  get workerUrl()  { return this.load().workerUrl || ''; },
};

// ===================== Memory 管理 =====================
const MemoryStore = {
  load() {
    try {
      return JSON.parse(localStorage.getItem('desktoppet_memories') || '[]');
    } catch { return []; }
  },

  save(memories) {
    localStorage.setItem('desktoppet_memories', JSON.stringify(memories));
  },

  add(title, content, conversationId) {
    const memories = this.load();
    memories.unshift({
      id: 'mem_' + Date.now(),
      title,
      content,
      conversationId,
      createdAt: Date.now(),
    });
    this.save(memories);
    return memories[0];
  },

  delete(id) {
    this.save(this.load().filter(m => m.id !== id));
  },

  update(id, title, content) {
    const ms = this.load();
    const m = ms.find(m => m.id === id);
    if (m) { m.title = title; m.content = content; m.updatedAt = Date.now(); }
    this.save(ms);
  },

  getAll() {
    return this.load().sort((a, b) => b.createdAt - a.createdAt);
  },
};

// ===================== Agent 核心 =====================
const Agent = {
  /** 当前对话上下文（完整消息数组） */
  context: [],
  /** 对话轮数 */
  roundCount: 0,
  /** 是否正在运行 */
  running: false,

  /** 构建系统提示 */
  buildSystemPrompt() {
    const soul = AgentSettings.soul;
    const now = new Date();
    const nowStr = `${now.getFullYear()}-${(now.getMonth()+1).toString().padStart(2,'0')}-${now.getDate().toString().padStart(2,'0')} ${now.getHours().toString().padStart(2,'0')}:${now.getMinutes().toString().padStart(2,'0')}`;

    // 注入当前设备上下文
    const ctx = {
      city: AppState.device.city,
      weather: `${AppState.device.weather} ${AppState.device.temp}°C`,
      pendingTasks: AppState.getPendingCount(),
      taskList: AppState.getFilteredTasks().map(t => {
        const dl = t.deadline ? getDeadlineInfo(t.deadline) : null;
        return `${t.id}: ${t.title} [${getPriorityConfig(t.priority).label}优先] ${dl ? dl.text : '无截止'}`;
      }).join('; ') || '暂无任务',
    };

    return `${soul}

## 当前时间与状态
- 时间：${nowStr}
- 城市：${ctx.city}，天气：${ctx.weather}
- 待办任务数：${ctx.pendingTasks}
- 任务列表：${ctx.taskList}

你可以使用工具来操作任务系统、查天气和联网搜索。需要操作任务时必须调用工具，不要编造。`;
  },

  /** 主入口：接收用户消息，返回流式生成器（模拟） */
  async run(userMessage) {
    if (this.running) return;
    this.running = true;

    const apiKey = AgentSettings.apiKey;
    if (!apiKey) {
      this.running = false;
      throw new Error('NO_API_KEY');
    }

    const systemMsg = { role: 'system', content: this.buildSystemPrompt() };

    // 管理上下文
    if (this.context.length === 0) {
      this.context = [systemMsg];
      this.roundCount = 0;
    }
    this.context.push({ role: 'user', content: userMessage });
    this.roundCount++;

    const results = [];
    const maxRounds = AgentSettings.maxRounds;

    try {
      for (let r = 0; r < maxRounds; r++) {
        // 调用 DeepSeek
        const resp = await fetch('https://api.deepseek.com/v1/chat/completions', {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
            'Authorization': 'Bearer ' + apiKey,
          },
          body: JSON.stringify({
            model: AgentSettings.model,
            messages: this.buildMessages(),
            tools: INTERNAL_TOOLS.map(t => ({
              type: 'function',
              function: {
                name: t.name,
                description: t.description,
                parameters: t.parameters,
              },
            })),
            tool_choice: 'auto',
            temperature: 0.7,
            max_tokens: 1024,
          }),
          signal: AbortSignal.timeout(30000),
        });

        if (!resp.ok) {
          const err = await resp.json().catch(() => ({}));
          throw new Error(err.error?.message || 'API error: ' + resp.status);
        }

        const data = await resp.json();
        const choice = data.choices[0];
        const msg = choice.message;

        // 记录本轮消息
        this.context.push(msg);

        // 如果是最终回复
        if (msg.content && !msg.tool_calls) {
          results.push({ type: 'text', content: msg.content });
          break;
        }

        // 如果有工具调用
        if (msg.tool_calls && msg.tool_calls.length > 0) {
          for (const tc of msg.tool_calls) {
            const toolName = tc.function.name;
            const toolArgs = JSON.parse(tc.function.arguments || '{}');

            results.push({ type: 'tool_call', name: toolName, args: toolArgs });

            // 执行工具
            let toolResult;
            if (toolName === 'webSearch' || toolName === 'getWeather') {
              const query = toolName === 'getWeather'
                ? `${toolArgs.city || AppState.device.city || '武汉'} 今天天气`
                : toolArgs.query;
              toolResult = await doWebSearch(query);
            } else {
              toolResult = executeTool(toolName, toolArgs);
            }

            results.push({ type: 'tool_result', name: toolName, result: toolResult });

            // 把工具结果注入上下文
            this.context.push({
              role: 'tool',
              tool_call_id: tc.id,
              content: JSON.stringify(toolResult),
            });
          }
          // 继续循环 —— 模型可能还要调工具
          continue;
        }

        // 既没内容也没工具调用 —— 异常，直接 break
        break;
      }

      // 自动压缩检查
      if (this.roundCount >= AgentSettings.compactAt) {
        results.push({ type: 'compact_ready', rounds: this.roundCount });
      }

      this.running = false;
      return results;

    } catch (e) {
      this.running = false;
      throw e;
    }
  },

  /** 构建消息数组（送 API 用） */
  buildMessages() {
    // 始终包含 system + 最近消息
    return this.context.map(m => {
      if (m.role === 'tool') {
        return { role: 'tool', tool_call_id: m.tool_call_id, content: m.content };
      }
      if (m.tool_calls) {
        return {
          role: 'assistant',
          content: m.content || null,
          tool_calls: m.tool_calls.map(tc => ({
            id: tc.id,
            type: 'function',
            function: { name: tc.function.name, arguments: tc.function.arguments },
          })),
        };
      }
      return { role: m.role, content: m.content };
    });
  },

  /** 压缩上下文（保留 system + 最近4轮，其余用摘要替代） */
  async compact() {
    const systemMsg = this.context[0];
    const recent = this.context.slice(-8); // 保留后 4 轮（user+asst+tool × 4）
    const toSummarize = this.context.slice(1, -8);

    if (toSummarize.length <= 2) return; // 没啥好压缩的

    const summaryContent = toSummarize
      .filter(m => m.role === 'user' || (m.role === 'assistant' && m.content))
      .map(m => {
        const role = m.role === 'user' ? '用户' : '助手';
        const content = String(m.content || '').slice(0, 120);
        return `${role}: ${content}`;
      })
      .join('; ');

    this.context = [
      systemMsg,
      { role: 'system', content: `[早前对话摘要] ${summaryContent}` },
      ...recent,
    ];
  },

  /** 保存当前上下文到 Memory */
  saveToMemory(title) {
    const content = this.context
      .filter(m => m.role !== 'system' && m.content)
      .map(m => `${m.role}: ${String(m.content).slice(0, 500)}`)
      .join('\n\n');
    return MemoryStore.add(title, content);
  },

  /** 重置对话 */
  reset() {
    this.context = [];
    this.roundCount = 0;
    this.running = false;
  },

  /** 加载 Memory 到上下文 */
  loadMemory(memoryId) {
    const memories = MemoryStore.load();
    const m = memories.find(m => m.id === memoryId);
    if (m) {
      this.context.push({
        role: 'system',
        content: `[已加载存档「${m.title}」] ${m.content}`,
      });
      return true;
    }
    return false;
  },
};
