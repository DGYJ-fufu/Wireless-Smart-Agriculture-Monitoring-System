import React, { useEffect, useState, memo, useCallback } from 'react';
import { BrowserRouter as Router, Routes, Route, Link, useNavigate } from 'react-router-dom';
import PrivateRoute from './components/PrivateRoute';
import LoginPage from './components/LoginPage';
import './App.css';
import axios from 'axios';
import HistoryChart from './components/HistoryChart';
import HistoryDataTable from './components/HistoryDataTable';
import ThresholdSettings from './components/ThresholdSettings';
import AutoControlSettings from './components/AutoControlSettings';
import SystemLog from './components/SystemLog';
import NotificationToast, { Notification } from './components/NotificationToast';
import { formatTime } from './utils/timeFormatter';
import config from '../config/config';
import { v4 as uuidv4 } from 'uuid';

// 设置axios默认配置
axios.defaults.timeout = 10000; // 10秒超时
axios.defaults.headers.common['Content-Type'] = 'application/json';

// 数据接口定义
interface DNS {
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

interface DNS_2 {
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

interface CNS {
  序号: string;
  设备号: string;
  风扇状态: string; // boolean(布尔)
  生长灯状态: string; // boolean(布尔)
  水泵状态: string; // boolean(布尔)
  风扇速度: string;
  水泵速度: string;
  上报时间: string;
}

interface ENS {
  序号: string;
  设备号: string;
  室外温度: string;
  室外湿度: string;
  室外光照强度: string;
  室外压强: string;
  位置: string; // 经纬度坐标
  海拔高度: string;
  上报时间: string;
}

// 自动控制设置接口
interface AutoControlSettings {
  temperatureThreshold: number; // 温度阈值
  idealTemperature: number; // 理想温度
  fanSpeed: number; // 风扇速度
  lightThreshold: number; // 光照阈值
  soilSalinityThreshold: number; // 土壤盐度阈值
  idealSoilSalinity: number; // 理想土壤盐度
  pumpSpeed: number; // 水泵速度
}

// 数据最佳和最差状态设置接口
interface DataThresholdSettings {
  室内温度: { best: number; worst: number };
  室内湿度: { best: number; worst: number };
  光照强度: { best: number; worst: number };
  土壤湿度: { best: number; worst: number };
  挥发性有机化合物浓度: { best: number; worst: number };
  二氧化碳浓度: { best: number; worst: number };
  土壤温度: { best: number; worst: number };
  土壤导电率: { best: number; worst: number };
  土壤PH值: { best: number; worst: number };
  土壤含氮量: { best: number; worst: number };
  土壤含钾量: { best: number; worst: number };
  土壤含磷量: { best: number; worst: number };
  土壤盐度: { best: number; worst: number };
  土壤总溶解固体: { best: number; worst: number };
}

// 系统日志接口定义
interface LogEntry {
  id: number;
  timestamp: string;
  action: string;
  device?: string;
  status?: string;
  details?: string;
  type: 'info' | 'warning' | 'error' | 'success';
}

// API响应类型
interface ApiResponse {
  status: 'success' | 'error';
  message: string;
  source?: string;
  error_code?: string;
}

const API_URL = config.API_URL;
const CONTROL_API_URL = config.CONTROL_API_URL;
const HISTORY_API_URL = config.HISTORY_API_URL; // 使用历史数据API地址

// 提取时间显示为独立组件，并使用memo包装
const TimeDisplay = memo(() => {
  const [currentTime, setCurrentTime] = useState<string>('');
  
  // 更新当前时间
  useEffect(() => {
    const updateCurrentTime = () => {
      const now = new Date();
      const formattedTime = `${now.getFullYear()}-${String(now.getMonth() + 1).padStart(2, '0')}-${String(now.getDate()).padStart(2, '0')} ${String(now.getHours()).padStart(2, '0')}:${String(now.getMinutes()).padStart(2, '0')}:${String(now.getSeconds()).padStart(2, '0')}`;
      setCurrentTime(formattedTime);
    };

    updateCurrentTime(); // 立即更新一次
    const timer = setInterval(updateCurrentTime, 1000); // 每秒更新一次

    return () => clearInterval(timer);
  }, []);
  
  return (
    <div className="current-time">
      <span className="time-label">当前时间:</span>
      <span className="time-value">{currentTime}</span>
    </div>
  );
});

// 在App函数开始前添加一个格式化数值的辅助函数
const formatNumber = (value: string | undefined): string => {
  if (!value || value === '--') return '--';
  const numValue = parseFloat(value);
  if (isNaN(numValue)) return value;
  return numValue.toFixed(2);
};

// 数据获取函数 - 从后端server.ts获取数据
const fetchDNS = async (): Promise<DNS[]> => {
  try {
    const response = await axios.get(`${API_URL}AgriculturalTestingDatas`);
    return response.data as DNS[];
  } catch (error) {
    console.error('获取检测节点数据失败:', error);
    return [];
  }
};

const fetchDNS_2 = async (): Promise<DNS_2[]> => {
  try {
    const response = await axios.get(`${API_URL}AgriculturalTestingDatas_2`);
    return response.data as DNS_2[];
  } catch (error) {
    console.error('获取土壤成分数据失败:', error);
    return [];
  }
};

const fetchCNS = async (): Promise<CNS[]> => {
  try {
    const response = await axios.get(`${API_URL}AgriculturalControlDatas`);
    return response.data as CNS[];
  } catch (error) {
    console.error('获取控制节点数据失败:', error);
    return [];
  }
};

const fetchENS = async (): Promise<ENS[]> => {
  try {
    const response = await axios.get(`${API_URL}Agri-environmentDatas`);
    return response.data as ENS[];
  } catch (error) {
    console.error('获取环境检测数据失败:', error);
    return [];
  }
};

function App() {
  // 数据状态
  const [dns, setDns] = useState<DNS[]>([]);
  const [dns2, setDns_2] = useState<DNS_2[]>([]);
  const [cns, setCns] = useState<CNS[]>([]);
  const [ens, setEns] = useState<ENS[]>([]);
  
  // 显示模式
  const [showLatestOnly, setShowLatestOnly] = useState<boolean>(true);
  const [displayCount, setDisplayCount] = useState<number>(5);
  
  // 设备控制状态
  const [fanStatus, setFanStatus] = useState<boolean>(false);
  const [lightStatus, setLightStatus] = useState<boolean>(false);
  const [pumpStatus, setPumpStatus] = useState<boolean>(false);
  const [fanSpeed, setFanSpeed] = useState<number>(0);
  const [pumpSpeed, setPumpSpeed] = useState<number>(0);
  
  // 新增: 统一更新控制节点数据首行的辅助函数，确保实时数据卡片立即同步
  const updateCnsField = (fields: Partial<CNS>) => {
    setCns(prev => {
      if (prev.length === 0) return prev;
      const updated = [...prev];
      updated[0] = { ...updated[0], ...fields } as CNS;
      return updated;
    });
  };
  
  // 临时速度状态（用于滑块调整但未确认的值）
  const [tempFanSpeed, setTempFanSpeed] = useState<number>(0);
  const [tempPumpSpeed, setTempPumpSpeed] = useState<number>(0);
  
  // 自动控制设置
  const [autoControl, setAutoControl] = useState<boolean>(false);
  const [autoSettings, setAutoSettings] = useState<AutoControlSettings>({
    temperatureThreshold: 28, // 默认温度阈值
    idealTemperature: 24, // 默认理想温度
    fanSpeed: 80, // 默认风扇速度
    lightThreshold: 500, // 默认光照阈值
    soilSalinityThreshold: 8, // 默认土壤盐度阈值
    idealSoilSalinity: 5, // 默认理想土壤盐度
    pumpSpeed: 70 // 默认水泵速度
  });

  // 数据阈值设置
  const [dataThresholds, setDataThresholds] = useState<DataThresholdSettings>({
    室内温度: { best: 25, worst: 35 },
    室内湿度: { best: 60, worst: 90 },
    光照强度: { best: 800, worst: 200 },
    土壤湿度: { best: 65, worst: 30 },
    挥发性有机化合物浓度: { best: 400, worst: 1000 },
    二氧化碳浓度: { best: 600, worst: 1200 },
    土壤温度: { best: 22, worst: 30 },
    土壤导电率: { best: 1.5, worst: 3.0 },
    土壤PH值: { best: 6.5, worst: 5.0 },
    土壤含氮量: { best: 150, worst: 50 },
    土壤含钾量: { best: 200, worst: 80 },
    土壤含磷量: { best: 100, worst: 30 },
    土壤盐度: { best: 3, worst: 8 },
    土壤总溶解固体: { best: 700, worst: 1500 }
  });

  // 显示数据阈值设置面板
  const [showThresholdSettings, setShowThresholdSettings] = useState<boolean>(false);
  
  // 当前编辑的阈值类别
  const [currentThresholdCategory, setCurrentThresholdCategory] = useState<string>("环境数据");

  // 数据刷新间隔 (30秒)
  const [refreshInterval, setRefreshInterval] = useState<number>(30000);
  const [showSettings, setShowSettings] = useState<boolean>(false);

  // 移除currentTime状态，改用TimeDisplay组件
  const [lastUpdateTime, setLastUpdateTime] = useState<string>('');

  // 添加系统日志状态
  const [systemLogs, setSystemLogs] = useState<LogEntry[]>([]);
  
  // 添加通知状态
  const [notifications, setNotifications] = useState<Notification[]>([]);

  // 生成随机ID的辅助函数
  const generateId = () => {
    return Date.now() + Math.floor(Math.random() * 1000);
  };

  // 添加新的日志条目
  const addLogEntry = useCallback((
    action: string, 
    type: 'info' | 'warning' | 'error' | 'success', 
    device?: string, 
    status?: string, 
    details?: string
  ) => {
    const newLog: LogEntry = {
      id: generateId(),
      timestamp: new Date().toISOString(),
      action,
      device,
      status,
      details,
      type
    };
    
    setSystemLogs(prevLogs => [newLog, ...prevLogs]);
    
    // 将日志保存到本地存储
    const savedLogs = JSON.parse(localStorage.getItem('systemLogs') || '[]');
    localStorage.setItem('systemLogs', JSON.stringify([newLog, ...savedLogs]));
    
    return newLog;
  }, []);

  // 清除所有日志
  const clearLogs = useCallback(() => {
    setSystemLogs([]);
    localStorage.removeItem('systemLogs');
  }, []);

  // 显示通知
  const showNotification = useCallback((message: string, type: 'success' | 'error' | 'info' | 'warning' = 'info', duration = 3000) => {
    const id = uuidv4();
    const newNotification: Notification = {
      id,
      message,
      type,
      duration
    };
    
    setNotifications(prev => [...prev, newNotification]);
    return id;
  }, []);

  // 移除通知
  const removeNotification = useCallback((id: string) => {
    setNotifications(prev => prev.filter(notification => notification.id !== id));
  }, []);

  // 初始加载系统日志
  useEffect(() => {
    const savedLogs = JSON.parse(localStorage.getItem('systemLogs') || '[]');
    if (savedLogs.length > 0) {
      setSystemLogs(savedLogs);
    } else {
      // 如果没有日志，添加一条初始化日志
      addLogEntry('系统初始化', 'info', '全部', '正常', '系统启动并初始化完成');
    }
  }, [addLogEntry]);

  // 加载数据
  const loadData = async () => {
    try {
      // 获取所有数据
      const [dnsData, dns2Data, cnsData, ensData] = await Promise.all([
        fetchDNS(),
        fetchDNS_2(),
        fetchCNS(),
        fetchENS()
      ]);
      
      // 更新状态
      setDns(dnsData);
      setDns_2(dns2Data);
      setCns(cnsData);
      setEns(ensData);
      
      // 从控制节点数据更新设备状态
      if (cnsData.length > 0) {
        const latestData = cnsData[0];
        console.log("控制节点数据:", cnsData);
        console.log("最新控制节点数据:", latestData);
        
        // 解析风扇状态
        const fanStatusValue = String(latestData.风扇状态);
        console.log("风扇状态:", fanStatusValue, "类型:", typeof fanStatusValue);
        
        // 设置设备状态
        const newStatus = {
          isFanOn: fanStatusValue === '1' || fanStatusValue.toUpperCase() === 'TRUE',
          isLightOn: String(latestData.生长灯状态) === '1' || String(latestData.生长灯状态).toUpperCase() === 'TRUE',
          isPumpOn: String(latestData.水泵状态) === '1' || String(latestData.水泵状态).toUpperCase() === 'TRUE'
        };
        
        console.log("解析后的状态:", newStatus);
        
        // 更新设备状态
        setFanStatus(newStatus.isFanOn);
        setLightStatus(newStatus.isLightOn);
        setPumpStatus(newStatus.isPumpOn);
        
        // 设置速度值
        if (latestData.风扇速度 !== undefined) {
          const fanSpeedValue = parseInt(String(latestData.风扇速度), 10);
          if (!isNaN(fanSpeedValue)) {
            setFanSpeed(fanSpeedValue);
            setTempFanSpeed(fanSpeedValue);
          }
        }
        
        if (latestData.水泵速度 !== undefined) {
          const pumpSpeedValue = parseInt(String(latestData.水泵速度), 10);
          if (!isNaN(pumpSpeedValue)) {
            setPumpSpeed(pumpSpeedValue);
            setTempPumpSpeed(pumpSpeedValue);
          }
        }
      }
      
      // 执行自动控制逻辑
      if (autoControl && dnsData.length > 0 && dns2Data.length > 0) {
        // 提取当前环境数据
        const currentTemp = parseFloat(dnsData[0].室内温度);
        const currentLight = parseFloat(dnsData[0].光照强度);
        const currentSalinity = parseFloat(dns2Data[0].土壤盐度);
        
        // 调试日志，检查阈值比较结果
        console.log("温度自动控制条件检查:", {
          currentTemp,
          temperatureThreshold: autoSettings.temperatureThreshold,
          idealTemperature: autoSettings.idealTemperature,
          fanStatus,
          conditionExceeded: currentTemp >= autoSettings.temperatureThreshold,
          conditionIdeal: currentTemp <= autoSettings.idealTemperature,
          shouldTurnOn: currentTemp >= autoSettings.temperatureThreshold && !fanStatus,
          shouldTurnOff: currentTemp <= autoSettings.idealTemperature && fanStatus
        });
        
        if (currentTemp >= autoSettings.temperatureThreshold && !fanStatus) {
          addLogEntry(
            '自动控制-温度阈值', 
            'warning', 
            '风扇', 
            '检测', 
            `温度(${currentTemp}°C)已超过设定阈值(${autoSettings.temperatureThreshold}°C)，正在尝试开启风扇`
          );
          
          // 通过POST请求控制风扇
          const result = await toggleFan(true, true);
          if (result) {
            addLogEntry(
              '自动控制-开启风扇', 
              'success', 
              '风扇', 
              '开启', 
              `当前温度${currentTemp}°C超过阈值${autoSettings.temperatureThreshold}°C，自动开启风扇，速度设置为${autoSettings.fanSpeed}%`
            );
            showNotification(`自动控制：温度${currentTemp}°C超过阈值，已开启风扇(${autoSettings.fanSpeed}%)`, 'info');
            // 通过POST请求设置风扇速度
            await setFanSpeedValue(autoSettings.fanSpeed, true);
          } else {
            addLogEntry(
              '自动控制-开启风扇', 
              'error', 
              '风扇', 
              '失败', 
              `尝试自动开启风扇失败，请检查设备连接或手动控制`
            );
          }
        } else if (currentTemp <= autoSettings.idealTemperature && fanStatus) {
          addLogEntry(
            '自动控制-温度理想', 
            'info', 
            '风扇', 
            '检测', 
            `温度(${currentTemp}°C)已达到理想温度(${autoSettings.idealTemperature}°C)，正在尝试关闭风扇`
          );
          
          // 通过POST请求控制风扇
          const result = await toggleFan(false, true);
          if (result) {
            addLogEntry(
              '自动控制-关闭风扇', 
              'success', 
              '风扇', 
              '关闭', 
              `当前温度${currentTemp}°C已达到理想温度${autoSettings.idealTemperature}°C，自动关闭风扇`
            );
            showNotification(`自动控制：温度${currentTemp}°C已达到理想温度，已关闭风扇`, 'info');
          } else {
            addLogEntry(
              '自动控制-关闭风扇', 
              'error', 
              '风扇', 
              '失败', 
              `尝试自动关闭风扇失败，请检查设备连接或手动控制`
            );
          }
        } else {
          // 修改此处的判断逻辑，区分不同状态
          if (currentTemp >= autoSettings.temperatureThreshold && fanStatus) {
            // 温度超过阈值但风扇已开启
            addLogEntry(
              '自动控制-温度超阈值', 
              'info', 
              '风扇', 
              '已运行', 
              `当前温度${currentTemp}°C超过设定阈值${autoSettings.temperatureThreshold}°C，风扇已处于开启状态`
            );
          } else if (currentTemp <= autoSettings.idealTemperature && !fanStatus) {
            // 温度低于理想温度且风扇已关闭
            addLogEntry(
              '自动控制-温度低于理想', 
              'info', 
              '风扇', 
              '已关闭', 
              `当前温度${currentTemp}°C低于理想温度${autoSettings.idealTemperature}°C或等于理想温度，风扇已处于关闭状态`
            );
          } else {
            // 温度在理想温度和阈值之间
            addLogEntry(
              '自动控制-温度正常', 
              'info', 
              '风扇', 
              '无需操作', 
              `当前温度${currentTemp}°C在设定范围内(${autoSettings.idealTemperature}°C-${autoSettings.temperatureThreshold}°C)，风扇状态保持${fanStatus ? '开启' : '关闭'}`
            );
          }
        }
        
        // 调试日志，检查光照阈值比较结果
        console.log("光照自动控制条件检查:", {
          currentLight,
          lightThreshold: autoSettings.lightThreshold,
          lightStatus,
          conditionBelow: currentLight < autoSettings.lightThreshold,
          conditionAbove: currentLight >= autoSettings.lightThreshold,
          shouldTurnOn: currentLight < autoSettings.lightThreshold && !lightStatus,
          shouldTurnOff: currentLight >= autoSettings.lightThreshold && lightStatus
        });
        
        if (currentLight < autoSettings.lightThreshold && !lightStatus) {
          addLogEntry(
            '自动控制-光照阈值', 
            'warning', 
            '生长灯', 
            '检测', 
            `光照强度(${currentLight}lx)低于设定阈值(${autoSettings.lightThreshold}lx)，正在尝试开启生长灯`
          );
          
          // 通过POST请求控制生长灯
          const result = await toggleLight(true, true);
          if (result) {
            addLogEntry(
              '自动控制-开启生长灯', 
              'success', 
              '生长灯', 
              '开启', 
              `当前光照强度${currentLight}lx低于阈值${autoSettings.lightThreshold}lx，自动开启生长灯`
            );
            showNotification(`自动控制：光照强度${currentLight}lx低于阈值，已开启生长灯`, 'info');
          } else {
            addLogEntry(
              '自动控制-开启生长灯', 
              'error', 
              '生长灯', 
              '失败', 
              `尝试自动开启生长灯失败，请检查设备连接或手动控制`
            );
          }
        } else if (currentLight >= autoSettings.lightThreshold && lightStatus) {
          addLogEntry(
            '自动控制-光照充足', 
            'info', 
            '生长灯', 
            '检测', 
            `光照强度(${currentLight}lx)已达到设定阈值(${autoSettings.lightThreshold}lx)，正在尝试关闭生长灯`
          );
          
          // 通过POST请求控制生长灯
          const result = await toggleLight(false, true);
          if (result) {
            addLogEntry(
              '自动控制-关闭生长灯', 
              'success', 
              '生长灯', 
              '关闭', 
              `当前光照强度${currentLight}lx已达到设定阈值${autoSettings.lightThreshold}lx，自动关闭生长灯`
            );
            showNotification(`自动控制：光照强度${currentLight}lx已达到阈值，已关闭生长灯`, 'info');
          } else {
            addLogEntry(
              '自动控制-关闭生长灯', 
              'error', 
              '生长灯', 
              '失败', 
              `尝试自动关闭生长灯失败，请检查设备连接或手动控制`
            );
          }
        } else {
          // 不需要操作
          if (currentLight < autoSettings.lightThreshold && lightStatus) {
            addLogEntry(
              '自动控制-光照不足', 
              'info', 
              '生长灯', 
              '已开启', 
              `当前光照强度${currentLight}lx低于设定阈值${autoSettings.lightThreshold}lx，生长灯已处于开启状态`
            );
          } else if (currentLight >= autoSettings.lightThreshold && !lightStatus) {
            addLogEntry(
              '自动控制-光照充足', 
              'info', 
              '生长灯', 
              '已关闭', 
              `当前光照强度${currentLight}lx高于设定阈值${autoSettings.lightThreshold}lx，生长灯已处于关闭状态`
            );
          }
        }
        
        // 调试日志，检查盐度阈值比较结果
        console.log("盐度自动控制条件检查:", {
          currentSalinity,
          soilSalinityThreshold: autoSettings.soilSalinityThreshold,
          idealSoilSalinity: autoSettings.idealSoilSalinity,
          pumpStatus,
          conditionExceeded: currentSalinity >= autoSettings.soilSalinityThreshold,
          conditionIdeal: currentSalinity <= autoSettings.idealSoilSalinity,
          shouldTurnOn: currentSalinity >= autoSettings.soilSalinityThreshold && !pumpStatus,
          shouldTurnOff: currentSalinity <= autoSettings.idealSoilSalinity && pumpStatus
        });
        
        if (currentSalinity >= autoSettings.soilSalinityThreshold && !pumpStatus) {
          addLogEntry(
            '自动控制-盐度阈值', 
            'warning', 
            '水泵', 
            '检测', 
            `土壤盐度(${currentSalinity}g/kg)已超过设定阈值(${autoSettings.soilSalinityThreshold}g/kg)，正在尝试开启水泵`
          );
          
          // 通过POST请求控制水泵
          const result = await togglePump(true, true);
          if (result) {
            addLogEntry(
              '自动控制-开启水泵', 
              'success', 
              '水泵', 
              '开启', 
              `当前土壤盐度${currentSalinity}g/kg超过阈值${autoSettings.soilSalinityThreshold}g/kg，自动开启水泵，速度设置为${autoSettings.pumpSpeed}%`
            );
            showNotification(`自动控制：土壤盐度${currentSalinity}g/kg超过阈值，已开启水泵(${autoSettings.pumpSpeed}%)`, 'info');
            // 通过POST请求设置水泵速度
            await setPumpSpeedValue(autoSettings.pumpSpeed, true);
          } else {
            addLogEntry(
              '自动控制-开启水泵', 
              'error', 
              '水泵', 
              '失败', 
              `尝试自动开启水泵失败，请检查设备连接或手动控制`
            );
          }
        } else if (currentSalinity <= autoSettings.idealSoilSalinity && pumpStatus) {
          addLogEntry(
            '自动控制-盐度理想', 
            'info', 
            '水泵', 
            '检测', 
            `土壤盐度(${currentSalinity}g/kg)已达到理想盐度(${autoSettings.idealSoilSalinity}g/kg)，正在尝试关闭水泵`
          );
          
          // 通过POST请求控制水泵
          const result = await togglePump(false, true);
          if (result) {
            addLogEntry(
              '自动控制-关闭水泵', 
              'success', 
              '水泵', 
              '关闭', 
              `当前土壤盐度${currentSalinity}g/kg已达到理想盐度${autoSettings.idealSoilSalinity}g/kg，自动关闭水泵`
            );
            showNotification(`自动控制：土壤盐度${currentSalinity}g/kg已达到理想盐度，已关闭水泵`, 'info');
          } else {
            addLogEntry(
              '自动控制-关闭水泵', 
              'error', 
              '水泵', 
              '失败', 
              `尝试自动关闭水泵失败，请检查设备连接或手动控制`
            );
          }
        } else {
          // 修改此处的判断逻辑，区分不同状态
          if (currentSalinity >= autoSettings.soilSalinityThreshold && pumpStatus) {
            // 盐度超过阈值但水泵已开启
            addLogEntry(
              '自动控制-盐度超阈值', 
              'info', 
              '水泵', 
              '已运行', 
              `当前土壤盐度${currentSalinity}g/kg超过设定阈值${autoSettings.soilSalinityThreshold}g/kg，水泵已处于开启状态`
            );
          } else if (currentSalinity <= autoSettings.idealSoilSalinity && !pumpStatus) {
            // 盐度低于理想盐度且水泵已关闭
            addLogEntry(
              '自动控制-盐度低于理想', 
              'info', 
              '水泵', 
              '已关闭', 
              `当前土壤盐度${currentSalinity}g/kg低于理想盐度${autoSettings.idealSoilSalinity}g/kg或等于理想盐度，水泵已处于关闭状态`
            );
          } else {
            // 盐度在理想盐度和阈值之间
            addLogEntry(
              '自动控制-盐度正常', 
              'info', 
              '水泵', 
              '无需操作', 
              `当前土壤盐度${currentSalinity}g/kg在设定范围内(${autoSettings.idealSoilSalinity}g/kg-${autoSettings.soilSalinityThreshold}g/kg)，水泵状态保持${pumpStatus ? '开启' : '关闭'}`
            );
          }
        }
      } else if (autoControl) {
        // 如果自动控制开启但数据不足
        const reason = !dnsData.length && !dns2Data.length ? '检测数据和土壤数据均未获取到' : 
                      !dnsData.length ? '检测数据未获取到' : '土壤数据未获取到';
        addLogEntry(
          '自动控制-未执行', 
          'warning', 
          '系统', 
          '数据不足', 
          `自动控制功能已启用，但${reason}，无法执行自动控制逻辑`
        );
      }

      // 设置最后更新时间
      const now = new Date();
      // 使用ISO字符串再格式化，确保显示完整日期和时间
      setLastUpdateTime(formatTime(now.toISOString(), 'YYYY-MM-DD HH:mm:ss'));
      
    } catch (error) {
      console.error('数据加载错误:', error);
      addLogEntry(
        '数据加载', 
        'error', 
        '系统', 
        '加载失败', 
        `数据加载时发生错误: ${error instanceof Error ? error.message : '未知错误'}`
      );
    }
  };

  // 初始加载和定时刷新
  useEffect(() => {
    loadData(); // 初始加载数据
    
    // 只在实时数据模式下设置定时刷新
    let intervalId: number | null = null;
    if (showLatestOnly) {
      intervalId = setInterval(loadData, refreshInterval);
    }
    
    return () => {
      if (intervalId) {
        clearInterval(intervalId);
      }
    };
  }, [refreshInterval, autoControl, autoSettings, showLatestOnly]);

  // 当从数据库加载新的风扇和水泵速度时，同步更新临时速度值
  useEffect(() => {
    setTempFanSpeed(fanSpeed);
    setTempPumpSpeed(pumpSpeed);
  }, [fanSpeed, pumpSpeed]);

  /**
   * 设备控制函数
   * 
   * 以下所有设备控制函数都通过POST请求发送控制命令到SendCmdServer.py服务。
   * 自动控制和手动控制通过请求头 X-Auto-Control 区分。
   * 所有请求都使用POST方法，严格遵循SendCmdServer.py中定义的接口格式。
   */

  // 切换风扇状态
  const toggleFan = async (status: boolean, isAutoControl: boolean = false) => {
    try {
      const endpoint = status ? 'stft' : 'stff';
      console.log(`正在发送风扇${status ? '开启' : '关闭'}请求到: ${CONTROL_API_URL}${endpoint}`);
      
      // 添加详细的请求信息
      const headers = {
        'Content-Type': 'application/json',
        'X-Auto-Control': isAutoControl ? 'true' : 'false'
      };
      console.log('请求头:', headers);
      
      // 尝试使用多种方式发送请求，首先尝试XMLHttpRequest
      return new Promise<boolean>((resolve) => {
        try {
          console.log("尝试使用XMLHttpRequest发送请求");
          const xhr = new XMLHttpRequest();
          xhr.open('POST', `${CONTROL_API_URL}${endpoint}`, true);
          xhr.setRequestHeader('Content-Type', 'application/json');
          xhr.setRequestHeader('X-Auto-Control', isAutoControl ? 'true' : 'false');
          
          xhr.onload = function() {
            if (xhr.status >= 200 && xhr.status < 300) {
              try {
                const responseData = JSON.parse(xhr.responseText) as ApiResponse;
                console.log(`[XHR] 风扇状态切换为${status ? '开启' : '关闭'}响应:`, responseData);
                
                if (responseData.status === 'success') {
                  setFanStatus(status);
                  updateCnsField({ 风扇状态: status ? '1' : '0' });
                  addLogEntry(
                    `${isAutoControl ? '自动控制' : '手动控制'}-风扇${status ? '开启' : '关闭'}`,
                    'success',
                    '风扇',
                    status ? '开启' : '关闭',
                    `已${status ? '开启' : '关闭'}风扇 (XHR)`
                  );
                  resolve(true);
                } else {
                  console.error('[XHR] 风扇控制失败:', responseData.message);
                  addLogEntry(
                    `${isAutoControl ? '自动控制' : '手动控制'}-风扇${status ? '开启' : '关闭'}`,
                    'error',
                    '风扇',
                    '失败',
                    `风扇${status ? '开启' : '关闭'}失败 (XHR): ${responseData.message}`
                  );
                  
                  // XHR失败，尝试Fetch
                  tryFetch();
                }
              } catch (parseError) {
                console.error('[XHR] 解析响应失败:', parseError, '原始响应:', xhr.responseText);
                // XHR响应解析失败，尝试Fetch
                tryFetch();
              }
            } else {
              console.error('[XHR] 风扇控制请求失败 - 状态码:', xhr.status);
              // XHR请求失败，尝试Fetch
              tryFetch();
            }
          };
          
          xhr.onerror = function() {
            console.error('[XHR] 风扇控制请求失败 - 网络错误');
            // XHR请求网络错误，尝试Fetch
            tryFetch();
          };
          
          xhr.send(JSON.stringify({}));
        } catch (xhrError) {
          console.error('[XHR] 创建或发送请求失败:', xhrError);
          // XHR请求创建失败，尝试Fetch
          tryFetch();
        }
        
        // 后备方案：使用fetch API发送请求
        async function tryFetch() {
          try {
            console.log("尝试使用fetch API发送请求");
            
            const response = await fetch(`${CONTROL_API_URL}${endpoint}`, {
              method: 'POST',
              headers: headers,
              body: JSON.stringify({})
            });
            
            if (!response.ok) {
              throw new Error(`HTTP错误，状态码: ${response.status}`);
            }
            
            const responseData = await response.json() as ApiResponse;
            console.log(`[Fetch] 风扇状态切换为${status ? '开启' : '关闭'}响应:`, responseData);
            
            if (responseData.status === 'success') {
              setFanStatus(status);
              updateCnsField({ 风扇状态: status ? '1' : '0' });
              addLogEntry(
                `${isAutoControl ? '自动控制' : '手动控制'}-风扇${status ? '开启' : '关闭'}`,
                'success',
                '风扇',
                status ? '开启' : '关闭',
                `已${status ? '开启' : '关闭'}风扇 (Fetch)`
              );
              resolve(true);
      } else {
              console.error('[Fetch] 风扇控制失败:', responseData.message);
              addLogEntry(
                `${isAutoControl ? '自动控制' : '手动控制'}-风扇${status ? '开启' : '关闭'}`,
                'error',
                '风扇',
                '失败',
                `风扇${status ? '开启' : '关闭'}失败 (Fetch): ${responseData.message}`
              );
              resolve(false);
            }
          } catch (fetchError: unknown) {
            console.error('[Fetch] 风扇控制请求失败:', fetchError);
            if (fetchError instanceof Error) {
              console.error('[Fetch] 错误详情:', fetchError.message);
            }
            
            // 最后尝试axios
            tryAxios();
          }
        }
        
        // 最后的备选方案：使用axios
        async function tryAxios() {
          try {
            console.log("尝试使用axios发送请求");
            
            const response = await axios.post(
              `${CONTROL_API_URL}${endpoint}`,
              {}, // 空对象作为请求体
              {
                headers: headers
              }
            );
            
            console.log(`[Axios] 风扇状态切换为${status ? '开启' : '关闭'}响应:`, response.data);
            
            if (response.data.status === 'success') {
              setFanStatus(status);
              updateCnsField({ 风扇状态: status ? '1' : '0' });
              addLogEntry(
                `${isAutoControl ? '自动控制' : '手动控制'}-风扇${status ? '开启' : '关闭'}`,
                'success',
                '风扇',
                status ? '开启' : '关闭',
                `已${status ? '开启' : '关闭'}风扇 (Axios)`
              );
              resolve(true);
            } else {
              console.error('[Axios] 风扇控制失败:', response.data.message);
              addLogEntry(
                `${isAutoControl ? '自动控制' : '手动控制'}-风扇${status ? '开启' : '关闭'}`,
                'error',
                '风扇',
                '失败',
                `风扇${status ? '开启' : '关闭'}失败 (Axios): ${response.data.message}`
              );
              resolve(false);
            }
          } catch (axiosError: unknown) {
            console.error('[Axios] 风扇控制请求失败:', axiosError);
            if (axiosError instanceof Error) {
              console.error('[Axios] 错误详情:', axiosError.message);
            }
            
            addLogEntry(
              `${isAutoControl ? '自动控制' : '手动控制'}-风扇${status ? '开启' : '关闭'}`,
              'error',
              '风扇',
              '失败',
              `风扇${status ? '开启' : '关闭'}请求失败 (所有方法): ${axiosError instanceof Error ? axiosError.message : '未知错误'}`
            );
            resolve(false);
          }
        }
      });
    } catch (error: unknown) {
      // 捕获Promise外层的错误
      console.error(`[外层错误] 风扇控制请求失败:`, error);
      if (error instanceof Error) {
        console.error('错误详情:', error.message);
      }
      
      addLogEntry(
        `${isAutoControl ? '自动控制' : '手动控制'}-风扇${status ? '开启' : '关闭'}`,
        'error',
        '风扇',
        '失败',
        `风扇${status ? '开启' : '关闭'}请求失败: ${error instanceof Error ? error.message : '未知错误'}`
      );
      return false;
    }
  };

  // 切换生长灯状态
  const toggleLight = async (status: boolean, isAutoControl: boolean = false) => {
    try {
      const endpoint = status ? 'stgt' : 'stgf';
      console.log(`正在发送生长灯${status ? '开启' : '关闭'}请求到: ${CONTROL_API_URL}${endpoint}`);
      
      // 添加详细的请求信息
      const headers = {
        'Content-Type': 'application/json',
        'X-Auto-Control': isAutoControl ? 'true' : 'false'
      };
      console.log('请求头:', headers);
      
      // 尝试使用多种方式发送请求，首先尝试XMLHttpRequest
      return new Promise<boolean>((resolve) => {
        try {
          console.log("尝试使用XMLHttpRequest发送请求");
          const xhr = new XMLHttpRequest();
          xhr.open('POST', `${CONTROL_API_URL}${endpoint}`, true);
          xhr.setRequestHeader('Content-Type', 'application/json');
          xhr.setRequestHeader('X-Auto-Control', isAutoControl ? 'true' : 'false');
          
          xhr.onload = function() {
            if (xhr.status >= 200 && xhr.status < 300) {
              try {
                const responseData = JSON.parse(xhr.responseText) as ApiResponse;
                console.log(`[XHR] 生长灯状态切换为${status ? '开启' : '关闭'}响应:`, responseData);
                
                if (responseData.status === 'success') {
                  setLightStatus(status);
                  updateCnsField({ 生长灯状态: status ? '1' : '0' });
                  addLogEntry(
                    `${isAutoControl ? '自动控制' : '手动控制'}-生长灯${status ? '开启' : '关闭'}`,
                    'success',
                    '生长灯',
                    status ? '开启' : '关闭',
                    `已${status ? '开启' : '关闭'}生长灯 (XHR)`
                  );
                  resolve(true);
                } else {
                  console.error('[XHR] 生长灯控制失败:', responseData.message);
                  addLogEntry(
                    `${isAutoControl ? '自动控制' : '手动控制'}-生长灯${status ? '开启' : '关闭'}`,
                    'error',
                    '生长灯',
                    '失败',
                    `生长灯${status ? '开启' : '关闭'}失败 (XHR): ${responseData.message}`
                  );
                  
                  // XHR失败，尝试Fetch
                  tryFetch();
                }
              } catch (parseError) {
                console.error('[XHR] 解析响应失败:', parseError, '原始响应:', xhr.responseText);
                // XHR响应解析失败，尝试Fetch
                tryFetch();
              }
            } else {
              console.error('[XHR] 生长灯控制请求失败 - 状态码:', xhr.status);
              // XHR请求失败，尝试Fetch
              tryFetch();
            }
          };
          
          xhr.onerror = function() {
            console.error('[XHR] 生长灯控制请求失败 - 网络错误');
            // XHR请求网络错误，尝试Fetch
            tryFetch();
          };
          
          xhr.send(JSON.stringify({}));
        } catch (xhrError) {
          console.error('[XHR] 创建或发送请求失败:', xhrError);
          // XHR请求创建失败，尝试Fetch
          tryFetch();
        }
        
        // 后备方案：使用fetch API发送请求
        async function tryFetch() {
          try {
            console.log("尝试使用fetch API发送请求");
            
            const response = await fetch(`${CONTROL_API_URL}${endpoint}`, {
              method: 'POST',
              headers: headers,
              body: JSON.stringify({})
            });
            
            if (!response.ok) {
              throw new Error(`HTTP错误，状态码: ${response.status}`);
            }
            
            const responseData = await response.json() as ApiResponse;
            console.log(`[Fetch] 生长灯状态切换为${status ? '开启' : '关闭'}响应:`, responseData);
            
            if (responseData.status === 'success') {
              setLightStatus(status);
              updateCnsField({ 生长灯状态: status ? '1' : '0' });
              addLogEntry(
                `${isAutoControl ? '自动控制' : '手动控制'}-生长灯${status ? '开启' : '关闭'}`,
                'success',
                '生长灯',
                status ? '开启' : '关闭',
                `已${status ? '开启' : '关闭'}生长灯 (Fetch)`
              );
              resolve(true);
      } else {
              console.error('[Fetch] 生长灯控制失败:', responseData.message);
              addLogEntry(
                `${isAutoControl ? '自动控制' : '手动控制'}-生长灯${status ? '开启' : '关闭'}`,
                'error',
                '生长灯',
                '失败',
                `生长灯${status ? '开启' : '关闭'}失败 (Fetch): ${responseData.message}`
              );
              resolve(false);
            }
          } catch (fetchError: unknown) {
            console.error('[Fetch] 生长灯控制请求失败:', fetchError);
            if (fetchError instanceof Error) {
              console.error('[Fetch] 错误详情:', fetchError.message);
            }
            
            // 最后尝试axios
            tryAxios();
          }
        }
        
        // 最后的备选方案：使用axios
        async function tryAxios() {
          try {
            console.log("尝试使用axios发送请求");
            
            const response = await axios.post(
              `${CONTROL_API_URL}${endpoint}`,
              {}, // 空对象作为请求体
              {
                headers: headers
              }
            );
            
            console.log(`[Axios] 生长灯状态切换为${status ? '开启' : '关闭'}响应:`, response.data);
            
            if (response.data.status === 'success') {
              setLightStatus(status);
              updateCnsField({ 生长灯状态: status ? '1' : '0' });
              addLogEntry(
                `${isAutoControl ? '自动控制' : '手动控制'}-生长灯${status ? '开启' : '关闭'}`,
                'success',
                '生长灯',
                status ? '开启' : '关闭',
                `已${status ? '开启' : '关闭'}生长灯 (Axios)`
              );
              resolve(true);
            } else {
              console.error('[Axios] 生长灯控制失败:', response.data.message);
              addLogEntry(
                `${isAutoControl ? '自动控制' : '手动控制'}-生长灯${status ? '开启' : '关闭'}`,
                'error',
                '生长灯',
                '失败',
                `生长灯${status ? '开启' : '关闭'}失败 (Axios): ${response.data.message}`
              );
              resolve(false);
            }
          } catch (axiosError: unknown) {
            console.error('[Axios] 生长灯控制请求失败:', axiosError);
            if (axiosError instanceof Error) {
              console.error('[Axios] 错误详情:', axiosError.message);
            }
            
            addLogEntry(
              `${isAutoControl ? '自动控制' : '手动控制'}-生长灯${status ? '开启' : '关闭'}`,
              'error',
              '生长灯',
              '失败',
              `生长灯${status ? '开启' : '关闭'}请求失败 (所有方法): ${axiosError instanceof Error ? axiosError.message : '未知错误'}`
            );
            resolve(false);
          }
        }
      });
    } catch (error: unknown) {
      // 捕获Promise外层的错误
      console.error(`[外层错误] 生长灯控制请求失败:`, error);
      if (error instanceof Error) {
        console.error('错误详情:', error.message);
      }
      
      addLogEntry(
        `${isAutoControl ? '自动控制' : '手动控制'}-生长灯${status ? '开启' : '关闭'}`,
        'error',
        '生长灯',
        '失败',
        `生长灯${status ? '开启' : '关闭'}请求失败: ${error instanceof Error ? error.message : '未知错误'}`
      );
      return false;
    }
  };

  // 切换水泵状态
  const togglePump = async (status: boolean, isAutoControl: boolean = false) => {
    try {
      const endpoint = status ? 'stpt' : 'stpf';
      console.log(`正在发送水泵${status ? '开启' : '关闭'}请求到: ${CONTROL_API_URL}${endpoint}`);
      
      // 添加详细的请求信息
      const headers = {
        'Content-Type': 'application/json',
        'X-Auto-Control': isAutoControl ? 'true' : 'false'
      };
      console.log('请求头:', headers);
      
      // 尝试使用多种方式发送请求，首先尝试XMLHttpRequest
      return new Promise<boolean>((resolve) => {
        try {
          console.log("尝试使用XMLHttpRequest发送请求");
          const xhr = new XMLHttpRequest();
          xhr.open('POST', `${CONTROL_API_URL}${endpoint}`, true);
          xhr.setRequestHeader('Content-Type', 'application/json');
          xhr.setRequestHeader('X-Auto-Control', isAutoControl ? 'true' : 'false');
          
          xhr.onload = function() {
            if (xhr.status >= 200 && xhr.status < 300) {
              try {
                const responseData = JSON.parse(xhr.responseText) as ApiResponse;
                console.log(`[XHR] 水泵状态切换为${status ? '开启' : '关闭'}响应:`, responseData);
                
                if (responseData.status === 'success') {
                  setPumpStatus(status);
                  updateCnsField({ 水泵状态: status ? '1' : '0' });
                  addLogEntry(
                    `${isAutoControl ? '自动控制' : '手动控制'}-水泵${status ? '开启' : '关闭'}`,
                    'success',
                    '水泵',
                    status ? '开启' : '关闭',
                    `已${status ? '开启' : '关闭'}水泵 (XHR)`
                  );
                  resolve(true);
                } else {
                  console.error('[XHR] 水泵控制失败:', responseData.message);
                  addLogEntry(
                    `${isAutoControl ? '自动控制' : '手动控制'}-水泵${status ? '开启' : '关闭'}`,
                    'error',
                    '水泵',
                    '失败',
                    `水泵${status ? '开启' : '关闭'}失败 (XHR): ${responseData.message}`
                  );
                  
                  // XHR失败，尝试Fetch
                  tryFetch();
                }
              } catch (parseError) {
                console.error('[XHR] 解析响应失败:', parseError, '原始响应:', xhr.responseText);
                // XHR响应解析失败，尝试Fetch
                tryFetch();
              }
            } else {
              console.error('[XHR] 水泵控制请求失败 - 状态码:', xhr.status);
              // XHR请求失败，尝试Fetch
              tryFetch();
            }
          };
          
          xhr.onerror = function() {
            console.error('[XHR] 水泵控制请求失败 - 网络错误');
            // XHR请求网络错误，尝试Fetch
            tryFetch();
          };
          
          xhr.send(JSON.stringify({}));
        } catch (xhrError) {
          console.error('[XHR] 创建或发送请求失败:', xhrError);
          // XHR请求创建失败，尝试Fetch
          tryFetch();
        }
        
        // 后备方案：使用fetch API发送请求
        async function tryFetch() {
          try {
            console.log("尝试使用fetch API发送请求");
            
            const response = await fetch(`${CONTROL_API_URL}${endpoint}`, {
              method: 'POST',
              headers: headers,
              body: JSON.stringify({})
            });
            
            if (!response.ok) {
              throw new Error(`HTTP错误，状态码: ${response.status}`);
            }
            
            const responseData = await response.json() as ApiResponse;
            console.log(`[Fetch] 水泵状态切换为${status ? '开启' : '关闭'}响应:`, responseData);
            
            if (responseData.status === 'success') {
              setPumpStatus(status);
              updateCnsField({ 水泵状态: status ? '1' : '0' });
              addLogEntry(
                `${isAutoControl ? '自动控制' : '手动控制'}-水泵${status ? '开启' : '关闭'}`,
                'success',
                '水泵',
                status ? '开启' : '关闭',
                `已${status ? '开启' : '关闭'}水泵 (Fetch)`
              );
              resolve(true);
      } else {
              console.error('[Fetch] 水泵控制失败:', responseData.message);
              addLogEntry(
                `${isAutoControl ? '自动控制' : '手动控制'}-水泵${status ? '开启' : '关闭'}`,
                'error',
                '水泵',
                '失败',
                `水泵${status ? '开启' : '关闭'}失败 (Fetch): ${responseData.message}`
              );
              resolve(false);
            }
          } catch (fetchError: unknown) {
            console.error('[Fetch] 水泵控制请求失败:', fetchError);
            if (fetchError instanceof Error) {
              console.error('[Fetch] 错误详情:', fetchError.message);
            }
            
            // 最后尝试axios
            tryAxios();
          }
        }
        
        // 最后的备选方案：使用axios
        async function tryAxios() {
          try {
            console.log("尝试使用axios发送请求");
            
            const response = await axios.post(
              `${CONTROL_API_URL}${endpoint}`,
              {}, // 空对象作为请求体
              {
                headers: headers
              }
            );
            
            console.log(`[Axios] 水泵状态切换为${status ? '开启' : '关闭'}响应:`, response.data);
            
            if (response.data.status === 'success') {
              setPumpStatus(status);
              updateCnsField({ 水泵状态: status ? '1' : '0' });
              addLogEntry(
                `${isAutoControl ? '自动控制' : '手动控制'}-水泵${status ? '开启' : '关闭'}`,
                'success',
                '水泵',
                status ? '开启' : '关闭',
                `已${status ? '开启' : '关闭'}水泵 (Axios)`
              );
              resolve(true);
            } else {
              console.error('[Axios] 水泵控制失败:', response.data.message);
              addLogEntry(
                `${isAutoControl ? '自动控制' : '手动控制'}-水泵${status ? '开启' : '关闭'}`,
                'error',
                '水泵',
                '失败',
                `水泵${status ? '开启' : '关闭'}失败 (Axios): ${response.data.message}`
              );
              resolve(false);
            }
          } catch (axiosError: unknown) {
            console.error('[Axios] 水泵控制请求失败:', axiosError);
            if (axiosError instanceof Error) {
              console.error('[Axios] 错误详情:', axiosError.message);
            }
            
            addLogEntry(
              `${isAutoControl ? '自动控制' : '手动控制'}-水泵${status ? '开启' : '关闭'}`,
              'error',
              '水泵',
              '失败',
              `水泵${status ? '开启' : '关闭'}请求失败 (所有方法): ${axiosError instanceof Error ? axiosError.message : '未知错误'}`
            );
            resolve(false);
          }
        }
      });
    } catch (error: unknown) {
      // 捕获Promise外层的错误
      console.error(`[外层错误] 水泵控制请求失败:`, error);
      if (error instanceof Error) {
        console.error('错误详情:', error.message);
      }
      
      addLogEntry(
        `${isAutoControl ? '自动控制' : '手动控制'}-水泵${status ? '开启' : '关闭'}`,
        'error',
        '水泵',
        '失败',
        `水泵${status ? '开启' : '关闭'}请求失败: ${error instanceof Error ? error.message : '未知错误'}`
      );
      return false;
    }
  };

  // 设置风扇速度
  const setFanSpeedValue = async (value: number, isAutoControl: boolean = false) => {
    try {
      // 确保风扇速度为整数
      const speedValue = Math.round(value);
      console.log(`正在发送风扇速度设置请求到: ${CONTROL_API_URL}setFanSpeed/${speedValue}`);
      
      return new Promise<boolean>((resolve) => {
        // 使用XMLHttpRequest直接发送请求
        const xhr = new XMLHttpRequest();
        xhr.open('POST', `${CONTROL_API_URL}setFanSpeed/${speedValue}`, true);
        xhr.setRequestHeader('Content-Type', 'application/json');
        xhr.setRequestHeader('X-Auto-Control', isAutoControl ? 'true' : 'false');
        
        xhr.onload = function() {
          if (xhr.status >= 200 && xhr.status < 300) {
            try {
              const responseData = JSON.parse(xhr.responseText) as ApiResponse;
              console.log(`[XHR] 设置风扇速度为${speedValue}响应:`, responseData);
              
              if (responseData.status === 'success') {
                setFanSpeed(speedValue);
                updateCnsField({ 风扇速度: String(speedValue) });
                addLogEntry(
                  `${isAutoControl ? '自动控制' : '手动控制'}-风扇速度设置`,
                  'success',
                  '风扇',
                  '速度调整',
                  `风扇速度已设置为${speedValue}`
                );
                resolve(true);
              } else {
                console.error('[XHR] 风扇速度设置失败:', responseData.message);
                addLogEntry(
                  `${isAutoControl ? '自动控制' : '手动控制'}-风扇速度设置`,
                  'error',
                  '风扇',
                  '失败',
                  `风扇速度设置失败: ${responseData.message}`
                );
                resolve(false);
              }
            } catch (parseError) {
              console.error('[XHR] 解析响应失败:', parseError, '原始响应:', xhr.responseText);
              resolve(false);
            }
      } else {
            console.error('[XHR] 风扇速度设置请求失败 - 状态码:', xhr.status);
            resolve(false);
          }
        };
        
        xhr.onerror = function() {
          console.error('[XHR] 风扇速度设置请求失败 - 网络错误');
          resolve(false);
        };
        
        xhr.send(JSON.stringify({}));
      });
    } catch (error: unknown) {
      // 捕获Promise外层的错误
      console.error(`[外层错误] 风扇速度设置请求失败:`, error);
      if (error instanceof Error) {
        console.error('错误详情:', error.message);
      }
      
      addLogEntry(
        `${isAutoControl ? '自动控制' : '手动控制'}-风扇速度设置`,
        'error',
        '风扇',
        '失败',
        `风扇速度设置请求失败: ${error instanceof Error ? error.message : '未知错误'}`
      );
      return false;
    }
  };

  // 设置水泵速度
  const setPumpSpeedValue = async (value: number, isAutoControl: boolean = false) => {
    try {
      // 确保水泵速度为整数
      const speedValue = Math.round(value);
      console.log(`正在发送水泵速度设置请求到: ${CONTROL_API_URL}setPumpSpeed/${speedValue}`);
      
      return new Promise<boolean>((resolve) => {
        // 使用XMLHttpRequest直接发送请求
        const xhr = new XMLHttpRequest();
        xhr.open('POST', `${CONTROL_API_URL}setPumpSpeed/${speedValue}`, true);
        xhr.setRequestHeader('Content-Type', 'application/json');
        xhr.setRequestHeader('X-Auto-Control', isAutoControl ? 'true' : 'false');
        
        xhr.onload = function() {
          if (xhr.status >= 200 && xhr.status < 300) {
            try {
              const responseData = JSON.parse(xhr.responseText) as ApiResponse;
              console.log(`[XHR] 设置水泵速度为${speedValue}响应:`, responseData);
              
              if (responseData.status === 'success') {
                setPumpSpeed(speedValue);
                updateCnsField({ 水泵速度: String(speedValue) });
                addLogEntry(
                  `${isAutoControl ? '自动控制' : '手动控制'}-水泵速度设置`,
                  'success',
                  '水泵',
                  '速度调整',
                  `水泵速度已设置为${speedValue}`
                );
                resolve(true);
              } else {
                console.error('[XHR] 水泵速度设置失败:', responseData.message);
                addLogEntry(
                  `${isAutoControl ? '自动控制' : '手动控制'}-水泵速度设置`,
                  'error',
                  '水泵',
                  '失败',
                  `水泵速度设置失败: ${responseData.message}`
                );
                resolve(false);
              }
            } catch (parseError) {
              console.error('[XHR] 解析响应失败:', parseError, '原始响应:', xhr.responseText);
              resolve(false);
            }
      } else {
            console.error('[XHR] 水泵速度设置请求失败 - 状态码:', xhr.status);
            resolve(false);
          }
        };
        
        xhr.onerror = function() {
          console.error('[XHR] 水泵速度设置请求失败 - 网络错误');
          resolve(false);
        };
        
        xhr.send(JSON.stringify({}));
      });
    } catch (error: unknown) {
      // 捕获Promise外层的错误
      console.error(`[外层错误] 水泵速度设置请求失败:`, error);
      if (error instanceof Error) {
        console.error('错误详情:', error.message);
      }
      
      addLogEntry(
        `${isAutoControl ? '自动控制' : '手动控制'}-水泵速度设置`,
        'error',
        '水泵',
        '失败',
        `水泵速度设置请求失败: ${error instanceof Error ? error.message : '未知错误'}`
      );
      return false;
    }
  };

  // 准备要显示的数据
  const displayDns = showLatestOnly ? dns.slice(0, 1) : dns.slice(0, displayCount);
  const displayDns2 = showLatestOnly ? dns2.slice(0, 1) : dns2.slice(0, displayCount);
  const displayCns = showLatestOnly ? cns.slice(0, 1) : cns.slice(0, displayCount);
  const displayEns = showLatestOnly ? ens.slice(0, 1) : ens.slice(0, displayCount);

  // 主界面组件
  const Dashboard = () => {
    // 用于退出登录后的页面跳转
    const navigate = useNavigate();
    
    // 添加测试按钮函数
    const testSendCmdServer = async () => {
      try {
        console.log("测试SendCmdServer连接 - 开始测试");
        
        // 测试健康检查接口 - 使用fetch
        try {
          console.log("使用fetch测试健康检查接口");
          const healthResponse = await fetch(`${CONTROL_API_URL}health`, {
            method: 'GET',
            headers: {
              'Accept': 'application/json'
            }
          });
          
          if (healthResponse.ok) {
            const healthData = await healthResponse.json();
            console.log("健康检查响应:", healthData);
            showNotification(`健康检查成功: ${JSON.stringify(healthData)}`, 'success');
          } else {
            console.error("健康检查失败 - 状态码:", healthResponse.status);
            showNotification(`健康检查失败: 状态码 ${healthResponse.status}`, 'error');
          }
        } catch (error) {
          console.error("健康检查失败:", error);
          showNotification(`健康检查失败: ${error instanceof Error ? error.message : '未知错误'}`, 'error');
        }
        
        // 测试专用的POST测试接口
        try {
          console.log("使用fetch测试专用POST接口");
          console.log("请求URL:", `${CONTROL_API_URL}test-post`);
          
          const testPostResponse = await fetch(`${CONTROL_API_URL}test-post`, {
            method: 'POST',
            headers: {
              'Content-Type': 'application/json',
              'X-Test-Mode': 'true'
            },
            body: JSON.stringify({
              test: true,
              timestamp: new Date().toISOString(),
              message: '这是一个测试POST请求'
            })
          });
          
          if (testPostResponse.ok) {
            const testPostData = await testPostResponse.json();
            console.log("专用POST测试响应:", testPostData);
            showNotification(`专用POST测试成功: ${JSON.stringify(testPostData)}`, 'success');
          } else {
            console.error("专用POST测试失败 - 状态码:", testPostResponse.status);
            showNotification(`专用POST测试失败: 状态码 ${testPostResponse.status}`, 'error');
          }
        } catch (error) {
          console.error("专用POST测试失败:", error);
          showNotification(`专用POST测试失败: ${error instanceof Error ? error.message : '未知错误'}`, 'error');
        }
        
        // 测试设备状态接口 - 风扇关闭 - 使用fetch
        try {
          console.log("使用fetch测试风扇关闭请求");
          console.log("请求URL:", `${CONTROL_API_URL}stff`);
          console.log("请求头:", {
            'Content-Type': 'application/json',
            'X-Test-Mode': 'true'
          });
          
          const fanOffResponse = await fetch(`${CONTROL_API_URL}stff`, {
            method: 'POST',
            headers: {
              'Content-Type': 'application/json',
              'X-Test-Mode': 'true'
            },
            body: JSON.stringify({}) // 空对象作为请求体
          });
          
          if (fanOffResponse.ok) {
            const fanOffData = await fanOffResponse.json();
            console.log("风扇关闭响应:", fanOffData);
            showNotification(`风扇关闭测试成功: ${JSON.stringify(fanOffData)}`, 'success');
          } else {
            console.error("风扇关闭测试失败 - 状态码:", fanOffResponse.status);
            showNotification(`风扇关闭测试失败: 状态码 ${fanOffResponse.status}`, 'error');
          }
        } catch (error) {
          console.error("风扇关闭测试失败:", error);
          showNotification(`风扇关闭测试失败: ${error instanceof Error ? error.message : '未知错误'}`, 'error');
        }
        
        // 直接通过XMLHttpRequest发送请求
        try {
          console.log("使用XMLHttpRequest测试风扇关闭请求");
          const xhr = new XMLHttpRequest();
          xhr.open('POST', `${CONTROL_API_URL}stff`, true);
          xhr.setRequestHeader('Content-Type', 'application/json');
          xhr.setRequestHeader('X-Test-Mode', 'true');
          
          xhr.onload = function() {
            if (xhr.status >= 200 && xhr.status < 300) {
              console.log("XHR风扇关闭响应:", xhr.responseText);
              showNotification(`XHR风扇关闭测试成功: ${xhr.responseText}`, 'success');
            } else {
              console.error("XHR风扇关闭测试失败 - 状态码:", xhr.status);
              showNotification(`XHR风扇关闭测试失败: 状态码 ${xhr.status}`, 'error');
            }
          };
          
          xhr.onerror = function() {
            console.error("XHR风扇关闭测试失败 - 网络错误");
            showNotification(`XHR风扇关闭测试失败: 网络错误`, 'error');
          };
          
          xhr.send(JSON.stringify({}));
        } catch (error) {
          console.error("XHR风扇关闭测试失败:", error);
          showNotification(`XHR风扇关闭测试失败: ${error instanceof Error ? error.message : '未知错误'}`, 'error');
        }
        
        console.log("测试SendCmdServer连接 - 测试完成");
      } catch (error) {
        console.error("测试失败:", error);
        showNotification(`测试失败: ${error instanceof Error ? error.message : '未知错误'}`, 'error');
      }
    };
    
    return (
      <div className="dashboard">
        <header className="dashboard-header">
          <div className="time-display">
            <TimeDisplay />
            <div className="update-time">
              <span className="time-label">数据更新时间:</span>
              <span className="time-value">{lastUpdateTime}</span>
            </div>
            
            {/* 删除测试按钮 */}
          </div>
          <h1>智慧农业物联网可视化监控平台</h1>
          <nav className="top-nav">
            <div className="nav-left">
              <button 
                className={`display-toggle ${showLatestOnly ? 'active' : ''}`}
                onClick={() => setShowLatestOnly(true)}
              >
                实时数据
              </button>
              <button 
                className={`display-toggle ${!showLatestOnly ? 'active' : ''}`}
                onClick={() => setShowLatestOnly(false)}
              >
                历史数据
              </button>
              <div className="display-count">
                <span>显示条数:</span>
                <select 
                  value={displayCount}
                  onChange={(e) => setDisplayCount(Number(e.target.value))}
                >
                  <option value="5">5条</option>
                  <option value="10">10条</option>
                  <option value="20">20条</option>
                  <option value="50">50条</option>
                </select>
              </div>
              <div className="refresh-rate">
                <span>刷新间隔:</span>
                <select 
                  value={refreshInterval}
                  onChange={(e) => setRefreshInterval(Number(e.target.value))}
                >
                  <option value="5000">5秒</option>
                  <option value="10000">10秒</option>
                  <option value="30000">30秒</option>
                  <option value="60000">1分钟</option>
                </select>
              </div>
              <button 
                className="refresh-btn"
                onClick={loadData}
              >
                刷新数据
              </button>
            </div>
            <div className="nav-right">
              <Link to="/threshold-settings" className="nav-link">
                阈值设置
              </Link>
              <Link to="/auto-control-settings" className="nav-link">
                自动控制设置
              </Link>
              <Link to="/system-log" className="nav-link">
                系统日志
              </Link>
              <button
                className="nav-link logout-btn"
                onClick={() => {
                  localStorage.removeItem('authToken');
                  navigate('/login');
                }}
              >
                退出登录
              </button>
            </div>
          </nav>
        </header>

        <div className="dashboard-content">
          {showLatestOnly ? (
            <>
              <div className="data-panel">
                {/* 环境数据 */}
                <section className="data-section">
                  <h2>环境数据 <span className="device-id">设备号: {dns[0]?.设备号 || '--'}</span></h2>
                  <div className="data-cards-grid">
                    <div className="data-card">
                      <div className="card-icon">🌡️</div>
                      <div className="card-label">室内温度</div>
                      <div className="card-value">{formatNumber(dns[0]?.室内温度)}</div>
                      <div className="card-unit">°C</div>
                    </div>
                    <div className="data-card">
                      <div className="card-icon">💧</div>
                      <div className="card-label">室内湿度</div>
                      <div className="card-value">{formatNumber(dns[0]?.室内湿度)}</div>
                      <div className="card-unit">%</div>
                    </div>
                    <div className="data-card">
                      <div className="card-icon">☀️</div>
                      <div className="card-label">光照强度</div>
                      <div className="card-value">{formatNumber(dns[0]?.光照强度)}</div>
                      <div className="card-unit">lx</div>
                    </div>
                    <div className="data-card">
                      <div className="card-icon">🌱</div>
                      <div className="card-label">土壤湿度</div>
                      <div className="card-value">{formatNumber(dns[0]?.土壤湿度)}</div>
                      <div className="card-unit">%</div>
                    </div>
                    <div className="data-card">
                      <div className="card-icon">🌬️</div>
                      <div className="card-label">VOC浓度</div>
                      <div className="card-value">{formatNumber(dns[0]?.挥发性有机化合物浓度)}</div>
                      <div className="card-unit">ppm</div>
                    </div>
                    <div className="data-card">
                      <div className="card-icon">☁️</div>
                      <div className="card-label">CO2浓度</div>
                      <div className="card-value">{formatNumber(dns[0]?.二氧化碳浓度)}</div>
                      <div className="card-unit">ppm</div>
                    </div>
                    <div className="data-card">
                      <div className="card-icon">🌡️</div>
                      <div className="card-label">土壤温度</div>
                      <div className="card-value">{formatNumber(dns[0]?.土壤温度)}</div>
                      <div className="card-unit">°C</div>
                    </div>
                  </div>
                </section>

                {/* 土壤数据 */}
                <section className="data-section">
                  <h2>土壤数据 <span className="device-id">设备号: {dns2[0]?.设备号 || '--'}</span></h2>
                  <div className="data-cards-grid">
                    <div className="data-card">
                      <div className="card-icon">⚡</div>
                      <div className="card-label">土壤导电率</div>
                      <div className="card-value">{formatNumber(dns2[0]?.土壤导电率)}</div>
                      <div className="card-unit">mS/cm</div>
                    </div>
                    <div className="data-card">
                      <div className="card-icon">🧪</div>
                      <div className="card-label">土壤PH值</div>
                      <div className="card-value">{formatNumber(dns2[0]?.土壤PH值)}</div>
                      <div className="card-unit">pH</div>
                    </div>
                    <div className="data-card">
                      <div className="card-icon">🌿</div>
                      <div className="card-label">土壤含氮量</div>
                      <div className="card-value">{formatNumber(dns2[0]?.土壤含氮量)}</div>
                      <div className="card-unit">mg/kg</div>
                    </div>
                    <div className="data-card">
                      <div className="card-icon">🌱</div>
                      <div className="card-label">土壤含钾量</div>
                      <div className="card-value">{formatNumber(dns2[0]?.土壤含钾量)}</div>
                      <div className="card-unit">mg/kg</div>
                    </div>
                    <div className="data-card">
                      <div className="card-icon">🌿</div>
                      <div className="card-label">土壤含磷量</div>
                      <div className="card-value">{formatNumber(dns2[0]?.土壤含磷量)}</div>
                      <div className="card-unit">mg/kg</div>
                    </div>
                    <div className="data-card">
                      <div className="card-icon">🧂</div>
                      <div className="card-label">土壤盐度</div>
                      <div className="card-value">{formatNumber(dns2[0]?.土壤盐度)}</div>
                      <div className="card-unit">g/kg</div>
                    </div>
                    <div className="data-card">
                      <div className="card-icon">💎</div>
                      <div className="card-label">总溶解固体</div>
                      <div className="card-value">{formatNumber(dns2[0]?.土壤总溶解固体)}</div>
                      <div className="card-unit">mg/L</div>
                    </div>
                    <div className="data-card">
                      <div className="card-icon">🌺</div>
                      <div className="card-label">土壤肥力</div>
                      <div className="card-value">{formatNumber(dns2[0]?.土壤肥力)}</div>
                      <div className="card-unit">%</div>
                    </div>
                  </div>
                </section>

                {/* 设备状态 */}
                <section className="data-section">
                  <h2>设备状态 <span className="device-id">设备号: {cns[0]?.设备号 || '--'}</span></h2>
                  <div className="data-cards-grid">
                    <div className="data-card">
                      <div className="card-icon">🌪️</div>
                      <div className="card-label">风扇状态</div>
                      <div className="card-value">{String(cns[0]?.风扇状态).toUpperCase() === '1' || String(cns[0]?.风扇状态).toUpperCase() === 'TRUE' ? '开启' : '关闭'}</div>
                      <div className="card-status-indicator" data-status={String(cns[0]?.风扇状态).toUpperCase() === '1' || String(cns[0]?.风扇状态).toUpperCase() === 'TRUE' ? 'on' : 'off'}></div>
                    </div>
                    <div className="data-card">
                      <div className="card-icon">💡</div>
                      <div className="card-label">生长灯状态</div>
                      <div className="card-value">{String(cns[0]?.生长灯状态).toUpperCase() === '1' || String(cns[0]?.生长灯状态).toUpperCase() === 'TRUE' ? '开启' : '关闭'}</div>
                      <div className="card-status-indicator" data-status={String(cns[0]?.生长灯状态).toUpperCase() === '1' || String(cns[0]?.生长灯状态).toUpperCase() === 'TRUE' ? 'on' : 'off'}></div>
                    </div>
                    <div className="data-card">
                      <div className="card-icon">💧</div>
                      <div className="card-label">水泵状态</div>
                      <div className="card-value">{String(cns[0]?.水泵状态).toUpperCase() === '1' || String(cns[0]?.水泵状态).toUpperCase() === 'TRUE' ? '开启' : '关闭'}</div>
                      <div className="card-status-indicator" data-status={String(cns[0]?.水泵状态).toUpperCase() === '1' || String(cns[0]?.水泵状态).toUpperCase() === 'TRUE' ? 'on' : 'off'}></div>
                    </div>
                    <div className="data-card">
                      <div className="card-icon">🌪️</div>
                      <div className="card-label">风扇速度</div>
                      <div className="card-value">{formatNumber(cns[0]?.风扇速度)}</div>
                      <div className="card-unit">%</div>
                    </div>
                    <div className="data-card">
                      <div className="card-icon">💧</div>
                      <div className="card-label">水泵速度</div>
                      <div className="card-value">{formatNumber(cns[0]?.水泵速度)}</div>
                      <div className="card-unit">%</div>
                    </div>
                  </div>
                </section>

                {/* 室外数据 */}
                <section className="data-section">
                  <h2>室外数据 <span className="device-id">设备号: {ens[0]?.设备号 || 'External_Sensor_1'}</span></h2>
                  <div className="data-cards-grid">
                    <div className="data-card">
                      <div className="card-icon">🌡️</div>
                      <div className="card-label">室外温度</div>
                      <div className="card-value">{formatNumber(ens[0]?.室外温度 || '25')}</div>
                      <div className="card-unit">°C</div>
                    </div>
                    <div className="data-card">
                      <div className="card-icon">💧</div>
                      <div className="card-label">室外湿度</div>
                      <div className="card-value">{formatNumber(ens[0]?.室外湿度)}</div>
                      <div className="card-unit">%</div>
                    </div>
                    <div className="data-card">
                      <div className="card-icon">☀️</div>
                      <div className="card-label">室外光照强度</div>
                      <div className="card-value">{formatNumber(ens[0]?.室外光照强度)}</div>
                      <div className="card-unit">lx</div>
                    </div>
                    <div className="data-card">
                      <div className="card-icon">🌍</div>
                      <div className="card-label">室外压强</div>
                      <div className="card-value">{formatNumber(ens[0]?.室外压强)}</div>
                      <div className="card-unit">hPa</div>
                    </div>
                    <div className="data-card">
                      <div className="card-icon">📍</div>
                      <div className="card-label">位置</div>
                      <div className="card-value" style={{ fontSize: '0.9rem' }}>{"江西省景德镇市"}</div>
                      <div className="card-unit">经纬度</div>
                    </div>
                    <div className="data-card">
                      <div className="card-icon">⛰️</div>
                      <div className="card-label">海拔高度</div>
                      <div className="card-value">{formatNumber('190')}</div>
                      <div className="card-unit">m</div>
                    </div>
                  </div>
                </section>
              </div>

              {/* 右侧控制面板 */}
              <div className="control-panel">
                <h2>设备手动控制</h2>
                
                <div className="control-card">
                  <h3><span className="card-icon">🌀</span> 风扇控制</h3>
                  <div className="status-display">
                    <span className="status-label">当前状态:</span>
                    <span className={`status-value ${fanStatus ? 'on' : 'off'}`}>
                      {fanStatus ? '运行中' : '已停止'}
                    </span>
                  </div>
                  <div className="control-buttons">
                    <button 
                      className={`control-btn on ${fanStatus ? 'active' : ''}`}
                      onClick={async () => {
                        try {
                          const endpoint = 'stft';
                          console.log(`正在发送风扇开启请求到: ${CONTROL_API_URL}${endpoint}`);
                          
                          // 使用XMLHttpRequest直接发送请求
                          const xhr = new XMLHttpRequest();
                          xhr.open('POST', `${CONTROL_API_URL}${endpoint}`, true);
                          xhr.setRequestHeader('Content-Type', 'application/json');
                          xhr.setRequestHeader('X-Auto-Control', 'false');
                          
                          xhr.onload = function() {
                            if (xhr.status >= 200 && xhr.status < 300) {
                              try {
                                const responseData = JSON.parse(xhr.responseText);
                                console.log('[XHR] 风扇开启响应:', responseData);
                                
                                if (responseData.status === 'success') {
                                  setFanStatus(true);
                                  updateCnsField({ 风扇状态: '1' });
                                  addLogEntry('手动控制-风扇开启', 'success', '风扇', '开启', '已开启风扇');
                                } else {
                                  console.error('[XHR] 风扇开启失败:', responseData.message);
                                  addLogEntry('手动控制-风扇开启', 'error', '风扇', '失败', `风扇开启失败: ${responseData.message}`);
                                }
                              } catch (e) {
                                console.error('[XHR] 解析响应失败:', e);
                              }
                            }
                          };
                          
                          xhr.onerror = function() {
                            console.error('[XHR] 风扇开启请求失败 - 网络错误');
                          };
                          
                          xhr.send(JSON.stringify({}));
                        } catch (error) {
                          console.error('风扇开启请求失败:', error);
                        }
                      }}
                    >
                      开启
                    </button>
                    <button 
                      className={`control-btn off ${!fanStatus ? 'active' : ''}`}
                      onClick={async () => {
                        try {
                          const endpoint = 'stff';
                          console.log(`正在发送风扇关闭请求到: ${CONTROL_API_URL}${endpoint}`);
                          
                          // 使用XMLHttpRequest直接发送请求
                          const xhr = new XMLHttpRequest();
                          xhr.open('POST', `${CONTROL_API_URL}${endpoint}`, true);
                          xhr.setRequestHeader('Content-Type', 'application/json');
                          xhr.setRequestHeader('X-Auto-Control', 'false');
                          
                          xhr.onload = function() {
                            if (xhr.status >= 200 && xhr.status < 300) {
                              try {
                                const responseData = JSON.parse(xhr.responseText);
                                console.log('[XHR] 风扇关闭响应:', responseData);
                                
                                if (responseData.status === 'success') {
                                  setFanStatus(false);
                                  updateCnsField({ 风扇状态: '0' });
                                  addLogEntry('手动控制-风扇关闭', 'success', '风扇', '关闭', '已关闭风扇');
                                } else {
                                  console.error('[XHR] 风扇关闭失败:', responseData.message);
                                  addLogEntry('手动控制-风扇关闭', 'error', '风扇', '失败', `风扇关闭失败: ${responseData.message}`);
                                }
                              } catch (e) {
                                console.error('[XHR] 解析响应失败:', e);
                              }
                            }
                          };
                          
                          xhr.onerror = function() {
                            console.error('[XHR] 风扇关闭请求失败 - 网络错误');
                          };
                          
                          xhr.send(JSON.stringify({}));
                        } catch (error) {
                          console.error('风扇关闭请求失败:', error);
                        }
                      }}
                    >
                      关闭
                    </button>
                  </div>
                  <div className="slider-container">
                    <label>风扇速度: {tempFanSpeed}% (当前: {fanSpeed}%)</label>
                    <input 
                      type="range" 
                      min="0" 
                      max="100" 
                      value={tempFanSpeed} 
                      onChange={(e) => setTempFanSpeed(parseInt(e.target.value))}
                      style={{
                        background: `linear-gradient(90deg, #4e7cf6 ${tempFanSpeed}%, rgba(30,50,80,0.6) ${tempFanSpeed}%)`
                      } as React.CSSProperties}
                    />
                    <button 
                      className="confirm-btn"
                      onClick={async () => {
                        try {
                          const speedValue = Math.round(tempFanSpeed);
                          const endpoint = `setFanSpeed/${speedValue}`;
                          console.log(`正在发送风扇速度设置请求到: ${CONTROL_API_URL}${endpoint}`);
                          
                          // 使用XMLHttpRequest直接发送请求
                          const xhr = new XMLHttpRequest();
                          xhr.open('POST', `${CONTROL_API_URL}${endpoint}`, true);
                          xhr.setRequestHeader('Content-Type', 'application/json');
                          xhr.setRequestHeader('X-Auto-Control', 'false');
                          
                          xhr.onload = function() {
                            if (xhr.status >= 200 && xhr.status < 300) {
                              try {
                                const responseData = JSON.parse(xhr.responseText);
                                console.log('[XHR] 风扇速度设置响应:', responseData);
                                
                                if (responseData.status === 'success') {
                                  setFanSpeed(speedValue);
                                  updateCnsField({ 风扇速度: String(speedValue) });
                                  addLogEntry('手动控制-风扇速度设置', 'success', '风扇', '速度调整', `风扇速度已设置为${speedValue}`);
                                } else {
                                  console.error('[XHR] 风扇速度设置失败:', responseData.message);
                                  addLogEntry('手动控制-风扇速度设置', 'error', '风扇', '失败', `风扇速度设置失败: ${responseData.message}`);
                                }
                              } catch (e) {
                                console.error('[XHR] 解析响应失败:', e);
                              }
                            }
                          };
                          
                          xhr.onerror = function() {
                            console.error('[XHR] 风扇速度设置请求失败 - 网络错误');
                          };
                          
                          xhr.send(JSON.stringify({}));
                        } catch (error) {
                          console.error('风扇速度设置请求失败:', error);
                        }
                      }}
                      disabled={tempFanSpeed === fanSpeed}
                    >
                      确认更改
                    </button>
                  </div>
                </div>

                <div className="control-card">
                  <h3><span className="card-icon">💡</span> 生长灯控制</h3>
                  <div className="status-display">
                    <span className="status-label">当前状态:</span>
                    <span className={`status-value ${lightStatus ? 'on' : 'off'}`}>
                      {lightStatus ? '照明中' : '已关闭'}
                    </span>
                  </div>
                  <div className="control-buttons">
                    <button 
                      className={`control-btn on ${lightStatus ? 'active' : ''}`}
                      onClick={async () => {
                        try {
                          const endpoint = 'stgt';
                          console.log(`正在发送生长灯开启请求到: ${CONTROL_API_URL}${endpoint}`);
                          
                          // 使用XMLHttpRequest直接发送请求
                          const xhr = new XMLHttpRequest();
                          xhr.open('POST', `${CONTROL_API_URL}${endpoint}`, true);
                          xhr.setRequestHeader('Content-Type', 'application/json');
                          xhr.setRequestHeader('X-Auto-Control', 'false');
                          
                          xhr.onload = function() {
                            if (xhr.status >= 200 && xhr.status < 300) {
                              try {
                                const responseData = JSON.parse(xhr.responseText);
                                console.log('[XHR] 生长灯开启响应:', responseData);
                                
                                if (responseData.status === 'success') {
                                  setLightStatus(true);
                                  updateCnsField({ 生长灯状态: '1' });
                                  addLogEntry('手动控制-生长灯开启', 'success', '生长灯', '开启', '已开启生长灯');
                                } else {
                                  console.error('[XHR] 生长灯开启失败:', responseData.message);
                                  addLogEntry('手动控制-生长灯开启', 'error', '生长灯', '失败', `生长灯开启失败: ${responseData.message}`);
                                }
                              } catch (e) {
                                console.error('[XHR] 解析响应失败:', e);
                              }
                            }
                          };
                          
                          xhr.onerror = function() {
                            console.error('[XHR] 生长灯开启请求失败 - 网络错误');
                          };
                          
                          xhr.send(JSON.stringify({}));
                        } catch (error) {
                          console.error('生长灯开启请求失败:', error);
                        }
                      }}
                    >
                      开启
                    </button>
                    <button 
                      className={`control-btn off ${!lightStatus ? 'active' : ''}`}
                      onClick={async () => {
                        try {
                          const endpoint = 'stgf';
                          console.log(`正在发送生长灯关闭请求到: ${CONTROL_API_URL}${endpoint}`);
                          
                          // 使用XMLHttpRequest直接发送请求
                          const xhr = new XMLHttpRequest();
                          xhr.open('POST', `${CONTROL_API_URL}${endpoint}`, true);
                          xhr.setRequestHeader('Content-Type', 'application/json');
                          xhr.setRequestHeader('X-Auto-Control', 'false');
                          
                          xhr.onload = function() {
                            if (xhr.status >= 200 && xhr.status < 300) {
                              try {
                                const responseData = JSON.parse(xhr.responseText);
                                console.log('[XHR] 生长灯关闭响应:', responseData);
                                
                                if (responseData.status === 'success') {
                                  setLightStatus(false);
                                  updateCnsField({ 生长灯状态: '0' });
                                  addLogEntry('手动控制-生长灯关闭', 'success', '生长灯', '关闭', '已关闭生长灯');
                                } else {
                                  console.error('[XHR] 生长灯关闭失败:', responseData.message);
                                  addLogEntry('手动控制-生长灯关闭', 'error', '生长灯', '失败', `生长灯关闭失败: ${responseData.message}`);
                                }
                              } catch (e) {
                                console.error('[XHR] 解析响应失败:', e);
                              }
                            }
                          };
                          
                          xhr.onerror = function() {
                            console.error('[XHR] 生长灯关闭请求失败 - 网络错误');
                          };
                          
                          xhr.send(JSON.stringify({}));
                        } catch (error) {
                          console.error('生长灯关闭请求失败:', error);
                        }
                      }}
                    >
                      关闭
                    </button>
                  </div>
                </div>

                <div className="control-card">
                  <h3><span className="card-icon">💧</span> 水泵控制</h3>
                  <div className="status-display">
                    <span className="status-label">当前状态:</span>
                    <span className={`status-value ${pumpStatus ? 'on' : 'off'}`}>
                      {pumpStatus ? '运行中' : '已停止'}
                    </span>
                  </div>
                  <div className="control-buttons">
                    <button 
                      className={`control-btn on ${pumpStatus ? 'active' : ''}`}
                      onClick={async () => {
                        try {
                          const endpoint = 'stpt';
                          console.log(`正在发送水泵开启请求到: ${CONTROL_API_URL}${endpoint}`);
                          
                          // 使用XMLHttpRequest直接发送请求
                          const xhr = new XMLHttpRequest();
                          xhr.open('POST', `${CONTROL_API_URL}${endpoint}`, true);
                          xhr.setRequestHeader('Content-Type', 'application/json');
                          xhr.setRequestHeader('X-Auto-Control', 'false');
                          
                          xhr.onload = function() {
                            if (xhr.status >= 200 && xhr.status < 300) {
                              try {
                                const responseData = JSON.parse(xhr.responseText);
                                console.log('[XHR] 水泵开启响应:', responseData);
                                
                                if (responseData.status === 'success') {
                                  setPumpStatus(true);
                                  updateCnsField({ 水泵状态: '1' });
                                  addLogEntry('手动控制-水泵开启', 'success', '水泵', '开启', '已开启水泵');
                                } else {
                                  console.error('[XHR] 水泵开启失败:', responseData.message);
                                  addLogEntry('手动控制-水泵开启', 'error', '水泵', '失败', `水泵开启失败: ${responseData.message}`);
                                }
                              } catch (e) {
                                console.error('[XHR] 解析响应失败:', e);
                              }
                            }
                          };
                          
                          xhr.onerror = function() {
                            console.error('[XHR] 水泵开启请求失败 - 网络错误');
                          };
                          
                          xhr.send(JSON.stringify({}));
                        } catch (error) {
                          console.error('水泵开启请求失败:', error);
                        }
                      }}
                    >
                      开启
                    </button>
                    <button 
                      className={`control-btn off ${!pumpStatus ? 'active' : ''}`}
                      onClick={async () => {
                        try {
                          const endpoint = 'stpf';
                          console.log(`正在发送水泵关闭请求到: ${CONTROL_API_URL}${endpoint}`);
                          
                          // 使用XMLHttpRequest直接发送请求
                          const xhr = new XMLHttpRequest();
                          xhr.open('POST', `${CONTROL_API_URL}${endpoint}`, true);
                          xhr.setRequestHeader('Content-Type', 'application/json');
                          xhr.setRequestHeader('X-Auto-Control', 'false');
                          
                          xhr.onload = function() {
                            if (xhr.status >= 200 && xhr.status < 300) {
                              try {
                                const responseData = JSON.parse(xhr.responseText);
                                console.log('[XHR] 水泵关闭响应:', responseData);
                                
                                if (responseData.status === 'success') {
                                  setPumpStatus(false);
                                  updateCnsField({ 水泵状态: '0' });
                                  addLogEntry('手动控制-水泵关闭', 'success', '水泵', '关闭', '已关闭水泵');
                                } else {
                                  console.error('[XHR] 水泵关闭失败:', responseData.message);
                                  addLogEntry('手动控制-水泵关闭', 'error', '水泵', '失败', `水泵关闭失败: ${responseData.message}`);
                                }
                              } catch (e) {
                                console.error('[XHR] 解析响应失败:', e);
                              }
                            }
                          };
                          
                          xhr.onerror = function() {
                            console.error('[XHR] 水泵关闭请求失败 - 网络错误');
                          };
                          
                          xhr.send(JSON.stringify({}));
                        } catch (error) {
                          console.error('水泵关闭请求失败:', error);
                        }
                      }}
                    >
                      关闭
                    </button>
                  </div>
                  <div className="slider-container">
                    <label>水泵速度: {tempPumpSpeed}% (当前: {pumpSpeed}%)</label>
                    <input 
                      type="range" 
                      min="0" 
                      max="100" 
                      value={tempPumpSpeed} 
                      onChange={(e) => setTempPumpSpeed(parseInt(e.target.value))}
                      style={{
                        background: `linear-gradient(90deg, #4e7cf6 ${tempPumpSpeed}%, rgba(30,50,80,0.6) ${tempPumpSpeed}%)`
                      } as React.CSSProperties}
                    />
                    <button 
                      className="confirm-btn"
                      onClick={async () => {
                        try {
                          const speedValue = Math.round(tempPumpSpeed);
                          const endpoint = `setPumpSpeed/${speedValue}`;
                          console.log(`正在发送水泵速度设置请求到: ${CONTROL_API_URL}${endpoint}`);
                          
                          // 使用XMLHttpRequest直接发送请求
                          const xhr = new XMLHttpRequest();
                          xhr.open('POST', `${CONTROL_API_URL}${endpoint}`, true);
                          xhr.setRequestHeader('Content-Type', 'application/json');
                          xhr.setRequestHeader('X-Auto-Control', 'false');
                          
                          xhr.onload = function() {
                            if (xhr.status >= 200 && xhr.status < 300) {
                              try {
                                const responseData = JSON.parse(xhr.responseText);
                                console.log('[XHR] 水泵速度设置响应:', responseData);
                                
                                if (responseData.status === 'success') {
                                  setPumpSpeed(speedValue);
                                  updateCnsField({ 水泵速度: String(speedValue) });
                                  addLogEntry('手动控制-水泵速度设置', 'success', '水泵', '速度调整', `水泵速度已设置为${speedValue}`);
                                } else {
                                  console.error('[XHR] 水泵速度设置失败:', responseData.message);
                                  addLogEntry('手动控制-水泵速度设置', 'error', '水泵', '失败', `水泵速度设置失败: ${responseData.message}`);
                                }
                              } catch (e) {
                                console.error('[XHR] 解析响应失败:', e);
                              }
                            }
                          };
                          
                          xhr.onerror = function() {
                            console.error('[XHR] 水泵速度设置请求失败 - 网络错误');
                          };
                          
                          xhr.send(JSON.stringify({}));
                        } catch (error) {
                          console.error('水泵速度设置请求失败:', error);
                        }
                      }}
                      disabled={tempPumpSpeed === pumpSpeed}
                    >
                      确认更改
                    </button>
                  </div>
                </div>

                <div className="control-card">
                  <h3><span className="card-icon">🤖</span> 自动控制</h3>
                  <div className="status-display">
                    <span className="status-label">当前状态:</span>
                    <span className={`status-value ${autoControl ? 'on' : 'off'}`}>
                      {autoControl ? '已启用' : '已禁用'}
                    </span>
                  </div>
                  <div className="control-buttons">
                    <button 
                      className={`control-btn on ${autoControl ? 'active' : ''}`}
                      onClick={() => {
                        setAutoControl(true);
                        addLogEntry('自动控制状态切换', 'info', '系统', '启用', '用户启用了自动控制功能');
                        showNotification('自动控制已启用', 'info');
                      }}
                    >
                      启用
                    </button>
                    <button 
                      className={`control-btn off ${!autoControl ? 'active' : ''}`}
                      onClick={() => {
                        setAutoControl(false);
                        addLogEntry('自动控制状态切换', 'info', '系统', '禁用', '用户禁用了自动控制功能');
                        showNotification('自动控制已禁用', 'info');
                      }}
                    >
                      禁用
                    </button>
                  </div>
                  <div className="settings-link-container">
                    <Link to="/auto-control-settings" className="settings-link">
                      自动控制设置
                    </Link>
                  </div>
                </div>
              </div>
            </>
          ) : (
            <div className="history-data-container">
              <section className="history-chart-section">
                <HistoryChart 
                  dns={dns}
                  dns2={dns2}
                  ens={ens}
                  displayCount={displayCount}
                />
              </section>
              <section className="history-data-table-container">
                <HistoryDataTable 
                  dns={dns}
                  dns2={dns2}
                  ens={ens}
                />
              </section>
            </div>
          )}
        </div>

        <footer className="dashboard-footer">
          <div className="footer-content">
            <p className="copyright">智慧农业物联网可视化监控平台 &copy; {new Date().getFullYear()}</p>
            <div className="authentication">
              <p className="auth-info"> · 开发团队：黄浩、王伟</p>
              <p className="contact">联系邮箱：2768607063@163.com</p>
            </div>
          </div>
        </footer>
      </div>
    );
  };

  return (
    <Router>
      {/* 通知组件 */}
      <NotificationToast 
        notifications={notifications}
        removeNotification={removeNotification}
      />
      
      <Routes>
        <Route path="/login" element={<LoginPage />} />
        <Route
          path="/"
          element={
            <PrivateRoute>
              <Dashboard />
            </PrivateRoute>
          }
        />
        <Route path="/threshold-settings" element={
          <PrivateRoute>
            <ThresholdSettings 
              dataThresholds={dataThresholds}
              setDataThresholds={setDataThresholds}
              dns={dns}
              dns2={dns2}
            />
          </PrivateRoute>
        } />
        <Route path="/auto-control-settings" element={
          <PrivateRoute>
            <AutoControlSettings
              thresholds={{
                temperature: autoSettings.temperatureThreshold,
                humidity: 70, // 默认湿度阈值
                light: autoSettings.lightThreshold,
                soilMoisture: 30, // 默认土壤湿度阈值
                soilEC: autoSettings.soilSalinityThreshold
              }}
              isSaving={false}
              saveSuccess={false}
              saveError={null}
              onSave={(newSettings) => {
                // 只在用户点击保存按钮时更新自动控制设置
                setAutoSettings({
                  ...autoSettings,
                  temperatureThreshold: newSettings.temperature,
                  fanSpeed: newSettings.fanSpeed * 10,
                  lightThreshold: newSettings.light,
                  soilSalinityThreshold: newSettings.soilEC,
                  pumpSpeed: newSettings.pumpSpeed * 10
                });
                showNotification('自动控制设置已保存', 'success');
              }}
              currentValues={{
                temperature: dns[0]?.室内温度 ? parseFloat(dns[0]?.室内温度) : 0,
                humidity: dns[0]?.室内湿度 ? parseFloat(dns[0]?.室内湿度) : 0,
                light: dns[0]?.光照强度 ? parseFloat(dns[0]?.光照强度) : 0,
                soilMoisture: dns[0]?.土壤湿度 ? parseFloat(dns[0]?.土壤湿度) : 0,
                soilEC: dns2[0]?.土壤盐度 ? parseFloat(dns2[0]?.土壤盐度) : 0
              }}
              autoControlEnabled={autoControl}
              onToggleAutoControl={(enabled) => setAutoControl(enabled)}
            />
          </PrivateRoute>
        } />
        <Route path="/system-log" element={
          <PrivateRoute>
            <SystemLog 
              logs={systemLogs}
              clearLogs={clearLogs}
            />
          </PrivateRoute>
        } />
      </Routes>
      <div className="watermark">
        <div className="watermark-content">
          <br/>
          黄浩、王伟<br/>
          2768607063@163.com
        </div>
      </div>
    </Router>
  );
}

export default App;
