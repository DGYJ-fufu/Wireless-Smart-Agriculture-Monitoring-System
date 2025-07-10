# 智慧农业物联网可视化平台 - Docker配置
# 版权所有 (c) 2023-2025 
# 开发者: 黄浩、王伟
# 联系方式: 2768607063@163.com

# 使用Node.js官方镜像作为基础镜像
FROM node:18-slim

# 安装Python和必要依赖
RUN apt-get update && apt-get install -y \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# 设置工作目录
WORKDIR /app

# 复制部署文件
COPY dist/ ./

# 安装Python依赖
COPY requirements.txt ./
RUN pip3 install --no-cache-dir -r requirements.txt

# 设置环境变量
ENV NODE_ENV=production
ENV PORT=5555
ENV CONTROL_API_URL=http://localhost:8080

# 暴露端口
EXPOSE 5555 8080

# 创建启动脚本
RUN echo '#!/bin/bash\n\
echo "启动智慧农业物联网可视化平台..."\n\
cd /app/api && node server.js &\n\
cd /app/control && python3 SendCmdServer.py\n\
' > /app/docker-start.sh && chmod +x /app/docker-start.sh

# 启动应用
CMD ["/app/docker-start.sh"] 