/**
 * 智慧农业物联网可视化平台 - 后端服务器（生产环境）
 * 版权所有 (c) 2023-2025 
 * 开发者: 黄浩、王伟
 * 联系方式: 2768607063@163.com
 * 未经授权，禁止复制、修改或分发本代码
 */

import express, { Request, Response, RequestHandler } from 'express';
import cors from 'cors'
import mssql from 'mssql'
import dotenv from 'dotenv'
import mysql from 'mysql2/promise';
import axios from 'axios';
import path from 'path';
import fs from 'fs';

// 加载环境变量
dotenv.config();

const app = express();
const PORT = process.env.PORT || 5555;

// 中间件
app.use(express.json());
app.use(cors());

// 静态文件服务 - 前端构建文件
app.use(express.static(path.join(__dirname, '../WebFront-enInterface/web/dist')));

// 数据库配置
const dbConfig = {
  host: process.env.DB_HOST || 'localhost',
  user: process.env.DB_USER || 'root',
  password: process.env.DB_PASSWORD || 'password',
  database: process.env.DB_NAME || 'agriculture_iot'
};

// API路由
app.use('/api', require('./routes/api'));

// 控制API代理
app.use('/control', (req: Request, res: Response) => {
  const controlUrl = process.env.CONTROL_API_URL || 'http://localhost:8080';
  const targetUrl = `${controlUrl}${req.url}`;
  
  axios({
    method: req.method,
    url: targetUrl,
    data: req.body,
    headers: {
      'Content-Type': 'application/json'
    }
  })
  .then(response => {
    res.status(response.status).json(response.data);
  })
  .catch(error => {
    console.error('控制API代理错误:', error);
    res.status(500).json({ status: 'error', message: '控制API请求失败' });
  });
});

// 所有其他请求返回前端应用
app.get('*', (req: Request, res: Response) => {
  res.sendFile(path.join(__dirname, '../WebFront-enInterface/web/dist/index.html'));
});

// 启动服务器
app.listen(PORT, () => {
  console.log(`服务器运行在 http://localhost:${PORT}`);
  console.log('智慧农业物联网可视化平台 - 生产环境');
  console.log('版权所有 (c) 2023-2025 ');
}); 