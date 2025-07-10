/**
 * 时间格式化工具
 * 支持多种时间格式的标准化显示
 */

/**
 * 将各种格式的时间字符串转换为标准格式
 * 支持的格式包括：
 * - ISO格式：2025-07-08T07:12:51Z
 * - 紧凑格式：20250708T071251Z
 * - 标准日期时间：2025-07-08 07:12:51
 * - 纯数字格式：20250708071251
 * 
 * @param timeStr 时间字符串
 * @param format 输出格式，默认为 'YYYY-MM-DD HH:mm:ss'
 * @returns 格式化后的时间字符串，如果解析失败则返回原字符串
 */
export const formatTime = (timeStr: string, format: string = 'YYYY-MM-DD HH:mm:ss'): string => {
  if (!timeStr) return '';
  
  try {
    let date: Date | null = null;
    
    // 特殊处理 20250708T071251Z 格式
    if (/^\d{8}T\d{6}Z$/.test(timeStr)) {
      const year = parseInt(timeStr.substring(0, 4));
      const month = parseInt(timeStr.substring(4, 6)) - 1; // 月份从0开始
      const day = parseInt(timeStr.substring(6, 8));
      const hour = parseInt(timeStr.substring(9, 11));
      const minute = parseInt(timeStr.substring(11, 13));
      const second = parseInt(timeStr.substring(13, 15));
      date = new Date(Date.UTC(year, month, day, hour, minute, second));
    }
    // 尝试解析ISO格式：2025-07-08T07:12:51Z
    else if (timeStr.includes('T') && (timeStr.includes('Z') || timeStr.includes('+'))) {
      date = new Date(timeStr);
    } 
    // 尝试解析标准日期时间：2025-07-08 07:12:51
    else if (timeStr.includes(' ') && timeStr.includes('-') && timeStr.includes(':')) {
      date = new Date(timeStr.replace(' ', 'T') + 'Z');
    }
    // 尝试解析纯数字格式：20250708071251
    else if (/^\d{14}$/.test(timeStr)) {
      const year = parseInt(timeStr.substring(0, 4));
      const month = parseInt(timeStr.substring(4, 6)) - 1; // 月份从0开始
      const day = parseInt(timeStr.substring(6, 8));
      const hour = parseInt(timeStr.substring(8, 10));
      const minute = parseInt(timeStr.substring(10, 12));
      const second = parseInt(timeStr.substring(12, 14));
      date = new Date(year, month, day, hour, minute, second);
    }
    // 尝试其他可能的格式
    else {
      date = new Date(timeStr);
    }
    
    // 检查日期是否有效
    if (isNaN(date.getTime())) {
      console.warn(`无法解析时间格式: ${timeStr}`);
      return timeStr;
    }
    
    // 根据指定格式返回时间字符串
    return formatDateToString(date, format);
  } catch (err) {
    console.error(`时间格式化错误: ${err}`, timeStr);
    return timeStr;
  }
};

/**
 * 将Date对象按照指定格式转换为字符串
 * 
 * @param date Date对象
 * @param format 格式字符串
 * @returns 格式化后的时间字符串
 */
const formatDateToString = (date: Date, format: string): string => {
  const year = date.getFullYear();
  const month = date.getMonth() + 1; // 月份从0开始
  const day = date.getDate();
  const hours = date.getHours();
  const minutes = date.getMinutes();
  const seconds = date.getSeconds();
  
  const pad = (num: number): string => num.toString().padStart(2, '0');
  
  return format
    .replace('YYYY', year.toString())
    .replace('MM', pad(month))
    .replace('DD', pad(day))
    .replace('HH', pad(hours))
    .replace('mm', pad(minutes))
    .replace('ss', pad(seconds));
};

/**
 * 获取相对时间描述
 * 例如：刚刚、5分钟前、1小时前、昨天 等
 * 
 * @param timeStr 时间字符串
 * @returns 相对时间描述
 */
export const getRelativeTime = (timeStr: string): string => {
  if (!timeStr) return '';
  
  try {
    const date = new Date(formatTime(timeStr, 'YYYY-MM-DD HH:mm:ss'));
    const now = new Date();
    const diffMs = now.getTime() - date.getTime();
    const diffSec = Math.floor(diffMs / 1000);
    
    if (diffSec < 60) return '刚刚';
    if (diffSec < 3600) return `${Math.floor(diffSec / 60)}分钟前`;
    if (diffSec < 86400) return `${Math.floor(diffSec / 3600)}小时前`;
    if (diffSec < 172800) return '昨天';
    if (diffSec < 2592000) return `${Math.floor(diffSec / 86400)}天前`;
    
    // 超过30天，返回标准日期
    return formatTime(timeStr, 'YYYY-MM-DD');
  } catch (err) {
    console.error(`相对时间计算错误: ${err}`, timeStr);
    return timeStr;
  }
}; 