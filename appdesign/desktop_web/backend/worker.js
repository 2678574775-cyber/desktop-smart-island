/* ==========================================================
   桌面智能灵动岛 — Cloudflare Worker · 搜索代理
   用途：绕过浏览器跨域限制，代理 DuckDuckGo 搜索
   部署：npx wrangler deploy 或复制粘贴到 CF Dashboard
   ========================================================== */

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
      if (!query) return json({ error: '缺少 q 参数' }, 400);

      try {
        const results = await searchDDG(query);
        return corsResponse(json({ results }));
      } catch (e) {
        return corsResponse(json({ error: '搜索失败: ' + e.message }), 500);
      }
    }

    // 其他路由 → 404
    return corsResponse(json({ error: 'Not found' }, 404));
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
  const resp = typeof body === 'string' ? new Response(body) : body;
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
