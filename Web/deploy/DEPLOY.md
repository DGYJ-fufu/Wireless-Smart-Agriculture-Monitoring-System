# 智慧农业物联网可视化平台部署文档

## 版权信息

版权所有 (c) 2023-2025   
开发团队：黄浩、王伟  
联系方式：2768607063@163.com  
未经授权，禁止复制、修改或分发本代码

## 系统要求

- **Node.js**: v16.0.0 或更高版本
- **Python**: v3.6 或更高版本
- **MySQL**: v5.7 或更高版本
- **操作系统**: Windows 10/11 或 Linux (Ubuntu 20.04+/CentOS 8+)

## 部署步骤

### 1. 准备工作

1. 安装所需软件:
   - Node.js: [https://nodejs.org/](https://nodejs.org/)
   - Python: [https://www.python.org/downloads/](https://www.python.org/downloads/)
   - MySQL: [https://www.mysql.com/downloads/](https://www.mysql.com/downloads/)

2. 创建数据库:
   ```sql
   CREATE DATABASE agriculture_iot CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
   ```

3. 导入数据库结构和初始数据（请联系开发者获取SQL文件）

### 2. 构建项目

#### Windows 环境

1. 打开命令提示符或PowerShell
2. 进入项目根目录
3. 执行构建脚本:
   ```
   deploy\build.bat
   ```

#### Linux 环境

1. 打开终端
2. 进入项目根目录
3. 设置脚本执行权限:
   ```bash
   chmod +x deploy/build.sh
   ```
4. 执行构建脚本:
   ```bash
   ./deploy/build.sh
   ```

### 3. 配置环境变量

构建完成后，编辑`dist/.env`文件，配置以下参数:

```
# 数据库配置
DB_HOST=你的数据库主机地址
DB_USER=你的数据库用户名
DB_PASSWORD=你的数据库密码
DB_NAME=agriculture_iot

# 服务器配置
PORT=5555
NODE_ENV=production

# 控制API配置
CONTROL_API_URL=http://localhost:8080
```

### 4. 启动应用

#### Windows 环境

双击`dist/start.bat`文件

#### Linux 环境

```bash
cd dist
chmod +x start.sh stop.sh
./start.sh
```

### 5. 访问应用

打开浏览器，访问:
- 应用界面: `http://服务器IP:5555`

## 服务器部署

### Nginx配置示例

```nginx
server {
    listen 80;
    server_name your-domain.com;

    location / {
        proxy_pass http://localhost:5555;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection 'upgrade';
        proxy_set_header Host $host;
        proxy_cache_bypass $http_upgrade;
    }

    location /control/ {
        proxy_pass http://localhost:8080/;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection 'upgrade';
        proxy_set_header Host $host;
        proxy_cache_bypass $http_upgrade;
    }
}
```

### PM2配置示例（用于生产环境进程管理）

1. 安装PM2:
   ```bash
   npm install -g pm2
   ```

2. 创建配置文件`ecosystem.config.js`:
   ```javascript
   module.exports = {
     apps: [
       {
         name: 'agriculture-iot-api',
         script: 'api/server.js',
         cwd: './dist',
         env: {
           NODE_ENV: 'production',
         },
         watch: false,
         instances: 1,
         exec_mode: 'fork',
       }
     ]
   };
   ```

3. 启动服务:
   ```bash
   pm2 start ecosystem.config.js
   ```

4. 为Python控制服务器创建启动脚本:
   ```bash
   cd dist/control
   pm2 start "python SendCmdServer.py" --name "agriculture-iot-control"
   ```

5. 保存PM2配置:
   ```bash
   pm2 save
   ```

## 故障排除

1. 如果无法连接到数据库:
   - 检查.env文件中的数据库配置
   - 确认MySQL服务是否运行
   - 检查防火墙设置

2. 如果前端无法加载:
   - 检查浏览器控制台是否有错误
   - 确认API服务器是否正常运行

3. 如果设备控制不起作用:
   - 检查控制服务器是否正常运行
   - 确认设备连接是否正常

## 联系支持

如有任何问题，请联系:
- 邮箱: 2768607063@163.com
- 开发团队: 黄浩、王伟 