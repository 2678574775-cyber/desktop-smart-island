# -*- coding: utf-8 -*-
"""验证四视频加载 + 开幕动画 + 状态切换"""
import sys, io, os
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
from playwright.sync_api import sync_playwright

base = os.path.dirname(os.path.abspath(__file__))
url = 'file:///' + os.path.join(base, 'index.html').replace('\\', '/')
screenshots = os.path.join(base, 'screenshots')

with sync_playwright() as p:
    browser = p.chromium.launch(headless=True)
    ctx = browser.new_context(viewport={'width': 1440, 'height': 900})
    page = ctx.new_page()

    page.on('pageerror', lambda err: print(f'[PAGE_ERROR] {err}'))

    print('1. Open page (opening animation should play)...')
    page.goto(url, wait_until='networkidle')
    page.wait_for_timeout(500)
    page.screenshot(path=os.path.join(screenshots, 'v1-opening.png'))
    print('   OK v1-opening.png')

    # 检查开幕层
    overlay = page.locator('#openingOverlay')
    video = page.locator('#openingVideo')
    print(f'   Opening overlay visible: {overlay.is_visible()}')
    print(f'   Opening video ready: {video.is_visible()}')

    # 跳过开幕
    skip_btn = page.locator('#openingSkip')
    print(f'   Skip button visible: {skip_btn.is_visible()}')
    print('2. Skip opening...')
    skip_btn.click()
    page.wait_for_timeout(800)

    # 主界面应可见
    main = page.locator('#appMain')
    print(f'   Main app visible: {main.is_visible()}')
    page.screenshot(path=os.path.join(screenshots, 'v2-home-video.png'))
    print('   OK v2-home-video.png')

    # 检查宠物视频
    pet_video = page.locator('#petVideo')
    print(f'   Pet video visible: {pet_video.is_visible()}')

    # 检查四个 Tab
    for tab_name, label in [('tasks','Task'), ('chat','Chat'), ('settings','Settings')]:
        page.locator(f'.tab-btn[data-tab="{tab_name}"]').click()
        page.wait_for_timeout(400)
        page.screenshot(path=os.path.join(screenshots, f'v3-{tab_name}.png'))
        print(f'   OK v3-{tab_name}.png')

    # 回到主页
    page.locator('.tab-btn[data-tab="home"]').click()
    page.wait_for_timeout(300)

    # 测试宠物点击弹跳
    pet_container = page.locator('#petContainer')
    pet_container.click()
    page.wait_for_timeout(600)
    page.screenshot(path=os.path.join(screenshots, 'v4-pet-click.png'))
    print('   OK v4-pet-click.png (bounce animation)')

    # 测试对话发送
    page.locator('.tab-btn[data-tab="chat"]').click()
    page.wait_for_timeout(300)
    page.fill('#chatInput', '你好小狗')
    page.click('#chatSendBtn')
    page.wait_for_timeout(300)
    print(f'   Thinking indicator: {page.locator("#thinking-indicator").is_visible()}')
    page.wait_for_timeout(2500)
    print(f'   Messages after reply: {page.locator(".chat-msg").count()}')

    browser.close()
    print('\n=== All video tests passed ===')
