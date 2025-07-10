# 智慧农业物联网可视化平台 - 部署工具

## 版权信息

版权所有 (c) 2023-2025   
开发团队：黄浩、王伟  
联系方式：2768607063@163.com  
未经授权，禁止复制、修改或分发本代码

## 部署工具说明

本目录包含智慧农业物联网可视化平台的部署相关工具和配置文件。

### 文件说明

- `build.bat` - Windows环境构建脚本
- `build.sh` - Linux环境构建脚本
- `test-deploy.bat` - 部署测试脚本
- `Dockerfile` - Docker镜像构建配置
- `docker-compose.yml` - Docker Compose配置
- `env.example` - 环境变量配置示例
- `DEPLOY.md` - 详细部署文档

## 快速开始

### Windows环境

1. 执行构建脚本
   ```
   build.bat
   ```

2. 测试部署是否成功
   ```
   test-deploy.bat
   ```

3. 启动应用
   ```
   cd ../dist
   start.bat
   ```

### Linux环境

1. 设置脚本执行权限
   ```bash
   chmod +x build.sh
   ```

2. 执行构建脚本
   ```bash
   ./build.sh
   ```

3. 启动应用
   ```bash
   cd ../dist
   chmod +x start.sh
   ./start.sh
   ```

### Docker环境

1. 构建并启动容器
   ```bash
   docker-compose -f docker-compose.yml up -d
   ```

## 详细部署指南

请参阅 [DEPLOY.md](DEPLOY.md) 获取详细的部署指南。

## 常见问题

1. **构建失败**
   - 检查Node.js和Python是否正确安装
   - 检查网络连接是否正常
   - 检查项目依赖是否完整

2. **应用启动失败**
   - 检查数据库配置是否正确
   - 检查端口是否被占用
   - 查看日志文件获取详细错误信息

3. **Docker部署问题**
   - 确保Docker和Docker Compose已正确安装
   - 检查Docker服务是否正在运行
   - 检查容器日志获取详细错误信息

## 联系支持

如有任何问题，请联系:
- 邮箱: 2768607063@163.com
- 开发团队: 黄浩、王伟 