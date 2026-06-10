/* ==========================================================
   桌面智能灵动岛 — Cloudflare Worker · 搜索代理
   用途：绕过浏览器跨域限制，代理 DuckDuckGo 搜索
   部署：npx wrangler deploy 或复制粘贴到 CF Dashboard
   ========================================================== */

/// Worker 启动时间
const startTime = Date.now();

/// 根路径状态页 HTML
const STATUS_PAGE = `<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>桌面智能灵动岛 · 搜索代理</title>
<style>
  * { margin: 0; padding: 0; box-sizing: border-box; }
  body {
    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', 'PingFang SC', 'Microsoft YaHei', sans-serif;
    background: #FFFBF5; color: #44403C;
    display: flex; justify-content: center; align-items: center; min-height: 100vh;
  }
  .card {
    background: #FFF; border-radius: 16px; padding: 40px;
    box-shadow: 0 4px 24px rgba(249,115,22,0.1); max-width: 480px; width: 90%;
    text-align: center;
  }
  .card .icon { font-size: 48px; margin-bottom: 16px; }
  .card h1 { font-size: 20px; color: #F97316; margin-bottom: 8px; }
  .card p { font-size: 14px; color: #78716C; line-height: 1.8; }
  .card .status { display: inline-block; margin: 16px 0; padding: 6px 16px;
    background: #DCFCE7; color: #16A34A; border-radius: 20px; font-size: 13px; font-weight: 600; }
  .card code { background: #FEF3C7; padding: 2px 8px; border-radius: 4px;
    font-size: 13px; color: #EA580C; }
  .card .footer { margin-top: 24px; font-size: 12px; color: #A8A29E; }
  .card a { color: #F97316; text-decoration: none; }
</style>
</head>
<body>
  <div class="card">
    <div class="icon">🐕</div>
    <h1>桌面智能灵动岛 · 搜索代理</h1>
    <div class="status">✅ 运行中</div>
    <p>
      这是 Cloudflare Worker 后端服务<br>
      为前端页面提供 <strong>联网搜索</strong> 能力
    </p>
    <p style="margin-top:16px;">
      接口：<br>
      <code>GET /search?q=关键词</code> — 搜索并返回 JSON<br>
      <code>GET /health</code> — 健康检查<br>
      <code>GET /</code> — 本页面
    </p>
    <p class="footer">
      前端页面 →
      <a href="https://desktoppet.pages.dev">desktoppet.pages.dev</a>
    </p>
  </div>
</body>
</html>`;

export default {
  async fetch(request, env, ctx) {
    const url = new URL(request.url);

    // CORS 预检
    if (request.method === 'OPTIONS') {
      return corsResponse();
    }

    // 搜索接口
    if (url.pathname === '/search') {
      const query = url.searchParams.get('q');
      if (!query) return corsResponse(json({ error: '缺少 q 参数', query: query || '' }, 400));

      try {
        const results = await searchDDG(query);
        return corsResponse(json({ results, total: results.length }));
      } catch (e) {
        return corsResponse(json({ error: '搜索失败: ' + e.message }, 500));
      }
    }

    // 根路径 → 状态页
    if (url.pathname === '/' || url.pathname === '') {
      return corsResponse(new Response(STATUS_PAGE, {
        headers: { 'Content-Type': 'text/html; charset=utf-8' },
      }));
    }

    // 健康检查
    if (url.pathname === '/health') {
      return corsResponse(json({ status: 'ok', uptime: Date.now() - startTime }));
    }

    // 其他路由 → 404
    return corsResponse(new Response('404 Not Found — 可用接口: GET /  GET /health  GET /search?q=关键词', {
      status: 404,
      headers: { 'Content-Type': 'text/plain; charset=utf-8', 'Access-Control-Allow-Origin': '*' },
    }));
  },
};

/** 搜索 DuckDuckGo Lite 并解析结果 */
async function searchDDG(query) {
  const resp = await fetch(
    'https://lite.duckduckgo.com/lite/?' + new URLSearchParams({ q: query }),
    {
      headers: {
        'User-Agent': 'Mozilla/5.0 (compatible; DesktopPet/1.0)',
        'Accept': 'text/html',
      },
      signal: AbortSignal.timeout(8000),
    }
  );

  if (!resp.ok) throw new Error('DDG returned ' + resp.status);
  const html = await resp.text();
  return parseResults(html);
}

/** 简易 HTML 结果解析 */
function parseResults(html) {
  const results = [];
  // 匹配 DDG Lite 的搜索结果行：<a>链接</a><span>摘要</span>
  const re = /<a\s[^>]*href="([^"]+)"[^>]*>([^<]+)<\/a>\s*<span[^>]*class=['"][^'"]*snippet[^'"]*['"][^>]*>([^<]*)<\/span>/gi;
  const altRe = /<a\s[^>]*href="(https?:\/\/[^"]+)"[^>]*class=['"][^'"]*result-link[^'"]*['"][^>]*>([^<]+)<\/a>[\s\S]*?<td[^>]*class=['"][^'"]*result-snippet[^'"]*['"][^>]*>([^<]*)</gi;

  let match;
  // 先尝试标准格式
  while ((match = re.exec(html)) !== null && results.length < 8) {
    const url = decodeURIComponent(match[1]);
    const title = stripHtml(match[2]);
    const snippet = stripHtml(match[3] || '');
    if (isValidResult(url, title)) {
      results.push({ title, url, snippet });
    }
  }

  // 备用：通用 <a> + 后续文本
  if (results.length === 0) {
    const linkRe = /<a\s[^>]*href="(https?:\/\/[^"]+)"[^>]*>([^<]{2,})<\/a>/gi;
    while ((match = linkRe.exec(html)) !== null && results.length < 8) {
      const url = decodeURIComponent(match[1]);
      const title = stripHtml(match[2]);
      if (isValidResult(url, title)) {
        results.push({ title, url, snippet: '' });
      }
    }
  }

  return results;
}

function stripHtml(str) {
  return str.replace(/<[^>]+>/g, '').replace(/&amp;/g, '&').replace(/&lt;/g, '<')
    .replace(/&gt;/g, '>').replace(/&quot;/g, '"').replace(/&#39;/g, "'").trim();
}

function isValidResult(url, title) {
  if (!url.startsWith('http')) return false;
  if (!title || title.length < 2) return false;
  const noise = /^(Next|Previous|Home|Back|下一页|上一页|\d+\)$)/i;
  if (noise.test(title)) return false;
  return true;
}

// 工具
function corsResponse(body) {
  const resp = (typeof body === 'string') ? new Response(body)
             : (body instanceof Response) ? body
             : new Response(null, { status: 204 });
  resp.headers.set('Access-Control-Allow-Origin', '*');
  resp.headers.set('Access-Control-Allow-Methods', 'GET, OPTIONS');
  resp.headers.set('Access-Control-Allow-Headers', 'Content-Type');
  return resp;
}

function json(data, status) {
  return new Response(JSON.stringify(data), {
    status: status || 200,
    headers: { 'Content-Type': 'application/json' },
  });
}
