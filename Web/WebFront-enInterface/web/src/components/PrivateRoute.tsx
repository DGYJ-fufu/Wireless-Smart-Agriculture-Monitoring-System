import React from 'react';
import { Navigate } from 'react-router-dom';

interface PrivateRouteProps {
  children: React.ReactNode;
}

/**
 * 路由保护组件
 *
 * 如果本地不存在 authToken，自动重定向到 /login。
 */
const PrivateRoute: React.FC<PrivateRouteProps> = ({ children }) => {
  const isAuthenticated = !!localStorage.getItem('authToken');
  return isAuthenticated ? children : <Navigate to="/login" replace />;
};

export default PrivateRoute; 