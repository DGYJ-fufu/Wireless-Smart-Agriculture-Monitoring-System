import React, { useState, useEffect, useMemo } from 'react';
import { Link } from 'react-router-dom';

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

// 组件属性接口
interface SystemLogProps {
  logs: LogEntry[];
  clearLogs?: () => void;
}

const SystemLog: React.FC<SystemLogProps> = ({ logs, clearLogs }) => {
  const [filter, setFilter] = useState<string>('all');
  const [deviceFilter, setDeviceFilter] = useState<string>('all');
  const [actionFilter, setActionFilter] = useState<string>('all');
  const [searchTerm, setSearchTerm] = useState<string>('');
  const [currentPage, setCurrentPage] = useState<number>(1);
  const [timeRange, setTimeRange] = useState<string>('all');
  const [showDetails, setShowDetails] = useState<number | null>(null);
  const [sortDirection, setSortDirection] = useState<'asc' | 'desc'>('desc');
  const logsPerPage = 20;

  // 获取所有设备类型
  const deviceTypes = useMemo(() => {
    const devices = new Set<string>();
    logs.forEach(log => {
      if (log.device) {
        devices.add(log.device);
      }
    });
    return Array.from(devices).sort();
  }, [logs]);

  // 获取所有动作类型
  const actionTypes = useMemo(() => {
    const actions = new Set<string>();
    logs.forEach(log => {
      if (log.action) {
        // 提取主要动作类别（例如从"自动控制-开启风扇"提取"自动控制"）
        const mainAction = log.action.split('-')[0];
        actions.add(mainAction);
      }
    });
    return Array.from(actions).sort();
  }, [logs]);

  // 获取时间范围
  const getTimeRangeFilter = (log: LogEntry) => {
    if (timeRange === 'all') return true;
    
    const logDate = new Date(log.timestamp);
    const now = new Date();
    
    switch (timeRange) {
      case 'today':
        const today = new Date(now.getFullYear(), now.getMonth(), now.getDate());
        return logDate >= today;
      case 'last24h':
        const last24h = new Date(now.getTime() - 24 * 60 * 60 * 1000);
        return logDate >= last24h;
      case 'thisWeek':
        const thisWeek = new Date(now.getTime() - 7 * 24 * 60 * 60 * 1000);
        return logDate >= thisWeek;
      default:
        return true;
    }
  };

  // 筛选和搜索日志
  const filteredLogs = useMemo(() => {
    return logs
      .filter(log => {
        // 类型筛选
        if (filter !== 'all' && log.type !== filter) return false;
        
        // 设备筛选
        if (deviceFilter !== 'all' && log.device !== deviceFilter) return false;
        
        // 动作筛选
        if (actionFilter !== 'all') {
          // 检查动作是否以选定的前缀开始
          if (!log.action.startsWith(actionFilter)) return false;
        }
        
        // 时间范围筛选
        if (!getTimeRangeFilter(log)) return false;
        
        // 搜索筛选
        if (searchTerm) {
          const searchLower = searchTerm.toLowerCase();
          return (
            log.action.toLowerCase().includes(searchLower) || 
            (log.details?.toLowerCase().includes(searchLower) || false) || 
            (log.device?.toLowerCase().includes(searchLower) || false) ||
            (log.status?.toLowerCase().includes(searchLower) || false)
          );
        }
        
        return true;
      })
      .sort((a, b) => {
        const dateA = new Date(a.timestamp).getTime();
        const dateB = new Date(b.timestamp).getTime();
        return sortDirection === 'desc' ? dateB - dateA : dateA - dateB;
      });
  }, [logs, filter, deviceFilter, actionFilter, timeRange, searchTerm, sortDirection]);

  // 计算当前页的日志
  const currentLogs = useMemo(() => {
    const indexOfLastLog = currentPage * logsPerPage;
    const indexOfFirstLog = indexOfLastLog - logsPerPage;
    return filteredLogs.slice(indexOfFirstLog, indexOfLastLog);
  }, [filteredLogs, currentPage]);
  
  // 计算总页数
  const totalPages = Math.ceil(filteredLogs.length / logsPerPage);
  
  // 生成页码数组
  const pageNumbers = useMemo(() => {
    const pages = [];
    for (let i = 1; i <= totalPages; i++) {
      pages.push(i);
    }
    return pages;
  }, [totalPages]);
  
  // 页面变更时重置到顶部
  useEffect(() => {
    window.scrollTo(0, 0);
  }, [currentPage]);

  // 当筛选条件改变时，重置页码
  useEffect(() => {
    setCurrentPage(1);
  }, [filter, deviceFilter, actionFilter, timeRange, searchTerm]);
  
  // 获取日志类型对应的图标和颜色
  const getLogTypeStyles = (type: string) => {
    switch(type) {
      case 'info':
        return { icon: 'ℹ️', className: 'info-log' };
      case 'warning':
        return { icon: '⚠️', className: 'warning-log' };
      case 'error':
        return { icon: '❌', className: 'error-log' };
      case 'success':
        return { icon: '✅', className: 'success-log' };
      default:
        return { icon: 'ℹ️', className: 'info-log' };
    }
  };

  // 格式化日志时间戳
  const formatTimestamp = (timestamp: string) => {
    try {
      const date = new Date(timestamp);
      return `${date.getFullYear()}-${(date.getMonth() + 1).toString().padStart(2, '0')}-${date.getDate().toString().padStart(2, '0')} ${date.getHours().toString().padStart(2, '0')}:${date.getMinutes().toString().padStart(2, '0')}:${date.getSeconds().toString().padStart(2, '0')}`;
    } catch (e) {
      return timestamp;
    }
  };
  
  // 导出日志为CSV
  const exportLogsToCSV = () => {
    const headers = ['时间', '类型', '操作', '设备', '状态', '详情'];
    const csvRows = [headers];
    
    filteredLogs.forEach(log => {
      const row = [
        formatTimestamp(log.timestamp),
        log.type,
        log.action,
        log.device || '',
        log.status || '',
        log.details ? `"${log.details.replace(/"/g, '""')}"` : ''
      ];
      csvRows.push(row);
    });
    
    const csvContent = csvRows.map(row => row.join(',')).join('\n');
    const blob = new Blob([csvContent], { type: 'text/csv;charset=utf-8;' });
    const url = URL.createObjectURL(blob);
    
    const link = document.createElement('a');
    const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
    link.setAttribute('href', url);
    link.setAttribute('download', `system-logs-${timestamp}.csv`);
    link.style.display = 'none';
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
  };

  return (
    <div className="system-log-page">
      <Link to="/" className="back-button">
        <span className="back-icon">&#8592;</span> 返回主页
      </Link>

      <div className="system-log-header">
        <h1>系统运行日志</h1>
        <p className="system-log-desc">查看系统运行以来的所有操作记录和状态变更</p>
      </div>
      
      <div className="system-log-controls">
        <div className="filter-controls">
          <div className="filter-row">
            <div className="filter-group">
              <label>日志类型:</label>
              <select value={filter} onChange={(e) => setFilter(e.target.value)}>
                <option value="all">全部类型</option>
                <option value="info">信息</option>
                <option value="success">成功</option>
                <option value="warning">警告</option>
                <option value="error">错误</option>
              </select>
            </div>
            
            <div className="filter-group">
              <label>设备筛选:</label>
              <select value={deviceFilter} onChange={(e) => setDeviceFilter(e.target.value)}>
                <option value="all">全部设备</option>
                {deviceTypes.map(device => (
                  <option key={device} value={device}>{device}</option>
                ))}
              </select>
            </div>
            
            <div className="filter-group">
              <label>动作类型:</label>
              <select value={actionFilter} onChange={(e) => setActionFilter(e.target.value)}>
                <option value="all">全部动作</option>
                {actionTypes.map(action => (
                  <option key={action} value={action}>{action}</option>
                ))}
              </select>
            </div>
            
            <div className="filter-group">
              <label>时间范围:</label>
              <select value={timeRange} onChange={(e) => setTimeRange(e.target.value)}>
                <option value="all">全部时间</option>
                <option value="today">今天</option>
                <option value="last24h">最近24小时</option>
                <option value="thisWeek">本周</option>
              </select>
            </div>
          </div>
          
          <div className="filter-row">
            <div className="search-group">
              <input 
                type="text" 
                placeholder="搜索日志内容..." 
                value={searchTerm}
                onChange={(e) => setSearchTerm(e.target.value)}
              />
              <button onClick={() => setSearchTerm('')} className="clear-search">清除</button>
            </div>
            
            <div className="sort-group">
              <label>排序:</label>
              <button 
                onClick={() => setSortDirection(dir => dir === 'desc' ? 'asc' : 'desc')}
                className="sort-btn"
              >
                {sortDirection === 'desc' ? '最新在前' : '最早在前'} {sortDirection === 'desc' ? '▼' : '▲'}
              </button>
            </div>
            
            <button className="export-logs-btn" onClick={exportLogsToCSV}>
              导出日志
            </button>
            
            {clearLogs && (
              <button className="clear-logs-btn" onClick={clearLogs}>
                清除所有日志
              </button>
            )}
          </div>
        </div>
        
        <div className="logs-info">
          <span>共 {logs.length} 条日志，当前筛选 {filteredLogs.length} 条</span>
        </div>
      </div>

      <div className="system-log-table-container">
        {currentLogs.length > 0 ? (
          <table className="system-log-table">
            <thead>
              <tr>
                <th>时间</th>
                <th>类型</th>
                <th>操作</th>
                <th>设备</th>
                <th>状态</th>
                <th>详情</th>
              </tr>
            </thead>
            <tbody>
              {currentLogs.map((log) => {
                const { icon, className } = getLogTypeStyles(log.type);
                const isAutoControl = log.action.startsWith('自动控制');
                const rowClassName = `${className} ${isAutoControl ? 'auto-control-log' : ''}`;
                
                return (
                  <React.Fragment key={log.id}>
                    <tr className={rowClassName} onClick={() => setShowDetails(showDetails === log.id ? null : log.id)}>
                      <td className="log-timestamp">{formatTimestamp(log.timestamp)}</td>
                      <td className="log-type">
                        <span className="log-icon">{icon}</span>
                        <span className="log-type-label">{log.type}</span>
                      </td>
                      <td className="log-action">
                        {isAutoControl && <span className="auto-control-badge">自动</span>}
                        {log.action}
                      </td>
                      <td className="log-device">{log.device || '-'}</td>
                      <td className="log-status">{log.status || '-'}</td>
                      <td className="log-details-cell">
                        <div className="log-details-preview">{log.details || '-'}</div>
                        {log.details && log.details.length > 50 && (
                          <button className="details-expand-btn">
                            {showDetails === log.id ? '收起' : '展开'}
                          </button>
                        )}
                      </td>
                    </tr>
                    {showDetails === log.id && log.details && (
                      <tr className="details-row">
                        <td colSpan={6}>
                          <div className="full-details">
                            <h4>详细信息：</h4>
                            <p>{log.details}</p>
                            {log.action.includes('响应') && (
                              <div className="response-data">
                                <pre>{log.details}</pre>
                              </div>
                            )}
                          </div>
                        </td>
                      </tr>
                    )}
                  </React.Fragment>
                );
              })}
            </tbody>
          </table>
        ) : (
          <div className="empty-logs">
            <p>没有找到匹配的日志记录</p>
            {searchTerm || filter !== 'all' || deviceFilter !== 'all' || actionFilter !== 'all' || timeRange !== 'all' ? (
              <button onClick={() => {
                setSearchTerm('');
                setFilter('all');
                setDeviceFilter('all');
                setActionFilter('all');
                setTimeRange('all');
              }} className="clear-filters-btn">
                清除所有筛选条件
              </button>
            ) : null}
          </div>
        )}
      </div>
      
      {totalPages > 1 && (
        <div className="pagination">
          <button 
            onClick={() => setCurrentPage(prev => Math.max(prev - 1, 1))}
            disabled={currentPage === 1}
            className="page-btn prev-btn"
          >
            上一页
          </button>
          
          <div className="page-numbers">
            {pageNumbers.length <= 7 ? (
              pageNumbers.map(number => (
                <button 
                  key={number}
                  onClick={() => setCurrentPage(number)}
                  className={currentPage === number ? 'page-btn active' : 'page-btn'}
                >
                  {number}
                </button>
              ))
            ) : (
              <>
                {currentPage > 2 && (
                  <button onClick={() => setCurrentPage(1)} className="page-btn">
                    1
                  </button>
                )}
                
                {currentPage > 3 && (
                  <span className="page-ellipsis">...</span>
                )}
                
                {pageNumbers
                  .filter(num => num >= currentPage - 1 && num <= currentPage + 1)
                  .map(number => (
                    <button 
                      key={number}
                      onClick={() => setCurrentPage(number)}
                      className={currentPage === number ? 'page-btn active' : 'page-btn'}
                    >
                      {number}
                    </button>
                  ))
                }
                
                {currentPage < totalPages - 2 && (
                  <span className="page-ellipsis">...</span>
                )}
                
                {currentPage < totalPages - 1 && (
                  <button 
                    onClick={() => setCurrentPage(totalPages)} 
                    className="page-btn"
                  >
                    {totalPages}
                  </button>
                )}
              </>
            )}
          </div>
          
          <button 
            onClick={() => setCurrentPage(prev => Math.min(prev + 1, totalPages))}
            disabled={currentPage === totalPages}
            className="page-btn next-btn"
          >
            下一页
          </button>
        </div>
      )}
    </div>
  );
};

export default SystemLog; 