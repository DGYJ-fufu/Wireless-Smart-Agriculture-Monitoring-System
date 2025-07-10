#!/bin/bash
# =========================================================
# 智慧农业物联网可视化平台 - 生产环境构建脚本 (Linux)
# 版权所有 (c) 2023-2025 
# 开发者: 黄浩、王伟
# 联系方式: 2768607063@163.com
# 未经授权，禁止复制、修改或分发本代码
# =========================================================
echo "开始构建智慧农业物联网可视化平台生产环境..."
echo

echo "1. 构建前端应用..."
cd WebFront-enInterface/web
npm install
npm run build
cd ../..
echo "前端构建完成！"
echo

echo "2. 准备后端服务器..."
cd Severs
npm install
npx tsc -p tsconfig.json
cd ..
echo "后端服务器准备完成！"
echo

echo "3. 创建部署目录..."
mkdir -p dist/api
mkdir -p dist/control
echo "部署目录创建完成！"
echo

echo "4. 复制文件到部署目录..."
cp -r WebFront-enInterface/web/dist/* dist/
cp -r Severs/*.js dist/api/
cp Severs/SendCmdServer.py dist/control/
cp Severs/bottle.py dist/control/
echo "文件复制完成！"
echo

echo "5. 创建启动脚本..."
cat > dist/start.sh << 'EOL'
#!/bin/bash
# 智慧农业物联网可视化平台启动脚本
echo "启动智慧农业物联网可视化平台..."
echo

echo "1. 启动API服务器..."
cd api
node server.js &
API_PID=$!
cd ..

echo "2. 启动控制服务器..."
cd control
python SendCmdServer.py &
CONTROL_PID=$!
cd ..

echo
echo "服务已启动！"
echo "API服务器: http://localhost:5555"
echo "应用访问: http://localhost:5555"
echo
echo "按Ctrl+C停止服务"

# 保存进程ID
echo $API_PID > .api.pid
echo $CONTROL_PID > .control.pid

# 等待用户中断
trap 'kill $API_PID $CONTROL_PID; rm .api.pid .control.pid; echo; echo "服务已停止"; exit 0' INT
wait
EOL

# 创建停止脚本
cat > dist/stop.sh << 'EOL'
#!/bin/bash
# 智慧农业物联网可视化平台停止脚本
if [ -f .api.pid ]; then
  kill $(cat .api.pid) 2>/dev/null
  rm .api.pid
fi

if [ -f .control.pid ]; then
  kill $(cat .control.pid) 2>/dev/null
  rm .control.pid
fi

echo "服务已停止"
EOL

# 设置执行权限
chmod +x dist/start.sh
chmod +x dist/stop.sh
echo "启动脚本创建完成！"
echo

echo "6. 复制环境变量配置示例..."
cp deploy/env.example dist/.env
echo "环境变量配置示例已复制！"
echo

echo "7. 创建README文件..."
cat > dist/README.md << 'EOL'
# 智慧农业物联网可视化平台

## 部署说明

1. 确保已安装Node.js和Python
2. 配置.env文件中的数据库连接信息
3. 运行以下命令启动应用:
   ```
   chmod +x start.sh stop.sh
   ./start.sh
   ```
4. 停止应用:
   ```
   ./stop.sh
   ```

## 版权信息

版权所有 (c) 2023-2025 
开发团队：黄浩、王伟
联系方式：2768607063@163.com
EOL
echo "README文件创建完成！"
echo

echo "构建完成！部署文件位于dist目录" 