@echo off
chcp 65001 >nul
:: =========================================================
:: 智慧农业物联网可视化平台 - 部署测试脚本
:: 版权所有 (c) 2023-2025 
:: 开发者: 黄浩、王伟
:: 联系方式: 2768607063@163.com
:: 未经授权，禁止复制、修改或分发本代码
:: =========================================================
echo 开始测试智慧农业物联网可视化平台部署...
echo.

echo 1. 检查部署目录...
if not exist "dist" (
    echo [错误] 部署目录不存在，请先执行build.bat脚本
    goto :error
) else (
    echo [成功] 部署目录存在
)
echo.

echo 2. 检查前端文件...
if not exist "dist\index.html" (
    echo [错误] 前端文件未正确部署
    goto :error
) else (
    echo [成功] 前端文件已正确部署
)
echo.

echo 3. 检查API服务器文件...
if not exist "dist\api\server.js" (
    echo [错误] API服务器文件未正确部署
    goto :error
) else (
    echo [成功] API服务器文件已正确部署
)
echo.

echo 4. 检查控制服务器文件...
if not exist "dist\control\SendCmdServer.py" (
    echo [错误] 控制服务器文件未正确部署
    goto :error
) else (
    echo [成功] 控制服务器文件已正确部署
)
echo.

echo 5. 检查启动脚本...
if not exist "dist\start.bat" (
    echo [错误] 启动脚本未正确创建
    goto :error
) else (
    echo [成功] 启动脚本已正确创建
)
echo.

echo 6. 检查环境变量配置...
if not exist "dist\.env" (
    echo [警告] 环境变量配置文件不存在，将创建默认配置
    copy deploy\env.example dist\.env
) else (
    echo [成功] 环境变量配置文件已存在
)
echo.

echo 7. 测试API服务器启动...
echo 正在尝试启动API服务器，请稍候...
cd dist\api
start /B cmd /c "node server.js > ..\api_test.log 2>&1"
timeout /t 5 /nobreak > nul
findstr /C:"服务器运行在" ..\api_test.log > nul
if %errorlevel% neq 0 (
    echo [错误] API服务器启动失败，请检查日志文件: dist\api_test.log
    taskkill /F /IM node.exe > nul 2>&1
) else (
    echo [成功] API服务器可以正常启动
    taskkill /F /IM node.exe > nul 2>&1
)
cd ..\..
echo.

echo 8. 测试Python环境...
python --version > nul 2>&1
if %errorlevel% neq 0 (
    echo [错误] Python未安装或未添加到PATH
    goto :error
) else (
    echo [成功] Python环境正常
)
echo.

echo 部署测试完成！
echo.
echo 如果所有测试都通过，您可以运行dist\start.bat启动应用
echo 如果有任何错误，请查看相应的错误信息并修复问题
goto :end

:error
echo.
echo [错误] 部署测试失败，请修复上述问题后重试
echo.

:end
echo.
echo 按任意键退出...
pause > nul 