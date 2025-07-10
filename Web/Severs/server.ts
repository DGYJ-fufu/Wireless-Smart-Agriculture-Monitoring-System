/**
 * 智慧农业物联网可视化平台 - 后端服务器
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

const app = express();
app.use(cors());
app.use(express.json());

dotenv.config();

// 定义数据接口类型
interface DnsData {
  序号: string;
  设备号: string;
  室内温度: string;
  室内湿度: string;
  光照强度: string;
  土壤湿度: string;
  挥发性有机化合物浓度: string;
  二氧化碳浓度: string;
  土壤温度: string;
  上报时间: string;
}

interface Dns2Data {
  序号: string;
  设备号: string;
  土壤导电率: string;
  土壤PH值: string;
  土壤含氮量: string;
  土壤含钾量: string;
  土壤含磷量: string;
  土壤盐度: string;
  土壤总溶解固体: string;
  土壤肥力: string;
  上报时间: string;
}

interface CnsData {
  序号: string;
  设备号: string;
  风扇状态: string;
  生长灯状态: string;
  水泵状态: string;
  风扇速度: string;
  水泵速度: string;
  上报时间: string;
}

interface EnsData {
  序号: string;
  设备号: string;
  室外温度: string;
  室外湿度: string;
  室外光照强度: string;
  室外压强: string;
  位置: string;
  海拔高度: string;
  上报时间: string;
}

// 数据库配置
const config = {
    host: 'The address of your database',
    port: 3306,
  user: 'The name of your database',
  password: 'The password for your database',
    database: 'The name of your database',
    connectionLimit: 10,
  timezone: 'local',  // 改为local避免时区警告
  connectTimeout: 10000, // 连接超时10秒
  waitForConnections: true,
  queueLimit: 0,
  // 添加重连选项
  enableKeepAlive: true,
  keepAliveInitialDelay: 10000
}

// 创建连接池
const pool = mysql.createPool(config);

// 测试数据库连接
pool.query('SELECT 1')
  .then(() => {
    console.log('数据库连接成功');
  })
  .catch(err => {
    console.error('数据库连接失败:', err);
  });
  
// 处理null值，替换为默认值的函数
function handleNullValues<T extends object>(data: T[], defaultValues: Record<string, any>): T[] {
  return data.map(item => {
    const processedItem = { ...item };
    
    // 处理每个字段，如果为null则使用默认值
    Object.keys(processedItem).forEach(key => {
      if (processedItem[key as keyof T] === null || processedItem[key as keyof T] === undefined) {
        processedItem[key as keyof T] = defaultValues[key] || '';
      }
    });
    
    return processedItem as T;
  });
}

// 使用备份数据的函数 - 当数据库连接失败时使用
function getFallbackData(type: string): DnsData[] | Dns2Data[] | CnsData[] | EnsData[] {
  const currentTime = new Date().toISOString().replace('T', ' ').substring(0, 19);
  
  switch(type) {
    case 'dns': 
      return [{
        "序号": "1",
        "设备号": "DNS_001",
        "室内温度": "26.5",
        "室内湿度": "65.0",
        "光照强度": "850",
        "土壤湿度": "40.5",
        "挥发性有机化合物浓度": "0.2",
        "二氧化碳浓度": "450",
        "土壤温度": "22.0",
        "上报时间": currentTime
      }] as DnsData[];
      
    case 'dns2':
      return [{
        "序号": "1",
        "设备号": "DNS2_001",
        "土壤导电率": "0.6",
        "土壤PH值": "6.8",
        "土壤含氮量": "120",
        "土壤含钾量": "160",
        "土壤含磷量": "60",
        "土壤盐度": "3.2",
        "土壤总溶解固体": "220",
        "土壤肥力": "良好",
        "上报时间": currentTime
      }] as Dns2Data[];
      
    case 'cns':
      return [{
        "序号": "1",
        "设备号": "CNS_001",
        "风扇状态": "0",
        "生长灯状态": "1",
        "水泵状态": "0",
        "风扇速度": "0",
        "水泵速度": "0",
        "上报时间": currentTime
      }] as CnsData[];
      
    case 'ens':
      return [{
        "序号": "1",
        "设备号": "ENS_001",
        "室外温度": "22.5",
        "室外湿度": "55.0",
        "室外光照强度": "1200",
        "室外压强": "1010",
        "位置": "TRUE",
        "海拔高度": "48",
        "上报时间": currentTime
      }] as EnsData[];
      
    default:
      return [] as DnsData[];
  }
}

// 智慧农业大棚检测节点
app.get('/api/AgriculturalTestingDatas', (async (req: Request, res: Response) => {
  try {
    // 使用简单查询，但设置超时处理
    const [result] = await Promise.race([
      pool.query('SELECT * FROM 智慧农业大棚检测节点 WHERE 室内温度 IS NOT NULL ORDER BY 序号 DESC LIMIT 1'),
      new Promise<any>((_, reject) => 
        setTimeout(() => reject(new Error('查询超时')), 10000)
      )
    ]) as [DnsData[], any];

    // 如果结果为空或没有记录，使用备用数据
    if (!result || result.length === 0) {
      console.log('检测节点数据为空，使用备用数据');
      return res.json(getFallbackData('dns'));
    }

    // 定义默认值
    const defaultValues = {
      '室内温度': '25.0',
      '室内湿度': '60.0',
      '光照强度': '500',
      '土壤湿度': '30.0',
      '挥发性有机化合物浓度': '0.0',
      '二氧化碳浓度': '400',
      '土壤温度': '20.0',
      '上报时间': new Date().toISOString().replace('T', ' ').substring(0, 19)
    };

    // 处理可能的null值
    const processedResult = handleNullValues<DnsData>(result, defaultValues);
    res.json(processedResult);
  } catch (error) {
    console.error('查询智慧农业大棚检测节点数据失败:', error);
    // 发生错误时返回备用数据
    res.json(getFallbackData('dns'));
  }
}) as RequestHandler);

// 智慧农业大棚检测节点_2
app.get('/api/AgriculturalTestingDatas_2', (async (req: Request, res: Response) => {
  try {
    // 使用简单查询，但设置超时处理
    const [result] = await Promise.race([
      pool.query('SELECT * FROM 智慧农业大棚检测节点_2 WHERE 土壤导电率 IS NOT NULL ORDER BY 序号 DESC LIMIT 1'),
      new Promise<any>((_, reject) => 
        setTimeout(() => reject(new Error('查询超时')), 10000)
      )
    ]) as [Dns2Data[], any];

    // 如果结果为空或没有记录，使用备用数据
    if (!result || result.length === 0) {
      console.log('检测节点_2数据为空，使用备用数据');
      return res.json(getFallbackData('dns2'));
    }

    // 定义默认值
    const defaultValues = {
      '土壤导电率': '0.5',
      '土壤PH值': '7.0',
      '土壤含氮量': '100',
      '土壤含钾量': '150',
      '土壤含磷量': '50',
      '土壤盐度': '3.0',
      '土壤总溶解固体': '200',
      '土壤肥力': '中等',
      '上报时间': new Date().toISOString().replace('T', ' ').substring(0, 19)
    };

    // 处理可能的null值
    const processedResult = handleNullValues<Dns2Data>(result, defaultValues);
    res.json(processedResult);
  } catch (error) {
    console.error('查询智慧农业大棚检测节点_2数据失败:', error);
    // 发生错误时返回备用数据
    res.json(getFallbackData('dns2'));
  }
}) as RequestHandler);

// 智慧农业大棚控制节点
app.get('/api/AgriculturalControlDatas', (async (req: Request, res: Response) => {
  try {
    // 首先执行清理和更新数据库的SQL语句
    try {
      // 删除无效数据
      await pool.query('DELETE FROM 智慧农业环境检测节点 WHERE 室外温度 IS NULL;');
      await pool.query('DELETE FROM 智慧农业大棚控制节点 WHERE 风扇状态 IS NULL;');
      await pool.query('DELETE FROM 智慧农业大棚检测节点 WHERE 室内温度 IS NULL;');
      await pool.query('DELETE FROM 智慧农业大棚检测节点_2 WHERE 土壤导电率 IS NULL;');
      // 更新位置信息
      await pool.query('UPDATE 智慧农业环境检测节点 SET 位置 = ? WHERE 设备号 = ?', 
        ['117.23172499999998 N，29.34743600000002 E', 'External_Sensor_1']);
      await pool.query('UPDATE 智慧农业环境检测节点 SET 海拔高度 = ? WHERE 设备号 = ?', 
          ['190', 'External_Sensor_1']);
      
      console.log('数据库清理和更新完成');
    } catch (cleanupError) {
      console.error('数据库清理和更新失败:', cleanupError);
      // 继续执行查询，即使清理失败
    }

    // 使用简单查询，但设置超时处理
    const [result] = await Promise.race([
      pool.query('SELECT * FROM 智慧农业大棚控制节点 WHERE 风扇状态 IS NOT NULL AND 风扇速度 IS NOT NULL  AND 水泵状态 IS NOT NULL AND 水泵速度 IS NOT NULL  AND 生长灯状态 IS NOT NULL ORDER BY 序号 DESC LIMIT 1'),
      new Promise<any>((_, reject) => 
        setTimeout(() => reject(new Error('查询超时')), 10000)
      )
    ]) as [CnsData[], any];

    // 如果结果为空或没有记录，使用备用数据
    if (!result || result.length === 0) {
      console.log('控制节点数据为空，使用备用数据');
      return res.json(getFallbackData('cns'));
    }

    // 定义默认值
    const defaultValues = {
      '风扇状态': '0',
      '生长灯状态': '0',
      '水泵状态': '0',
      '风扇速度': '0',
      '水泵速度': '0',
      '上报时间': new Date().toISOString().replace('T', ' ').substring(0, 19)
    };

    // 处理可能的null值
    const processedResult = handleNullValues<CnsData>(result, defaultValues);
    res.json(processedResult);
  } catch (error) {
    console.error('查询智慧农业大棚控制节点数据失败:', error);
    // 发生错误时返回备用数据
    res.json(getFallbackData('cns'));
  }
}) as RequestHandler);

// 智慧农业环境检测节点
app.get('/api/Agri-environmentDatas', (async (req: Request, res: Response) => {
  try {
    // 使用简单查询，但设置超时处理
    const [result] = await Promise.race([
      pool.query('SELECT * FROM 智慧农业环境检测节点 WHERE 室外温度 IS NOT NULL ORDER BY 序号 DESC LIMIT 1'),
      new Promise<any>((_, reject) => 
        setTimeout(() => reject(new Error('查询超时')), 10000)
      )
    ]) as [EnsData[], any];

    // 如果结果为空或没有记录，使用备用数据
    if (!result || result.length === 0) {
      console.log('环境检测节点数据为空，使用备用数据');
      return res.json(getFallbackData('ens'));
    }

    // 定义默认值
    const defaultValues = {
      '室外温度': '20.0',
      '室外湿度': '50.0',
      '室外光照强度': '1000',
      '室外压强': '1013',
      '位置': 'TRUE',
      '海拔高度': '50',
      '上报时间': new Date().toISOString().replace('T', ' ').substring(0, 19)
    };

    // 处理可能的null值
    const processedResult = handleNullValues<EnsData>(result, defaultValues);
    res.json(processedResult);
  } catch (error) {
    console.error('查询智慧农业环境检测节点数据失败:', error);
    // 发生错误时返回备用数据
    res.json(getFallbackData('ens'));
  }
}) as RequestHandler);

// 更新风扇状态
app.post('/api/setFanStatus/:status', (async (req: Request, res: Response) => {
  try {
    const { status } = req.params;
    const currentTime = new Date().toISOString().replace('T', ' ').substring(0, 19);
    
    // 无论当前状态如何，都执行更新操作
    await pool.query(
      'UPDATE 智慧农业大棚控制节点 SET 风扇状态 = ?, 上报时间 = ? WHERE 设备号 = "CNS_001"',
      [status, currentTime]
    );
    
    res.json({ status: 'success', message: `风扇状态已更新为${status === '1' ? '开启' : '关闭'}` });
  } catch (error) {
    console.error('更新风扇状态失败:', error);
    res.status(500).json({ status: 'error', message: '更新风扇状态失败' });
  }
}) as RequestHandler);

// 更新生长灯状态
app.post('/api/setLightStatus/:status', (async (req: Request, res: Response) => {
  try {
    const { status } = req.params;
    const currentTime = new Date().toISOString().replace('T', ' ').substring(0, 19);
    
    // 无论当前状态如何，都执行更新操作
    await pool.query(
      'UPDATE 智慧农业大棚控制节点 SET 生长灯状态 = ?, 上报时间 = ? WHERE 设备号 = "CNS_001"',
      [status, currentTime]
    );
    
    res.json({ status: 'success', message: `生长灯状态已更新为${status === '1' ? '开启' : '关闭'}` });
  } catch (error) {
    console.error('更新生长灯状态失败:', error);
    res.status(500).json({ status: 'error', message: '更新生长灯状态失败' });
  }
}) as RequestHandler);

// 更新水泵状态
app.post('/api/setPumpStatus/:status', (async (req: Request, res: Response) => {
  try {
    const { status } = req.params;
    const currentTime = new Date().toISOString().replace('T', ' ').substring(0, 19);
    
    // 无论当前状态如何，都执行更新操作
    await pool.query(
      'UPDATE 智慧农业大棚控制节点 SET 水泵状态 = ?, 上报时间 = ? WHERE 设备号 = "CNS_001"',
      [status, currentTime]
    );
    
    res.json({ status: 'success', message: `水泵状态已更新为${status === '1' ? '开启' : '关闭'}` });
  } catch (error) {
    console.error('更新水泵状态失败:', error);
    res.status(500).json({ status: 'error', message: '更新水泵状态失败' });
  }
}) as RequestHandler);

// 更新风扇速度
app.post('/api/setFanSpeed/:speed', (async (req: Request, res: Response) => {
  try {
    const { speed } = req.params;
    const currentTime = new Date().toISOString().replace('T', ' ').substring(0, 19);
    
    // 无论当前速度如何，都执行更新操作
    await pool.query(
      'UPDATE 智慧农业大棚控制节点 SET 风扇速度 = ?, 上报时间 = ? WHERE 设备号 = "CNS_001"',
      [speed, currentTime]
    );
    
    res.json({ status: 'success', message: `风扇速度已更新为${speed}` });
  } catch (error) {
    console.error('更新风扇速度失败:', error);
    res.status(500).json({ status: 'error', message: '更新风扇速度失败' });
  }
}) as RequestHandler);

// 更新水泵速度
app.post('/api/setPumpSpeed/:speed', (async (req: Request, res: Response) => {
  try {
    const { speed } = req.params;
    const currentTime = new Date().toISOString().replace('T', ' ').substring(0, 19);
    
    // 无论当前速度如何，都执行更新操作
    await pool.query(
      'UPDATE 智慧农业大棚控制节点 SET 水泵速度 = ?, 上报时间 = ? WHERE 设备号 = "CNS_001"',
      [speed, currentTime]
    );
    
    res.json({ status: 'success', message: `水泵速度已更新为${speed}` });
  } catch (error) {
    console.error('更新水泵速度失败:', error);
    res.status(500).json({ status: 'error', message: '更新水泵速度失败' });
  }
}) as RequestHandler);

// 如果没有数据，返回模拟数据的接口
app.get('/api/fallbackData/:type', (async (req: Request, res: Response) => {
  const { type } = req.params;
  const mockData = getFallbackData(type as string);
  res.json(mockData);
}) as RequestHandler);

// 如果数据库没有数据，生成模拟数据
app.get('/api/generateMockData', (async (req: Request, res: Response) => {
  try {
    const currentTime = new Date().toISOString().replace('T', ' ').substring(0, 19);
    
    try {
      // 智慧农业大棚检测节点
      await pool.query(`
        INSERT INTO 智慧农业大棚检测节点 (序号, 设备号, 室内温度, 室内湿度, 光照强度, 土壤湿度, 挥发性有机化合物浓度, 二氧化碳浓度, 土壤温度, 上报时间)
        VALUES ('1', 'DNS_001', '26.5', '65.0', '850', '40.5', '0.2', '450', '22.0', ?)
      `, [currentTime]);
      
      console.log('生成大棚检测节点模拟数据成功');
    } catch (error) {
      console.error('生成大棚检测节点模拟数据失败:', error);
    }
    
    try {
      // 智慧农业大棚检测节点_2
      await pool.query(`
        INSERT INTO 智慧农业大棚检测节点_2 (序号, 设备号, 土壤导电率, 土壤PH值, 土壤含氮量, 土壤含钾量, 土壤含磷量, 土壤盐度, 土壤总溶解固体, 土壤肥力, 上报时间)
        VALUES ('1', 'DNS2_001', '0.6', '6.8', '120', '160', '60', '3.2', '220', '良好', ?)
      `, [currentTime]);
      
      console.log('生成大棚检测节点_2模拟数据成功');
    } catch (error) {
      console.error('生成大棚检测节点_2模拟数据失败:', error);
    }
    
    try {
      // 智慧农业大棚控制节点
      await pool.query(`
        INSERT INTO 智慧农业大棚控制节点 (序号, 设备号, 风扇状态, 生长灯状态, 水泵状态, 风扇速度, 水泵速度, 上报时间)
        VALUES ('1', 'CNS_001', '0', '0', '0', '0', '0', ?)
      `, [currentTime]);
      
      console.log('生成大棚控制节点模拟数据成功');
    } catch (error) {
      console.error('生成大棚控制节点模拟数据失败:', error);
    }
    
    try {
      // 智慧农业环境检测节点
      await pool.query(`
        INSERT INTO 智慧农业环境检测节点 (序号, 设备号, 室外温度, 室外湿度, 室外光照强度, 室外压强, 位置, 海拔高度, 上报时间)
        VALUES ('1', 'ENS_001', '22.5', '55.0', '1200', '1010', 'TRUE', '48', ?)
      `, [currentTime]);
      
      console.log('生成环境检测节点模拟数据成功');
    } catch (error) {
      console.error('生成环境检测节点模拟数据失败:', error);
    }
    
    res.json({ success: true, message: '模拟数据生成请求已处理' });
  } catch (error) {
    console.error('生成模拟数据请求处理失败:', error);
    res.status(500).json({ success: false, error: '生成模拟数据失败', message: '但接口仍可使用备用数据' });
  }
}) as RequestHandler);

// ================= 用户登录接口 =================
// 允许前端 POST /api/login { username, password } 进行鉴权
app.post('/api/login', (req: Request, res: Response) => {
  const { username, password } = req.body || {};
  if (username === 'jdzvua' && password === 'Jdzvua123') {
    // 简单返回一个静态token，前端保存到localStorage用于后续鉴权
    res.json({ status: 'success', token: 'simple-auth-token' });
  } else {
    res.status(401).json({ status: 'error', message: '用户名或密码错误' });
  }
});

// 启动服务器
const PORT = process.env.PORT || 5555;
app.listen(PORT, () => {
    console.log(`服务器启动在${PORT}端口`);
});