/**
 * 智慧农业物联网可视化平台 - API路由
 * 版权所有 (c) 2023-2025 
 * 开发者: 黄浩、王伟
 * 联系方式: 2768607063@163.com
 * 未经授权，禁止复制、修改或分发本代码
 */

import express, { Request, Response } from 'express';
import mysql from 'mysql2/promise';
import dotenv from 'dotenv';

// 加载环境变量
dotenv.config();

const router = express.Router();

// 数据库配置
const dbConfig = {
  host: process.env.DB_HOST || 'localhost',
  user: process.env.DB_USER || 'root',
  password: process.env.DB_PASSWORD || 'password',
  database: process.env.DB_NAME || 'agriculture_iot'
};

// 获取大棚内检测节点数据
router.get('/AgriculturalTestingDatas', async (req: Request, res: Response) => {
  try {
    const connection = await mysql.createConnection(dbConfig);
    const [rows] = await connection.query('SELECT * FROM AgriculturalTestingDatas ORDER BY 序号 DESC LIMIT 50');
    await connection.end();
    res.json(rows);
  } catch (error) {
    console.error('获取大棚内检测节点数据失败:', error);
    res.status(500).json({ error: '获取数据失败' });
  }
});

// 获取土壤成分数据
router.get('/AgriculturalTestingDatas_2', async (req: Request, res: Response) => {
  try {
    const connection = await mysql.createConnection(dbConfig);
    const [rows] = await connection.query('SELECT * FROM AgriculturalTestingDatas_2 ORDER BY 序号 DESC LIMIT 50');
    await connection.end();
    res.json(rows);
  } catch (error) {
    console.error('获取土壤成分数据失败:', error);
    res.status(500).json({ error: '获取数据失败' });
  }
});

// 获取控制节点数据
router.get('/AgriculturalControlDatas', async (req: Request, res: Response) => {
  try {
    const connection = await mysql.createConnection(dbConfig);
    // 自动清理NULL值记录
    await connection.query('DELETE FROM AgriculturalControlDatas WHERE 风扇状态 IS NULL AND 生长灯状态 IS NULL AND 水泵状态 IS NULL');
    // 更新位置信息
    await connection.query('UPDATE AgriculturalControlDatas SET 位置 = "江西省景德镇市" WHERE 位置 IS NULL OR 位置 = ""');
    
    const [rows] = await connection.query('SELECT * FROM AgriculturalControlDatas ORDER BY 序号 DESC LIMIT 50');
    await connection.end();
    res.json(rows);
  } catch (error) {
    console.error('获取控制节点数据失败:', error);
    res.status(500).json({ error: '获取数据失败' });
  }
});

// 获取环境数据
router.get('/Agri-environmentDatas', async (req: Request, res: Response) => {
  try {
    const connection = await mysql.createConnection(dbConfig);
    const [rows] = await connection.query('SELECT * FROM `Agri-environmentDatas` ORDER BY 序号 DESC LIMIT 50');
    await connection.end();
    res.json(rows);
  } catch (error) {
    console.error('获取大棚外环境数据失败:', error);
    res.status(500).json({ error: '获取数据失败' });
  }
});

module.exports = router; 