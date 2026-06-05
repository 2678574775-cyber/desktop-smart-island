# -*- coding: utf-8 -*-
"""验证 Agent 系统：工具定义、settings 页、对话无 API Key 降级"""
import sys, io, os
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
from playwright.sync_api import sync_playwright

base = os.path.dirname(os.path.abspath(__file__))
url = 'file:///' + os.path.join(base, 'index.html').replace('\\', '/')
scr = os.path.join(base, 'screenshots')

with sync_playwright() as p:
    browser = p.chromium.launch(headless=True)
    ctx = browser.new_context(viewport={'width': 1440, 'height': 900})
    page = ctx.new_page()
    page.on('pageerror', lambda err: print(f'[PAGE_ERROR] {err}'))
    page.on('console', lambda msg: print(f'  [console:{msg.type}] {msg.text[:100]}'))

    # Skip opening
    print('--- 1. Skip opening ---')
    page.goto(url, wait_until='networkidle')
    page.wait_for_timeout(500)
    skip = page.locator('#openingSkip')
    if skip.is_visible(): skip.click()
    page.wait_for_timeout(600)
    print('OK main visible:', page.locator('#appMain').is_visible())

    # Test chat without API key
    print('\n--- 2. Chat without API key ---')
    page.locator('[data-tab="chat"]').click()
    page.wait_for_timeout(300)
    page.fill('#chatInput', '你好')
    page.click('#chatSendBtn')
    page.wait_for_timeout(800)

    msgs = page.locator('.chat-msg')
    n = msgs.count()
    print(f'Messages: {n}')
    if n > 0:
        last_text = msgs.nth(n - 1).inner_text()
        print(f'Last msg: {last_text[:80]}')
        if 'API Key' in last_text or '设置' in last_text:
            print('OK No-API-key reminder works')
        else:
            print('WARN: Unexpected message content')

    # Test Agent settings page
    print('\n--- 3. Agent settings page ---')
    page.locator('[data-tab="settings"]').click()
    page.wait_for_timeout(300)
    page.screenshot(path=os.path.join(scr, 'agent-1-settings.png'))

    ai_link = page.locator('#tab-settings').get_by_text('AI 助手设置')
    if ai_link.count() > 0:
        ai_link.click()
        page.wait_for_timeout(500)
        page.screenshot(path=os.path.join(scr, 'agent-2-agent-page.png'))
        ag_page = page.locator('#tab-agent-settings')
        print('Agent page visible:', ag_page.is_visible())

        soul_editor = page.locator('#agentSoulEditor')
        if soul_editor.is_visible():
            print('Soul editor: OK, content len:', len(soul_editor.input_value()))

        api_key_input = page.locator('#agentApiKey')
        print('API key input visible:', api_key_input.is_visible())

        # Enter test key
        api_key_input.fill('sk-test-key-123')
        api_key_input.press('Tab')
        page.wait_for_timeout(500)

        print('Memory items:', page.locator('.memory-item').count())

        page.get_by_text('← 返回设置').click()
        page.wait_for_timeout(300)

    # Verify tool system
    print('\n--- 4. Tool system ---')
    r = page.evaluate('''() => {
        try {
            return JSON.stringify({
                toolsCount: INTERNAL_TOOLS.length,
                toolNames: INTERNAL_TOOLS.map(t => t.name),
                agentDefined: typeof Agent !== 'undefined',
                settingsDefined: typeof AgentSettings !== 'undefined',
                memoryDefined: typeof MemoryStore !== 'undefined',
                executeTool: typeof executeTool === 'function',
                apiKeyEmpty: AgentSettings.apiKey === '',
                testExecute: JSON.stringify(executeTool("listTasks", {filter:"pending",limit:2})),
            });
        } catch(e) { return JSON.stringify({error: e.message}); }
    }''')
    print(r[:300])

    page.screenshot(path=os.path.join(scr, 'agent-3-final.png'))
    browser.close()
    print('\n=== Done ===')
