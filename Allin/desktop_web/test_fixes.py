# -*- coding: utf-8 -*-
"""验证：冷却限制 + 删除确认"""
import sys, io, os
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
from playwright.sync_api import sync_playwright

base = os.path.dirname(os.path.abspath(__file__))
url = 'file:///' + os.path.join(base, 'index.html').replace('\\', '/')

with sync_playwright() as p:
    browser = p.chromium.launch(headless=True)
    page = browser.new_page(viewport={'width': 1440, 'height': 900})
    errors = []
    page.on('pageerror', lambda err: errors.append(str(err)))

    page.goto(url, wait_until='networkidle')
    page.wait_for_timeout(500)
    skip = page.locator('#openingSkip')
    if skip.is_visible(): skip.click()
    page.wait_for_timeout(500)

    # 1. 测试 deleteTask 确认机制
    print('--- 1. deleteTask confirmation ---')
    r = page.evaluate('''() => {
        // 首次调用 without confirmed → 应返回 needConfirm
        const r1 = JSON.parse(JSON.stringify(executeTool("deleteTask", {taskId: "t1"})));
        // 强制确认调用
        const r2 = JSON.parse(JSON.stringify(executeTool("deleteTask", {taskId: "t1", confirmed: true})));
        return JSON.stringify({r1, r2});
    }''')
    print(r[:300])
    assert '"needConfirm\":true' in r, 'First call should return needConfirm'
    assert '"deleted\":true' in r, 'Second call should delete'
    print('OK delete confirmation works')

    # 2. 测试冷却限制
    print('\n--- 2. Rate limit cooldown ---')
    page.locator('[data-tab="chat"]').click()
    page.wait_for_timeout(200)
    page.fill('#chatInput', 'msg1')
    page.click('#chatSendBtn')
    page.wait_for_timeout(200)

    # 立即再发 → 应被拦截
    page.fill('#chatInput', 'msg2')
    page.click('#chatSendBtn')
    page.wait_for_timeout(400)

    # 检查 Toast 是否出现
    toasts = page.locator('.toast')
    toast_msgs = [toasts.nth(i).inner_text() for i in range(toasts.count())]
    print(f'Toast messages: {toast_msgs}')

    cooldown_hit = any('秒' in t for t in toast_msgs)
    print(f'Cooldown triggered: {cooldown_hit}')
    print('OK rate limit active' if cooldown_hit else 'WARN: cooldown not triggered')

    browser.close()
    print('\n=== Done ===')
