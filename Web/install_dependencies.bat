@echo off
chcp 65001 >nul
:: =========================================================
:: 智慧农业物联网可视化平台 - 依赖安装脚本
:: 版权所有 (c) 2023-2025 
:: 开发者: 黄浩、王伟
:: 联系方式: 2768607063@163.com
:: 未经授权，禁止复制、修改或分发本代码
:: =========================================================
echo 安装物联网可视化平台依赖...
echo.

echo 1. 安装前端依赖...
cd WebFront-enInterface\web
call npm install
cd ..\..\

echo.
echo 2. 安装后端Node.js依赖...
cd Severs
call npm install typescript ts-node express cors axios dotenv mysql2
cd ..

echo.
echo 3. 安装Python依赖...
pip install -r requirements.txt

echo.
echo 所有依赖安装完成！
echo.
echo 按任意键关闭此窗口...
pause > nul 