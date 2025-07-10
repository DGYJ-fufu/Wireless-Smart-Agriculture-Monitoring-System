@echo off
chcp 65001 >nul
:: =========================================================
:: 智慧农业物联网可视化平台 - 生产环境构建脚本
:: 版权所有 (c) 2023-2025 
:: 开发者: 黄浩、王伟
:: 联系方式: 2768607063@163.com
:: 未经授权，禁止复制、修改或分发本代码
:: =========================================================
echo 开始构建智慧农业物联网可视化平台生产环境...
echo.

echo 1. 构建前端应用...
cd WebFront-enInterface\web
call npm install
call npm run build
cd ..\..
echo 前端构建完成！
echo.

echo 2. 准备后端服务器...
cd Severs
call npm install
call tsc -p tsconfig.json
cd ..
echo 后端服务器准备完成！
echo.

echo 3. 创建部署目录...
if not exist "dist" mkdir dist
if not exist "dist\api" mkdir dist\api
if not exist "dist\control" mkdir dist\control
echo 部署目录创建完成！
echo.

echo 4. 复制文件到部署目录...
xcopy /E /Y WebFront-enInterface\web\dist\* dist\
xcopy /E /Y Severs\*.js dist\api\
copy Severs\SendCmdServer.py dist\control\
copy Severs\bottle.py dist\control\
echo 文件复制完成！
echo.

echo 5. 创建启动脚本...
echo @echo off > dist\start.bat
echo chcp 65001 ^>nul >> dist\start.bat
echo echo 启动智慧农业物联网可视化平台... >> dist\start.bat
echo echo. >> dist\start.bat
echo echo 1. 启动API服务器... >> dist\start.bat
echo start cmd /k "cd api && node server.js" >> dist\start.bat
echo echo 2. 启动控制服务器... >> dist\start.bat
echo start cmd /k "cd control && python SendCmdServer.py" >> dist\start.bat
echo echo. >> dist\start.bat
echo echo 服务已启动！ >> dist\start.bat
echo echo API服务器: http://localhost:5555 >> dist\start.bat
echo echo 应用访问: http://localhost:5555 >> dist\start.bat
echo echo. >> dist\start.bat
echo echo 按任意键关闭此窗口... >> dist\start.bat
echo pause ^> nul >> dist\start.bat
echo 启动脚本创建完成！
echo.

echo 6. 复制环境变量配置示例...
copy deploy\env.example dist\.env
echo 环境变量配置示例已复制！
echo.

echo 7. 创建README文件...
echo # 智慧农业物联网可视化平台 > dist\README.md
echo. >> dist\README.md
echo ## 部署说明 >> dist\README.md
echo. >> dist\README.md
echo 1. 确保已安装Node.js和Python >> dist\README.md
echo 2. 配置.env文件中的数据库连接信息 >> dist\README.md
echo 3. 运行start.bat启动应用 >> dist\README.md
echo. >> dist\README.md
echo ## 版权信息 >> dist\README.md
echo. >> dist\README.md
echo 版权所有 (c) 2023-2025  >> dist\README.md
echo 开发团队：黄浩、王伟 >> dist\README.md
echo 联系方式：2768607063@163.com >> dist\README.md
echo README文件创建完成！
echo.

echo 构建完成！部署文件位于dist目录
echo.
echo 按任意键关闭此窗口...
pause > nul 