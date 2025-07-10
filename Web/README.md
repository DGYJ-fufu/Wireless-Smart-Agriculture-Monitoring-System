# 模板物联网可视化平台 V1.0

Template IoT Visualization Platform V1.0

---

## 项目简介 / Project Overview

该项目旨在提供一个快速搭建的物联网数据可视化平台示例，整合了前后端、部署脚本与示例设备控制逻辑，方便开发者在此基础上进行二次开发或教学示范。

This repository contains a full-stack example of an Internet-of-Things (IoT) visualization platform. It includes a React/TypeScript front-end, Bottle/Python back-end APIs, deployment scripts (Docker & shell/batch), and sample device-control services. Feel free to fork and adapt it for your own needs.

## 目录结构 / Directory Structure

```text
├── deploy/              # Docker & build scripts
├── Severs/              # Backend services (TypeScript & Python)
├── WebFront-enInterface/# Front-end projects (React/Vite)
├── start_project.bat    # One-click startup helper (Windows)
├── install_dependencies.bat # Dependency installer (Windows)
├── requirements.txt     # Python dependencies
└── README.md            # Project documentation (this file)
```

> 更多详细信息请参见各子目录的 `README.md`。

## 快速开始 / Quick Start

### 1. 克隆仓库 / Clone the repo

```bash
git clone <repo-url>
cd 模板物联网可视化平台V1.0
```

### 2. 安装依赖 / Install dependencies

#### Windows

```powershell
./install_dependencies.bat
```

#### Linux / macOS

```bash
pip install -r requirements.txt
cd WebFront-enInterface/web
npm install
```

### 3. 运行项目 / Run the project

```powershell
# Start backend (Python Bottle and Node.js)
cd Severs
python SendCmdServer.py
npx ts-node server.ts
# In a new terminal: start frontend
cd WebFront-enInterface/web
npm run dev
```

访问浏览器 `http://localhost:5173` 查看前端界面。

### 4. 使用 Docker / Use Docker

项目提供了 `deploy/docker-compose.yml` 以便一键部署全部服务：

```bash
cd deploy
docker-compose up --build -d
```

## 开发者 / Authors

本项目由 **景德镇艺术职业大学** 的 **黄浩、王伟** 开发完成。

Developed by Huang Hao & Wang Wei, Jingdezhen Vocational University of Arts.

## 联系方式 / Contact

如有问题或建议，请发送邮件至：<a2768607063@163.com>

## 许可证 / License

Unless specified otherwise, this project is released under the MIT License. See the `LICENSE` file for details. 