import React, { useState, useEffect } from 'react';
import { formatTime } from '../utils/timeFormatter';

// 添加调试代码，确保时间格式化正常工作
console.log("测试时间格式化:", formatTime("20250708T071251Z"));
console.log("测试时间格式化:", formatTime("20250708T043301Z"));
console.log("测试时间格式化:", formatTime("2025-07-08 07:12:51"));

/**
 * 历史数据表格组件
 * 
 * 此组件显示从server.ts服务器获取的真实历史数据，而非模拟数据。
 * 数据通过App.tsx中的loadData函数从HISTORY_API_URL获取，
 * 然后通过props传递给此组件进行表格展示。
 * 
 * 特性：
 * - 支持三种数据类型：大棚内环境数据、土壤成分数据、大棚外环境数据
 * - 可按任何字段排序（升序/降序）
 * - 支持分页浏览和调整每页显示记录数
 * - 支持文本搜索过滤
 * - 适配不同数据类型的表头和单位显示
 */

// 数据接口定义，与App.tsx保持一致
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

// 组件属性接口
interface HistoryDataTableProps {
  dns: DNS[];
  dns2: DNS_2[];
  ens: ENS[];
}

// 表格类型
type TableType = 'dns' | 'dns2' | 'ens';

const HistoryDataTable: React.FC<HistoryDataTableProps> = ({ dns, dns2, ens }) => {
  // 当前选择的表格类型
  const [activeTable, setActiveTable] = useState<TableType>('dns');
  
  // 分页状态
  const [currentPage, setCurrentPage] = useState<number>(1);
  const [itemsPerPage, setItemsPerPage] = useState<number>(10);
  
  // 排序状态
  const [sortField, setSortField] = useState<string>('序号');
  const [sortDirection, setSortDirection] = useState<'asc' | 'desc'>('desc');
  
  // 搜索状态
  const [searchTerm, setSearchTerm] = useState<string>('');
  
  // 表格标题映射
  const tableHeaders = {
    dns: [
      { field: '序号', label: '序号' },
      { field: '设备号', label: '设备号' },
      { field: '室内温度', label: '室内温度', unit: '°C' },
      { field: '室内湿度', label: '室内湿度', unit: '%' },
      { field: '光照强度', label: '光照强度', unit: 'lx' },
      { field: '土壤湿度', label: '土壤湿度', unit: '%' },
      { field: '挥发性有机化合物浓度', label: 'VOC浓度', unit: 'ppm' },
      { field: '二氧化碳浓度', label: 'CO₂浓度', unit: 'ppm' },
      { field: '土壤温度', label: '土壤温度', unit: '°C' },
      { field: '上报时间', label: '上报时间' }
    ],
    dns2: [
      { field: '序号', label: '序号' },
      { field: '设备号', label: '设备号' },
      { field: '土壤导电率', label: '土壤导电率', unit: 'mS/cm' },
      { field: '土壤PH值', label: '土壤PH值' },
      { field: '土壤含氮量', label: '土壤含氮量', unit: 'mg/kg' },
      { field: '土壤含钾量', label: '土壤含钾量', unit: 'mg/kg' },
      { field: '土壤含磷量', label: '土壤含磷量', unit: 'mg/kg' },
      { field: '土壤盐度', label: '土壤盐度', unit: 'g/kg' },
      { field: '土壤总溶解固体', label: '土壤TDS', unit: 'ppm' },
      { field: '土壤肥力', label: '土壤肥力' },
      { field: '上报时间', label: '上报时间' }
    ],
    ens: [
      { field: '序号', label: '序号' },
      { field: '设备号', label: '设备号' },
      { field: '室外温度', label: '室外温度', unit: '°C' },
      { field: '室外湿度', label: '室外湿度', unit: '%' },
      { field: '室外光照强度', label: '室外光照强度', unit: 'lx' },
      { field: '室外压强', label: '室外压强', unit: 'hPa' },
      { field: '位置', label: '位置' },
      { field: '海拔高度', label: '海拔高度', unit: 'm' },
      { field: '上报时间', label: '上报时间' }
    ]
  };

  // 获取当前表格数据
  const getCurrentData = () => {
    let data: any[] = [];
    switch (activeTable) {
      case 'dns':
        data = [...dns];
        break;
      case 'dns2':
        data = [...dns2];
        break;
      case 'ens':
        data = [...ens];
        break;
    }
    
    return data;
  };

  // 处理排序
  const handleSort = (field: string) => {
    if (sortField === field) {
      // 如果已经按这个字段排序，则切换排序方向
      setSortDirection(sortDirection === 'asc' ? 'desc' : 'asc');
    } else {
      // 否则，设置新的排序字段并默认为降序
      setSortField(field);
      setSortDirection('desc');
    }
  };

  // 获取排序和过滤后的数据
  const getProcessedData = () => {
    let data = getCurrentData();
    
    // 搜索过滤
    if (searchTerm.trim() !== '') {
      const term = searchTerm.toLowerCase();
      data = data.filter(item => {
        return Object.values(item).some(value => 
          String(value).toLowerCase().includes(term)
        );
      });
    }
    
    // 排序
    data.sort((a, b) => {
      let aValue = a[sortField as keyof typeof a];
      let bValue = b[sortField as keyof typeof b];
      
      // 尝试转换为数字进行比较
      const aNum = Number(aValue);
      const bNum = Number(bValue);
      
      if (!isNaN(aNum) && !isNaN(bNum)) {
        return sortDirection === 'asc' ? aNum - bNum : bNum - aNum;
      }
      
      // 如果不是数字，按字符串比较
      aValue = String(aValue).toLowerCase();
      bValue = String(bValue).toLowerCase();
      
      if (sortDirection === 'asc') {
        return aValue.localeCompare(bValue);
      } else {
        return bValue.localeCompare(aValue);
      }
    });
    
    return data;
  };

  // 获取当前页的数据
  const getCurrentPageData = () => {
    const processedData = getProcessedData();
    const startIndex = (currentPage - 1) * itemsPerPage;
    return processedData.slice(startIndex, startIndex + itemsPerPage);
  };

  // 计算总页数
  const totalPages = Math.ceil(getProcessedData().length / itemsPerPage);

  // 页面改变处理
  const handlePageChange = (page: number) => {
    setCurrentPage(page);
  };

  // 每页条目数改变处理
  const handleItemsPerPageChange = (e: React.ChangeEvent<HTMLSelectElement>) => {
    setItemsPerPage(Number(e.target.value));
    setCurrentPage(1); // 重置到第一页
  };

  // 表格类型切换处理
  const handleTableTypeChange = (type: TableType) => {
    setActiveTable(type);
    setCurrentPage(1); // 重置到第一页
    setSortField('序号'); // 重置排序
    setSortDirection('desc');
  };

  // 搜索处理
  const handleSearch = (e: React.ChangeEvent<HTMLInputElement>) => {
    setSearchTerm(e.target.value);
    setCurrentPage(1); // 重置到第一页
  };

  // 获取当前表格标题
  const getCurrentHeaders = () => {
    return tableHeaders[activeTable];
  };

  // 生成分页按钮
  const renderPaginationButtons = () => {
    const buttons = [];
    
    // 添加"上一页"按钮
    buttons.push(
      <button 
        key="prev" 
        className={`pagination-btn prev ${currentPage === 1 ? 'disabled' : ''}`}
        onClick={() => currentPage > 1 && handlePageChange(currentPage - 1)}
        disabled={currentPage === 1}
      >
        上一页
      </button>
    );
    
    // 显示页码按钮
    const maxButtons = 5; // 最多显示的页码按钮数
    let startPage = Math.max(1, currentPage - Math.floor(maxButtons / 2));
    let endPage = Math.min(totalPages, startPage + maxButtons - 1);
    
    // 调整起始页，确保显示足够的按钮
    if (endPage - startPage + 1 < maxButtons && startPage > 1) {
      startPage = Math.max(1, endPage - maxButtons + 1);
    }
    
    // 添加第一页按钮
    if (startPage > 1) {
      buttons.push(
        <button 
          key="1" 
          className={`pagination-btn ${currentPage === 1 ? 'active' : ''}`}
          onClick={() => handlePageChange(1)}
        >
          1
        </button>
      );
      
      // 添加省略号
      if (startPage > 2) {
        buttons.push(
          <span key="ellipsis1" className="pagination-ellipsis">...</span>
        );
      }
    }
    
    // 添加页码按钮
    for (let i = startPage; i <= endPage; i++) {
      buttons.push(
        <button 
          key={i} 
          className={`pagination-btn ${currentPage === i ? 'active' : ''}`}
          onClick={() => handlePageChange(i)}
        >
          {i}
        </button>
      );
    }
    
    // 添加最后一页按钮
    if (endPage < totalPages) {
      // 添加省略号
      if (endPage < totalPages - 1) {
        buttons.push(
          <span key="ellipsis2" className="pagination-ellipsis">...</span>
        );
      }
      
      buttons.push(
        <button 
          key={totalPages} 
          className={`pagination-btn ${currentPage === totalPages ? 'active' : ''}`}
          onClick={() => handlePageChange(totalPages)}
        >
          {totalPages}
        </button>
      );
    }
    
    // 添加"下一页"按钮
    buttons.push(
      <button 
        key="next" 
        className={`pagination-btn next ${currentPage === totalPages ? 'disabled' : ''}`}
        onClick={() => currentPage < totalPages && handlePageChange(currentPage + 1)}
        disabled={currentPage === totalPages}
      >
        下一页
      </button>
    );
    
    return buttons;
  };

  // 渲染表格
  const renderTable = () => {
    const headers = getCurrentHeaders();
    const data = getCurrentPageData();
    
    return (
      <div className="history-table-wrapper">
        <table className="history-data-table">
          <thead>
            <tr>
              {headers.map(header => (
                <th 
                  key={header.field}
                  onClick={() => handleSort(header.field)}
                  className={sortField === header.field ? `sorted ${sortDirection}` : ''}
                >
                  {header.label}
                  {sortField === header.field && (
                    <span className="sort-indicator">
                      {sortDirection === 'asc' ? ' ▲' : ' ▼'}
                    </span>
                  )}
                </th>
              ))}
            </tr>
          </thead>
          <tbody>
            {data.length > 0 ? (
              data.map((row, index) => (
                <tr key={`${row.序号}-${index}`}>
                  {headers.map(header => {
                    let value = row[header.field as keyof typeof row];
                    
                    // 格式化上报时间
                    if (header.field === '上报时间' && value) {
                      value = formatTime(String(value), 'YYYY-MM-DD HH:mm:ss');
                    }
                    
                    const displayValue = value !== undefined ? value : '--';
                    return (
                      <td key={`${row.序号}-${header.field}`}>
                        {displayValue}
                        {header.unit && displayValue !== '--' ? header.unit : ''}
                      </td>
                    );
                  })}
                </tr>
              ))
            ) : (
              <tr>
                <td colSpan={headers.length} className="no-data">
                  暂无数据
                </td>
              </tr>
            )}
          </tbody>
        </table>
      </div>
    );
  };

  return (
    <div className="history-data-table-container">
      <div className="table-controls">
        <div className="table-tabs">
          <button 
            className={`tab-btn ${activeTable === 'dns' ? 'active' : ''}`}
            onClick={() => handleTableTypeChange('dns')}
          >
            室内环境数据
          </button>
          <button 
            className={`tab-btn ${activeTable === 'dns2' ? 'active' : ''}`}
            onClick={() => handleTableTypeChange('dns2')}
          >
            土壤成分数据
          </button>
          <button 
            className={`tab-btn ${activeTable === 'ens' ? 'active' : ''}`}
            onClick={() => handleTableTypeChange('ens')}
          >
            室外环境数据
          </button>
        </div>
        
        <div className="table-actions">
          <div className="search-box">
            <input
              type="text"
              placeholder="搜索..."
              value={searchTerm}
              onChange={handleSearch}
              className="search-input"
            />
            <span className="search-icon">🔍</span>
          </div>
          
          <div className="items-per-page">
            <label>每页显示:</label>
            <select 
              value={itemsPerPage} 
              onChange={handleItemsPerPageChange}
            >
              <option value="5">5</option>
              <option value="10">10</option>
              <option value="20">20</option>
              <option value="50">50</option>
              <option value="100">100</option>
            </select>
          </div>
        </div>
      </div>
      
      {renderTable()}
      
      <div className="table-footer">
        <div className="pagination">
          {renderPaginationButtons()}
        </div>
        
        <div className="table-info">
          显示 {getProcessedData().length} 条数据中的 
          {Math.min((currentPage - 1) * itemsPerPage + 1, getProcessedData().length)} - 
          {Math.min(currentPage * itemsPerPage, getProcessedData().length)} 条
        </div>
      </div>
    </div>
  );
};

// 使用React.memo包装组件，添加自定义比较函数，避免不必要的重新渲染
export default React.memo(HistoryDataTable, (prevProps, nextProps) => {
  // 只有当数据数量发生变化时才重新渲染
  const prevDnsLength = prevProps.dns.length;
  const nextDnsLength = nextProps.dns.length;
  
  const prevDns2Length = prevProps.dns2.length;
  const nextDns2Length = nextProps.dns2.length;
  
  const prevEnsLength = prevProps.ens.length;
  const nextEnsLength = nextProps.ens.length;
  
  // 如果数据数量没有变化，则不重新渲染
  return prevDnsLength === nextDnsLength && 
         prevDns2Length === nextDns2Length && 
         prevEnsLength === nextEnsLength;
}); 