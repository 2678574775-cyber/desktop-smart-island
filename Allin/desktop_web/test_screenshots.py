# -*- coding: utf-8 -*-
"""验证网页版各 Tab 页面渲染效果，截图保存"""
import sys, io, os
# 强制 UTF-8 输出（避免 Windows GBK 编码错误）
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

from playwright.sync_api import sync_playwright

base = r'D:\program\desktopdog\appdesign\desktop_web'
url = 'file:///' + os.path.join(base, 'index.html').replace('\\', '/')
screenshots_dir = os.path.join(base, 'screenshots')

with sync_playwright() as p:
    browser = p.chromium.launch(headless=True)
    context = browser.new_context(viewport={'width': 1440, 'height': 900})
    page = context.new_page()

    # 捕获 console 和 error
    page.on('console', lambda msg: print(f'  [console:{msg.type}] {msg.text}'))
    page.on('pageerror', lambda err: print(f'  [PAGE ERROR] {err}'))

    # 1. 加载主页
    print('--- 1. Loading Home tab ---')
    page.goto(url, wait_until='networkidle')
    page.wait_for_timeout(1000)
    page.screenshot(path=os.path.join(screenshots_dir, '1-home.png'), full_page=False)
    print('OK 1-home.png saved')

    # 检查关键元素
    for sel in ['#petContainer', '#petStatusLabel', '.indicator-row', '.device-info-card']:
        el = page.locator(sel)
        status = 'OK' if el.is_visible() else 'MISSING'
        print(f'  {sel}: {status}')

    # DDL
    ddls = page.locator('.ddl-item')
    print(f'  DDL items: {ddls.count()}')
    # 快捷操作
    qas = page.locator('.quick-action-card')
    print(f'  Quick actions: {qas.count()}')

    # 2. 切换到任务 Tab
    print('\n--- 2. Switching to Tasks tab ---')
    page.locator('.tab-btn[data-tab="tasks"]').click()
    page.wait_for_timeout(500)
    page.screenshot(path=os.path.join(screenshots_dir, '2-tasks.png'), full_page=False)
    print('OK 2-tasks.png saved')

    tasks = page.locator('.task-item')
    print(f'  Task items: {tasks.count()}')

    # 测试筛选
    page.locator('.filter-btn[data-filter="done"]').click()
    page.wait_for_timeout(300)
    done_tasks = page.locator('.task-item.done')
    print(f'  Done tasks: {done_tasks.count()}')
    page.locator('.filter-btn[data-filter="all"]').click()

    # 3. 切换到对话 Tab
    print('\n--- 3. Switching to Chat tab ---')
    page.locator('.tab-btn[data-tab="chat"]').click()
    page.wait_for_timeout(500)
    page.screenshot(path=os.path.join(screenshots_dir, '3-chat.png'), full_page=False)
    print('OK 3-chat.png saved')

    msgs = page.locator('.chat-msg')
    print(f'  Messages: {msgs.count()}')
    input_el = page.locator('#chatInput')
    print(f'  Input visible: {input_el.is_visible()}')

    # 测试发送消息
    input_el.fill('Test message')
    page.locator('#chatSendBtn').click()
    page.wait_for_timeout(200)
    thinking = page.locator('#thinking-indicator')
    print(f'  Thinking indicator: {thinking.is_visible()}')
    # 等待 AI 回复
    page.wait_for_timeout(2500)
    msgs2 = page.locator('.chat-msg')
    print(f'  Messages after send: {msgs2.count()}')

    # 4. 切换到设置 Tab
    print('\n--- 4. Switching to Settings tab ---')
    page.locator('.tab-btn[data-tab="settings"]').click()
    page.wait_for_timeout(500)
    page.screenshot(path=os.path.join(screenshots_dir, '4-settings.png'), full_page=False)
    print('OK 4-settings.png saved')

    device_card = page.locator('.settings-device-card')
    print(f'  Device card: {device_card.is_visible()}')
    toggles = page.locator('.toggle')
    print(f'  Toggle switches: {toggles.count()}')

    # 5. 测试任务弹窗
    print('\n--- 5. Testing task modal ---')
    page.locator('.tab-btn[data-tab="tasks"]').click()
    page.wait_for_timeout(300)
    page.locator('.btn-add').click()
    page.wait_for_timeout(500)
    modal = page.locator('#taskModal')
    print(f'  Modal open: {modal.is_visible()}')
    if modal.is_visible():
        page.screenshot(path=os.path.join(screenshots_dir, '5-modal.png'), full_page=False)
        print('OK 5-modal.png saved')
        page.locator('.modal-close').click()
        page.wait_for_timeout(300)

    # 6. 测试编辑
    print('\n--- 6. Testing task edit ---')
    # 点击第一个任务项触发编辑（使用编辑按钮）
    editBtn = page.locator('.task-action-btn.edit').first()
    if editBtn.is_visible():
        editBtn.click()
        page.wait_for_timeout(500)
        print(f'  Edit modal: {page.locator("#taskModal").is_visible()}')
        page.locator('.modal-close').click()

    # 7. 完整页面截图
    print('\n--- 7. Full page screenshot ---')
    page.screenshot(path=os.path.join(screenshots_dir, '0-full-page.png'), full_page=True)
    print('OK 0-full-page.png saved')

    browser.close()
    print('\n=== All done! Screenshots saved to screenshots/ ===')
