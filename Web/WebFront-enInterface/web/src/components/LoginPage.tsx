import React, { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import config from '../../config/config';

const { API_URL } = config;

const LoginPage: React.FC = () => {
  const navigate = useNavigate();
  // 如果已经登录则直接跳转到首页
  useEffect(() => {
    if (localStorage.getItem('authToken')) {
      navigate('/');
    }
  }, [navigate]);
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const [error, setError] = useState('');
  const [loading, setLoading] = useState(false);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError('');
    setLoading(true);
    try {
      const response = await fetch(`${API_URL}login`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify({ username, password })
      });

      const data = await response.json();
      if (response.ok && data.status === 'success') {
        localStorage.setItem('authToken', data.token || 'simple-auth-token');
        navigate('/');
      } else {
        setError(data.message || '登录失败');
      }
    } catch (err) {
      console.error('登录请求错误:', err);
      setError('无法连接服务器');
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="login-page">
      <div className="login-container">
        <h2 className="login-title">智慧农业监控平台登录</h2>
        <form className="login-form" onSubmit={handleSubmit}>
          <div className="form-group">
            <label htmlFor="username">用户名</label>
            <input
              id="username"
              type="text"
              value={username}
              onChange={(e) => setUsername(e.target.value)}
              placeholder="请输入用户名"
              required
            />
          </div>
          <div className="form-group">
            <label htmlFor="password">密码</label>
            <input
              id="password"
              type="password"
              value={password}
              onChange={(e) => setPassword(e.target.value)}
              placeholder="请输入密码"
              required
            />
          </div>
          {error && <p className="error-message">{error}</p>}
          <button type="submit" disabled={loading}>
            {loading ? '登录中...' : '登录'}
          </button>
        </form>
      </div>
    </div>
  );
};

export default LoginPage; 