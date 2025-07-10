import React, { useState, useEffect, useMemo } from 'react';
import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
  Filler,
  ChartOptions
} from 'chart.js';
import { Line } from 'react-chartjs-2';
import { formatTime } from '../utils/timeFormatter';

// 注册Chart.js组件
ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
  Filler
);

/**
 * 历史数据图表组件
 * 
 * 此组件显示从server.ts服务器获取的真实历史数据，而非模拟数据。
 * 数据通过App.tsx中的loadData函数从HISTORY_API_URL获取，
 * 然后通过props传递给此组件进行展示。
 * 
 * 支持多种数据类型的展示：
 * - 室内环境数据：温度、湿度、光照、VOC浓度、CO2浓度
 * - 土壤数据：湿度、温度、导电率、PH值、养分含量等
 * - 室外环境数据：温度、湿度、光照、压强等
 */

// 数据项定义，与App.tsx保持一致
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

interface ENS {
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

// 可选择的数据项定义
interface DataOption {
  value: string;
  label: string;
  dataType: 'dns' | 'dns2' | 'ens';
  unit: string;
  color: string;
  gradientFrom?: string;
  gradientTo?: string;
  category?: string;
}

// 组件属性接口
interface HistoryChartProps {
  dns: DNS[];
  dns2: DNS_2[];
  ens: ENS[];
  displayCount: number;
}

// 格式化时间函数
const formatChartTime = (timeStr: string): string => {
  if (!timeStr) return '';
  
  // 先使用通用的时间格式化函数转换为标准格式，再提取时间部分
  const formattedTime = formatTime(String(timeStr), 'YYYY-MM-DD HH:mm:ss');
  // 只返回时间部分
  return formattedTime.split(' ')[1] || '';
};

// 按时间排序函数
const sortByTimeAndNumber = (a: any, b: any) => {
  // 首先按序号排序
  const aNum = Number(a.序号 || 0);
  const bNum = Number(b.序号 || 0);
  
  if (aNum !== bNum) {
    return aNum - bNum;
  }
  
  // 如果序号相同，按时间排序
  const aTime = a.上报时间 || '';
  const bTime = b.上报时间 || '';
  
  // 使用标准化的时间进行比较
  const aStandardTime = formatTime(aTime);
  const bStandardTime = formatTime(bTime);
  
  return new Date(aStandardTime).getTime() - new Date(bStandardTime).getTime();
};

const HistoryChart: React.FC<HistoryChartProps> = ({ dns, dns2, ens, displayCount }) => {
  // 所有可选数据项，添加渐变色和分类
  const dataOptions: DataOption[] = [
    // 室内环境数据
    { value: '室内温度', label: '室内温度', dataType: 'dns', unit: '°C', color: '#0c5d8f', gradientFrom: '#052c40', gradientTo: 'rgba(12, 93, 143, 0.1)', category: '室内环境' },
    { value: '室内湿度', label: '室内湿度', dataType: 'dns', unit: '%', color: '#2e90c7', gradientFrom: '#0c5d8f', gradientTo: 'rgba(46, 144, 199, 0.1)', category: '室内环境' },
    { value: '光照强度', label: '光照强度', dataType: 'dns', unit: 'lx', color: '#fbbf24', gradientFrom: '#f59e0b', gradientTo: 'rgba(251, 191, 36, 0.1)', category: '室内环境' },
    { value: '挥发性有机化合物浓度', label: 'VOC浓度', dataType: 'dns', unit: 'ppm', color: '#a78bfa', gradientFrom: '#8b5cf6', gradientTo: 'rgba(167, 139, 250, 0.1)', category: '室内环境' },
    { value: '二氧化碳浓度', label: '二氧化碳浓度', dataType: 'dns', unit: 'ppm', color: '#fb7185', gradientFrom: '#f43f5e', gradientTo: 'rgba(251, 113, 133, 0.1)', category: '室内环境' },
    
    // 土壤数据
    { value: '土壤湿度', label: '土壤湿度', dataType: 'dns', unit: '%', color: '#2e90c7', gradientFrom: '#0c5d8f', gradientTo: 'rgba(46, 144, 199, 0.1)', category: '土壤数据' },
    { value: '土壤温度', label: '土壤温度', dataType: 'dns', unit: '°C', color: '#94a3b8', gradientFrom: '#64748b', gradientTo: 'rgba(148, 163, 184, 0.1)', category: '土壤数据' },
    { value: '土壤导电率', label: '土壤导电率', dataType: 'dns2', unit: 'mS/cm', color: '#f472b6', gradientFrom: '#ec4899', gradientTo: 'rgba(244, 114, 182, 0.1)', category: '土壤数据' },
    { value: '土壤PH值', label: '土壤PH值', dataType: 'dns2', unit: '', color: '#2dd4bf', gradientFrom: '#14b8a6', gradientTo: 'rgba(45, 212, 191, 0.1)', category: '土壤数据' },
    { value: '土壤含氮量', label: '土壤含氮量', dataType: 'dns2', unit: 'mg/kg', color: '#34d399', gradientFrom: '#10b981', gradientTo: 'rgba(52, 211, 153, 0.1)', category: '土壤数据' },
    { value: '土壤含钾量', label: '土壤含钾量', dataType: 'dns2', unit: 'mg/kg', color: '#facc15', gradientFrom: '#eab308', gradientTo: 'rgba(250, 204, 21, 0.1)', category: '土壤数据' },
    { value: '土壤含磷量', label: '土壤含磷量', dataType: 'dns2', unit: 'mg/kg', color: '#c084fc', gradientFrom: '#a855f7', gradientTo: 'rgba(192, 132, 252, 0.1)', category: '土壤数据' },
    { value: '土壤盐度', label: '土壤盐度', dataType: 'dns2', unit: 'g/kg', color: '#fb923c', gradientFrom: '#f97316', gradientTo: 'rgba(251, 146, 60, 0.1)', category: '土壤数据' },
    { value: '土壤总溶解固体', label: '土壤TDS', dataType: 'dns2', unit: 'ppm', color: '#818cf8', gradientFrom: '#6366f1', gradientTo: 'rgba(129, 140, 248, 0.1)', category: '土壤数据' },
    
    // 室外环境数据
    { value: '室外温度', label: '室外温度', dataType: 'ens', unit: '°C', color: '#f87171', gradientFrom: '#ef4444', gradientTo: 'rgba(248, 113, 113, 0.1)', category: '室外环境' },
    { value: '室外湿度', label: '室外湿度', dataType: 'ens', unit: '%', color: '#2e90c7', gradientFrom: '#0c5d8f', gradientTo: 'rgba(46, 144, 199, 0.1)', category: '室外环境' },
    { value: '室外光照强度', label: '室外光照', dataType: 'ens', unit: 'lx', color: '#fbbf24', gradientFrom: '#f59e0b', gradientTo: 'rgba(251, 191, 36, 0.1)', category: '室外环境' },
    { value: '室外压强', label: '室外压强', dataType: 'ens', unit: 'hPa', color: '#a78bfa', gradientFrom: '#8b5cf6', gradientTo: 'rgba(167, 139, 250, 0.1)', category: '室外环境' },
    { value: '海拔高度', label: '海拔高度', dataType: 'ens', unit: 'm', color: '#94a3b8', gradientFrom: '#64748b', gradientTo: 'rgba(148, 163, 184, 0.1)', category: '室外环境' },
  ];

  // 状态管理
  const [selectedDataItems, setSelectedDataItems] = useState<string[]>(['室内温度']);
  const [categoryFilter, setCategoryFilter] = useState<string>('全部');
  const [showControls, setShowControls] = useState<boolean>(false);
  
  // 数据分类
  const categories = ['全部', '室内环境', '土壤数据', '室外环境'];

  // 处理数据项选择
  const handleDataItemToggle = (value: string) => {
    setSelectedDataItems(prev => {
      if (prev.includes(value)) {
        return prev.filter(item => item !== value);
      } else {
        return [...prev, value];
      }
    });
  };

  // 处理分类过滤
  const handleCategoryFilter = (category: string) => {
    setCategoryFilter(category);
  };

  // 过滤数据选项
  const filteredDataOptions = dataOptions.filter(option => 
    categoryFilter === '全部' || option.category === categoryFilter
  );

  // 准备图表数据
  const prepareChartData = () => {
    // 对数据进行排序
    const sortedDns = [...dns].sort(sortByTimeAndNumber);
    const sortedDns2 = [...dns2].sort(sortByTimeAndNumber);
    const sortedEns = [...ens].sort(sortByTimeAndNumber);
    
    // 限制显示的数据点数量
    const limitedDns = sortedDns.slice(-displayCount);
    const limitedDns2 = sortedDns2.slice(-displayCount);
    const limitedEns = sortedEns.slice(-displayCount);

    // 提取时间标签
    let labels: string[] = [];
      
    // 优先使用DNS数据作为时间轴
    if (limitedDns.length > 0) {
      labels = limitedDns.map(item => formatChartTime(item.上报时间));
    } else if (limitedDns2.length > 0) {
      labels = limitedDns2.map(item => formatChartTime(item.上报时间));
    } else if (limitedEns.length > 0) {
      labels = limitedEns.map(item => formatChartTime(item.上报时间));
      }
      
    // 准备数据集
    const datasets = selectedDataItems.map(dataItemKey => {
      const dataOption = dataOptions.find(option => option.value === dataItemKey);
      
        if (!dataOption) return null;

        let data: number[] = [];
      
      // 根据数据类型获取对应数据
      if (dataOption.dataType === 'dns') {
        data = limitedDns.map(item => parseFloat(item[dataItemKey as keyof DNS] as string) || 0);
      } else if (dataOption.dataType === 'dns2') {
        data = limitedDns2.map(item => parseFloat(item[dataItemKey as keyof DNS_2] as string) || 0);
      } else if (dataOption.dataType === 'ens') {
        data = limitedEns.map(item => parseFloat(item[dataItemKey as keyof ENS] as string) || 0);
      }
      
          return {
        label: `${dataOption.label} (${dataOption.unit})`,
        data,
            borderColor: dataOption.color,
        backgroundColor: function(context: any) {
          const chart = context.chart;
          const {ctx, chartArea} = chart;
          
          if (!chartArea) {
            return;
          }
          
          // 创建渐变背景
          const gradient = ctx.createLinearGradient(0, chartArea.bottom, 0, chartArea.top);
          gradient.addColorStop(0, dataOption.gradientTo || 'rgba(255, 255, 255, 0)');
          gradient.addColorStop(1, dataOption.gradientFrom || dataOption.color);
          
          return gradient;
          },
        borderWidth: 2,
        pointRadius: 3,
        pointHoverRadius: 5,
          pointBackgroundColor: dataOption.color,
          pointBorderColor: '#fff',
        pointHoverBackgroundColor: '#fff',
        pointHoverBorderColor: dataOption.color,
          tension: 0.4,
        fill: true
        };
    }).filter(Boolean);

    return {
      labels,
      datasets: datasets as any[]
    };
  };

  // 图表配置
  const options: ChartOptions<'line'> = {
    responsive: true,
    maintainAspectRatio: false,
    interaction: {
      mode: 'index',
      intersect: false,
    },
    plugins: {
      legend: {
        position: 'top' as const,
        labels: {
          boxWidth: 12,
          usePointStyle: true,
          pointStyle: 'circle',
          color: '#a3d5f0',
          font: {
            size: 11
          }
        }
      },
      tooltip: {
        backgroundColor: 'rgba(5, 44, 64, 0.85)',
        titleColor: '#fff',
        bodyColor: '#e2e8f0',
        borderColor: 'rgba(12, 93, 143, 0.5)',
        borderWidth: 1,
        padding: 10,
        cornerRadius: 6,
        displayColors: true,
        usePointStyle: true,
        boxWidth: 8,
        boxHeight: 8,
        boxPadding: 4,
        titleFont: {
          size: 13,
          weight: 'bold'
        },
        bodyFont: {
          size: 12
        },
        callbacks: {
          title: function(context) {
            return context[0].label ? `时间: ${context[0].label}` : '';
          }
        }
      }
    },
    scales: {
      x: {
        grid: {
          color: 'rgba(12, 93, 143, 0.15)',
          tickLength: 0
        },
        ticks: {
          color: '#a3d5f0',
          font: {
            size: 10
          }
          },
        title: {
          display: true,
          text: '时间',
          color: '#2e90c7',
          font: {
            size: 12,
            weight: 'bold'
          }
        }
      },
      y: {
        grid: {
          color: 'rgba(12, 93, 143, 0.15)',
        },
        ticks: {
          color: '#a3d5f0',
          font: {
            size: 10
          }
        },
        title: {
          display: true,
          text: '数值',
          color: '#2e90c7',
          font: {
            size: 12,
            weight: 'bold'
      }
    }
      }
    },
    animation: {
      duration: 1000,
      easing: 'easeOutQuart'
    }
  };
  
  // 渲染组件
  return (
    <div className="history-chart-container">
      <div className="chart-header">
        <h2>历史数据趋势</h2>
        <button 
          className="toggle-controls-btn"
          onClick={() => setShowControls(prev => !prev)}
        >
          {showControls ? '隐藏控制面板' : '显示控制面板'}
        </button>
      </div>
      
      {showControls && (
        <div className="chart-controls">
          <div className="category-filters">
            <h3>数据分类</h3>
            <div className="category-buttons">
              {categories.map(category => (
                <button
                  key={category}
                  className={`category-btn ${categoryFilter === category ? 'active' : ''}`}
                  onClick={() => handleCategoryFilter(category)}
                >
                  {category}
                </button>
              ))}
              {selectedDataItems.length > 0 && (
                <button
                  className="category-btn clear"
                  onClick={() => setSelectedDataItems([])}
                >
                  清除选择
                </button>
              )}
            </div>
          </div>
          
          <div className="data-options">
            <h3>选择数据项 <span className="selected-count">({selectedDataItems.length}项已选)</span></h3>
          <div className="data-options-grid">
              {filteredDataOptions.map((option) => (
              <div 
                key={option.value}
                  className={`data-option-card ${selectedDataItems.includes(option.value) ? 'selected' : ''}`}
                  onClick={() => handleDataItemToggle(option.value)}
                style={{
                    borderColor: selectedDataItems.includes(option.value) ? option.color : 'transparent'
                }}
              >
                  <div className="data-option-content">
                    <span className="data-option-color" style={{ backgroundColor: option.color }}></span>
                    <span className="data-option-label">{option.label}</span>
                    <span className="data-option-unit">{option.unit}</span>
                </div>
              </div>
            ))}
          </div>
          </div>
        </div>
      )}
      
      <div className="chart-wrapper">
        {selectedDataItems.length > 0 ? (
          <Line data={prepareChartData()} options={options} height={350} />
        ) : (
          <div className="no-data-message">请选择至少一项数据进行显示</div>
        )}
      </div>
    </div>
  );
};

// 使用React.memo包装组件，添加自定义比较函数，避免不必要的重新渲染
export default React.memo(HistoryChart, (prevProps, nextProps) => {
  // 只有当数据数量或displayCount发生变化时才重新渲染
  const prevDnsLength = prevProps.dns.length;
  const nextDnsLength = nextProps.dns.length;
  
  const prevDns2Length = prevProps.dns2.length;
  const nextDns2Length = nextProps.dns2.length;
  
  const prevEnsLength = prevProps.ens.length;
  const nextEnsLength = nextProps.ens.length;
  
  const displayCountChanged = prevProps.displayCount !== nextProps.displayCount;
  
  // 如果数据数量或显示数量没有变化，则不重新渲染
  return prevDnsLength === nextDnsLength && 
         prevDns2Length === nextDns2Length && 
         prevEnsLength === nextEnsLength && 
         !displayCountChanged;
}); 