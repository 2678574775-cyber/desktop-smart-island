/* ==========================================================
   桌面智能灵动岛 — 内部工具封装
   标准 Function Calling Schema + 统一执行入口
   ========================================================== */

// ===================== 工具定义（OpenAI Function Calling 兼容） =====================
const INTERNAL_TOOLS = [
  {
    name: 'createTask',
    description: '创建一个新的待办任务。当用户说「帮我设置一个任务」「添加任务」「创建一个DDL」等时调用。',
    parameters: {
      type: 'object',
      properties: {
        title: { type: 'string', description: '任务标题' },
        priority: { type: 'number', enum: [1, 2, 3], description: '优先级：1=高 2=中 3=低，默认2' },
        deadline: { type: 'string', description: '截止时间，ISO格式如 2026-06-01T15:00:00，不提供则默认明天此时' },
        estimatedMinutes: { type: 'number', description: '预估时长（分钟），可选' },
        notes: { type: 'string', description: '备注，可选' },
      },
      required: ['title'],
    },
  },
  {
    name: 'updateTask',
    description: '修改已有任务的内容（标题/优先级/截止时间/备注）。需要 taskId。',
    parameters: {
      type: 'object',
      properties: {
        taskId: { type: 'string', description: '要修改的任务ID' },
        title: { type: 'string', description: '新标题，可选' },
        priority: { type: 'number', enum: [1, 2, 3], description: '新优先级，可选' },
        deadline: { type: 'string', description: '新截止时间，ISO格式，可选' },
        estimatedMinutes: { type: 'number', description: '新预估时长，可选' },
        notes: { type: 'string', description: '新备注，可选' },
      },
      required: ['taskId'],
    },
  },
  {
    name: 'deleteTask',
    description: '删除一个任务。**首次调用会返回任务详情要求确认，必须由用户明确同意后才能再次调用并传入 confirmed:true 执行删除。不要在用户未明确同意的情况下删除。**',
    parameters: {
      type: 'object',
      properties: {
        taskId: { type: 'string', description: '要删除的任务ID' },
        confirmed: { type: 'boolean', description: '用户已确认删除，设为true才会真正执行。不传或false只返回任务信息等待确认。' },
      },
      required: ['taskId'],
    },
  },
  {
    name: 'listTasks',
    description: '列出当前所有任务。可按状态筛选。',
    parameters: {
      type: 'object',
      properties: {
        filter: { type: 'string', enum: ['all', 'pending', 'done', 'high'], description: '筛选：all全部/pending待办/done已完成/high高优先，默认pending' },
        limit: { type: 'number', description: '返回条数，默认5' },
      },
    },
  },
  {
    name: 'toggleTask',
    description: '标记任务为完成或取消完成。需要 taskId。',
    parameters: {
      type: 'object',
      properties: {
        taskId: { type: 'string', description: '要切换完成状态的任务ID' },
      },
      required: ['taskId'],
    },
  },
  {
    name: 'getDDLSummary',
    description: '查看所有即将到期的DDL（截止日期提醒）概况，包括优先级、剩余时间。',
    parameters: {
      type: 'object',
      properties: {},
    },
  },
  {
    name: 'getWeather',
    description: '查询指定城市的天气。内部通过联网搜索获取最新天气预报。',
    parameters: {
      type: 'object',
      properties: {
        city: { type: 'string', description: '城市名，如「武汉」「北京」。不提供则使用设备当前城市。' },
      },
    },
  },
  {
    name: 'webSearch',
    description: '联网搜索实时信息。当用户问的问题需要最新数据时使用。',
    parameters: {
      type: 'object',
      properties: {
        query: { type: 'string', description: '搜索关键词' },
      },
      required: ['query'],
    },
  },
];

