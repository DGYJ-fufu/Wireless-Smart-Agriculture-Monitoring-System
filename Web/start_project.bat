@echo off
chcp 65001 >nul
:: =========================================================
:: 智慧农业物联网可视化平台 - 项目启动脚本
:: 版权所有 (c) 2023-2025 
:: 开发者: 黄浩、王伟
:: 联系方式: 2768607063@163.com
:: 未经授权，禁止复制、修改或分发本代码
:: =========================================================
echo 启动物联网可视化平台...
echo.

echo 1. 启动Node.js后端服务器...
start cmd /k "cd Severs && npx ts-node server.ts"

echo 2. 启动Python控制服务器...
start cmd /k "cd Severs && python SendCmdServer.py"

echo 3. 启动React前端...
start cmd /k "cd WebFront-enInterface\web && npm run dev"

echo.
echo 所有服务已启动！
echo 后端服务器: http://localhost:5555
echo 控制服务器: http://localhost:8080
echo 前端应用: http://localhost:5173
echo.
echo 按任意键关闭此窗口...
pause > nul 