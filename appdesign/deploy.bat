@echo off
chcp 65001 >nul
echo.
echo ╔══════════════════════════════════════════╗
echo ║  桌面智能灵动岛 — Cloudflare 部署脚本  ║
echo ╚══════════════════════════════════════════╝
echo.

:menu
echo ┌──────────────────────────────────────────┐
echo │  1. 安装 Wrangler (npm i -g wrangler)   │
echo │  2. 登录 Cloudflare (wrangler login)     │
echo │  3. 部署后端 Worker (搜索代理)           │
echo │  4. 部署前端 Pages (静态站点)            │
echo │  5. 全部部署 (Worker → Pages)            │
echo │  0. 退出                                  │
echo └──────────────────────────────────────────┘
echo.
set /p choice="选择操作 (0-5): "

if "%choice%"=="1" goto install
if "%choice%"=="2" goto login
if "%choice%"=="3" goto deploy_worker
if "%choice%"=="4" goto deploy_pages
if "%choice%"=="5" goto deploy_all
if "%choice%"=="0" goto end
echo 无效选择，请重试
goto menu

:install
echo.
echo [1/1] 安装 Wrangler CLI...
call npm install -g wrangler
if %errorlevel% neq 0 (
    echo ❌ 安装失败，尝试用 npx 方式（无需全局安装）
    echo    后续命令将使用 npx wrangler 代替
)
echo ✅ 完成
goto menu

:login
echo.
echo [1/1] 登录 Cloudflare...
echo 浏览器将打开 Cloudflare 授权页面，请登录你的账号
echo.
npx wrangler login
if %errorlevel% neq 0 (
    echo ❌ 登录失败，请检查网络或手动运行: npx wrangler login
)
echo ✅ 完成
goto menu

:deploy_worker
echo.
echo ═══════════════════════════════════════
echo  部署后端 Worker (搜索代理)
echo ═══════════════════════════════════════
echo.
cd /d "%~dp0desktop_web\backend"

echo [1/2] 检查 wrangler.toml...
if not exist wrangler.toml (
    echo ❌ 缺少 wrangler.toml，请确保在正确目录
    goto menu
)

echo [2/2] 部署到 Cloudflare Workers...
npx wrangler deploy
if %errorlevel% neq 0 (
    echo ❌ 部署失败
    goto menu
)

echo.
echo ╔══════════════════════════════════════════╗
echo ║  ✅ Worker 部署成功！                    ║
echo ║                                         ║
echo ║  URL: https://desktoppet-search.xxx     ║
echo ║       .workers.dev                      ║
echo ║                                         ║
echo ║  ⚠ 复制这个 URL，下一步配置前端时需要   ║
echo ╚══════════════════════════════════════════╝
echo.
cd /d "%~dp0"
goto menu

:deploy_pages
echo.
echo ═══════════════════════════════════════════
echo  部署前端 Pages (静态站点)
echo ═══════════════════════════════════════════
echo.
echo [1/2] 发布到 Cloudflare Pages...
echo   项目目录: desktop_web\
echo.
cd /d "%~dp0"
npx wrangler pages deploy desktop_web --project-name=desktoppet
if %errorlevel% neq 0 (
    echo.
    echo ⚠ 如果上面提示需要创建项目，请运行:
    echo   npx wrangler pages project create desktoppet
    echo   然后再重试部署
    goto menu
)

echo.
echo ╔══════════════════════════════════════════╗
echo ║  ✅ 前端部署成功！                       ║
echo ║                                         ║
echo ║  URL: https://desktoppet.pages.dev      ║
echo ║                                         ║
echo ║  ⚠ 部署后，登录网页 → 设置 →            ║
echo ║    AI 助手设置 → 填入 Worker URL         ║
echo ║    保存后，联网搜索功能即可使用           ║
echo ╚══════════════════════════════════════════╝
echo.
goto menu

:deploy_all
echo.
echo ═══════════════════════════════════════════
echo  一键部署: Worker + Pages
echo ═══════════════════════════════════════════

call :deploy_worker_inline
if %deploy_ok%==0 (
    echo ⚠ Worker 部署跳过，继续部署前端...
)

call :deploy_pages_inline

echo.
echo ╔══════════════════════════════════════════╗
echo ║  🎉 部署全部完成！                      ║
echo ║                                         ║
echo ║  前端: https://desktoppet.pages.dev     ║
echo ║  Worker: https://desktoppet-search       ║
echo ║          .xxx.workers.dev               ║
echo ║                                         ║
echo ║  下一步：打开前端页面 → 设置 →           ║
echo ║  AI助手设置 → 填入 Worker URL → 保存    ║
echo ╚══════════════════════════════════════════╝
goto menu

:deploy_worker_inline
cd /d "%~dp0desktop_web\backend"
npx wrangler deploy
set deploy_ok=%errorlevel%
cd /d "%~dp0"
goto :eof

:deploy_pages_inline
cd /d "%~dp0"
npx wrangler pages deploy desktop_web --project-name=desktoppet
goto :eof

:end
echo 再见~
pause