// ===================== 工具执行入口 =====================
/** 执行工具调用，返回 {success, data, error} */
function executeTool(toolName, args) {
  try {
    switch (toolName) {

      // === 任务操作 ===
      case 'createTask': {
        if (!args.title || !String(args.title).trim()) return fail('任务标题不能为空');
        const task = AppState.addTask({
          title: String(args.title).trim(),
          priority: parseInt(args.priority) || 2,
          deadline: args.deadline ? new Date(args.deadline).toISOString() : null,
          estimatedMinutes: args.estimatedMinutes ? parseInt(args.estimatedMinutes) : null,
          notes: args.notes ? String(args.notes).trim() : '',
        });
        renderAll();
        return ok({ taskId: task.id, title: task.title, priority: task.priority });
      }

      case 'updateTask': {
        if (!args.taskId) return fail('缺少 taskId');
        AppState.updateTask(String(args.taskId), {
          title: args.title ? String(args.title).trim() : undefined,
          priority: args.priority ? parseInt(args.priority) : undefined,
          deadline: args.deadline ? new Date(args.deadline).toISOString() : undefined,
          estimatedMinutes: args.estimatedMinutes ? parseInt(args.estimatedMinutes) : undefined,
          notes: args.notes !== undefined ? String(args.notes).trim() : undefined,
        });
        renderAll();
        return ok({ taskId: args.taskId, updated: true });
      }

      case 'deleteTask': {
        if (!args.taskId) return fail('缺少 taskId');
        const taskToDelete = AppState.tasks.find(t => t.id === String(args.taskId));
        if (!taskToDelete) return fail('任务不存在: ' + args.taskId);

        // 未确认：返回任务详情，等待用户同意
        if (!args.confirmed) {
          return ok({
            needConfirm: true,
            taskId: taskToDelete.id,
            taskTitle: taskToDelete.title,
            priority: getPriorityConfig(taskToDelete.priority).label,
            note: '请向用户确认是否删除此任务。用户同意后，再次调用 deleteTask 并传入 confirmed:true。',
          });
        }

        // 已确认：执行删除
        AppState.deleteTask(String(args.taskId));
        renderAll();
        return ok({ deleted: true, taskId: args.taskId, taskTitle: taskToDelete.title });
      }

      case 'listTasks': {
        const originalFilter = AppState.activeTaskFilter;
        const f = args.filter || 'pending';
        AppState.activeTaskFilter = f === 'high' ? 'high' : f === 'done' ? 'done' : f === 'pending' ? 'pending' : 'all';
        const tasks = AppState.getFilteredTasks();
        AppState.activeTaskFilter = originalFilter;

        const limit = parseInt(args.limit) || 5;
        const sliced = tasks.slice(0, limit).map(t => ({
          id: t.id,
          title: t.title,
          priority: t.priority,
          done: t.done,
          deadline: t.deadline || null,
          remaining: t.deadline ? getDeadlineInfo(t.deadline).remaining : null,
          estimatedMinutes: t.estimatedMinutes,
        }));
        return ok({ count: sliced.length, total: tasks.length, filter: f, tasks: sliced });
      }

      case 'toggleTask': {
        if (!args.taskId) return fail('缺少 taskId');
        AppState.toggleTask(String(args.taskId));
        renderAll();
        const task = AppState.tasks.find(t => t.id === String(args.taskId));
        return ok({ taskId: args.taskId, done: task ? task.done : null });
      }

      case 'getDDLSummary': {
        const ddls = AppState.getUrgentDDLs();
        const summary = ddls.map(t => {
          const dl = getDeadlineInfo(t.deadline);
          const pri = getPriorityConfig(t.priority);
          return {
            title: t.title,
            priority: pri.label,
            urgency: dl.cls,
            remaining: dl.remaining,
            deadline: dl.text,
          };
        });
        const pendingCount = AppState.getPendingCount();
        return ok({ pendingCount, ddls: summary });
      }

      // === 天气 → 走联网搜索 ===
      case 'getWeather': {
        const city = String(args.city || AppState.device.city || '武汉').trim();
        // 异步搜索在 agent 中处理，此处返回占位
        return ok({ city, searching: true, note: '正在联网查询天气，请稍候' });
      }

      // === 联网搜索 ===
      case 'webSearch': {
        if (!args.query) return fail('搜索关键词不能为空');
        // 异步搜索在 agent 中处理
        return ok({ query: args.query, searching: true, note: '正在搜索，请稍候' });
      }

      default:
        return fail('未知工具: ' + toolName);
    }
  } catch (e) {
    return fail('工具执行失败: ' + e.message);
  }
}

/** 异步联网搜索（通过 Worker 代理） */
async function doWebSearch(query) {
  const workerUrl = getWorkerUrl();

  // 如果有 Worker URL，优先走 Worker 代理（无跨域限制）
  if (workerUrl) {
    try {
      const resp = await fetch(workerUrl + '/search?q=' + encodeURIComponent(query), {
        signal: AbortSignal.timeout(8000),
      });
      if (!resp.ok) throw new Error('HTTP ' + resp.status);
      const data = await resp.json();
      if (data.error) throw new Error(data.error);
      return ok({ query, results: data.results.slice(0, 5) });
    } catch (e) {
      console.warn('[Search] Worker 搜索失败，降级直连:', e.message);
    }
  }

  // 兜底：直连 DuckDuckGo Lite（浏览器端跨域通常被拦截，仅作尽力而为）
  try {
    const html = await (await fetch(
      'https://lite.duckduckgo.com/lite/?q=' + encodeURIComponent(query),
      { signal: AbortSignal.timeout(6000) }
    )).text();
    const results = parseDDGHTML(html);
    return ok({ query, results: results.slice(0, 5) });
  } catch (e2) {
    return fail('搜索功能需要部署 Cloudflare Worker。请在设置 → AI 助手设置中填入 Worker URL，或稍后重试。');
  }
}

function getWorkerUrl() {
  const setting = AgentSettings.load();
  return setting.workerUrl || '';
}

/** 简易 DuckDuckGo HTML 解析 */
function parseDDGHTML(html) {
  const results = [];
  const re = /<a\s[^>]*href="([^"]+)"[^>]*>([^<]+)<\/a>\s*<span[^>]*>([^<]*)<\/span>/gi;
  let match;
  while ((match = re.exec(html)) !== null && results.length < 8) {
    const url = match[1];
    const title = match[2].replace(/<[^>]+>/g, '').trim();
    const snippet = (match[3] || '').replace(/<[^>]+>/g, '').trim();
    if (title && !title.match(/Next|Previous|Home/i) && url.startsWith('http')) {
      results.push({ title, url, snippet });
    }
  }
  return results;
}

// 工具函数
function ok(data) { return { success: true, data }; }
function fail(error) { return { success: false, error }; }
