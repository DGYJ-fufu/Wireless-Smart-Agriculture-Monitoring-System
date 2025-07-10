import React, { useEffect, useState } from 'react';

// 通知类型定义
export interface Notification {
  id: string;
  message: string;
  type: 'success' | 'error' | 'info' | 'warning';
  duration?: number;
}

interface NotificationToastProps {
  notifications: Notification[];
  removeNotification: (id: string) => void;
}

const NotificationToast: React.FC<NotificationToastProps> = ({ 
  notifications, 
  removeNotification 
}) => {
  // 计算通知在页面上的停留时间，默认3秒
  useEffect(() => {
    if (notifications.length > 0) {
      const timeouts = notifications.map(notification => {
        const duration = notification.duration || 3000; // 默认3秒
        return setTimeout(() => {
          removeNotification(notification.id);
        }, duration);
      });

      return () => {
        timeouts.forEach(timeout => clearTimeout(timeout));
      };
    }
  }, [notifications, removeNotification]);

  // 根据通知类型返回对应的图标
  const getIcon = (type: string) => {
    switch (type) {
      case 'success':
        return '✅';
      case 'error':
        return '❌';
      case 'warning':
        return '⚠️';
      case 'info':
      default:
        return 'ℹ️';
    }
  };

  return (
    <div className="notification-container">
      {notifications.map(notification => (
        <div 
          key={notification.id} 
          className={`notification-toast ${notification.type}`}
        >
          <div className="notification-icon">
            {getIcon(notification.type)}
          </div>
          <div className="notification-content">
            {notification.message}
          </div>
          <button 
            className="notification-close"
            onClick={() => removeNotification(notification.id)}
          >
            &times;
          </button>
        </div>
      ))}
    </div>
  );
};

export default NotificationToast; 