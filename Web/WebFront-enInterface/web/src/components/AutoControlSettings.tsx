import React, { useState, useEffect } from 'react';
import { useNavigate, Link } from 'react-router-dom';

/**
 * 自动控制设置组件
 * 
 * 此组件用于配置自动控制的阈值和设备参数。
 * 当用户保存设置后，这些设置将被用于自动控制逻辑，
 * 当触发自动控制条件时，系统会通过POST请求发送控制命令到SendCmdServer.py服务。
 * 
 * 所有设备控制都严格遵循SendCmdServer.py中定义的接口，
 * 通过X-Auto-Control请求头区分自动控制和手动控制。
 */

interface AutoControlSettingsProps {
  thresholds: {
    temperature: number;
    humidity: number;
    light: number;
    soilMoisture: number;
    soilEC: number;
  };
  onSave: (newSettings: any) => void;
  currentValues: {
    temperature: number;
    humidity: number;
    light: number;
    soilMoisture: number;
    soilEC: number;
  };
  autoControlEnabled: boolean;
  onToggleAutoControl: (enabled: boolean) => void;
  isSaving: boolean;
  saveSuccess: boolean;
  saveError: string | null;
}

const AutoControlSettings: React.FC<AutoControlSettingsProps> = ({
  thresholds,
  onSave,
  currentValues,
  autoControlEnabled,
  onToggleAutoControl,
  isSaving,
  saveSuccess,
  saveError,
}) => {
  const navigate = useNavigate();
  const [settings, setSettings] = useState({ ...thresholds });
  const [fanSpeed, setFanSpeed] = useState(5);
  const [pumpSpeed, setPumpSpeed] = useState(5);
  const [ledIntensity, setLedIntensity] = useState(5);
  const [validationErrors, setValidationErrors] = useState<Record<string, string>>({});
  
  // 移除useEffect，防止settings被自动更新
  // 仅在首次渲染时初始化settings，之后只由用户操作更新

  const handleInputChange = (key: keyof typeof settings, value: number) => {
    setSettings(prev => ({
      ...prev,
      [key]: value
    }));
    
    // 清除该字段的验证错误
    if (validationErrors[key]) {
      setValidationErrors(prev => {
        const newErrors = { ...prev };
        delete newErrors[key];
        return newErrors;
      });
    }
  };

  const handleSpeedChange = (setter: React.Dispatch<React.SetStateAction<number>>, value: number) => {
    setter(value);
  };

  const validateSettings = () => {
    const errors: Record<string, string> = {};
    
    if (settings.temperature <= 0 || settings.temperature > 40) {
      errors.temperature = '温度阈值必须在1-40°C之间';
    }
    
    if (settings.humidity < 20 || settings.humidity > 90) {
      errors.humidity = '湿度阈值必须在20-90%之间';
    }
    
    if (settings.light < 0 || settings.light > 100000) {
      errors.light = '光照阈值必须在0-100000 lux之间';
    }
    
    if (settings.soilMoisture < 0 || settings.soilMoisture > 100) {
      errors.soilMoisture = '土壤湿度阈值必须在0-100%之间';
    }
    
    if (settings.soilEC < 0 || settings.soilEC > 10) {
      errors.soilEC = '土壤盐度阈值必须在0-10 ms/cm之间';
    }
    
    setValidationErrors(errors);
    return Object.keys(errors).length === 0;
  };

  const handleSave = () => {
    if (validateSettings()) {
      onSave({
        ...settings,
        fanSpeed,
        pumpSpeed,
        ledIntensity
      });
    }
  };

  const handleReset = () => {
    setSettings({ ...thresholds });
    setFanSpeed(5);
    setPumpSpeed(5);
    setLedIntensity(5);
    setValidationErrors({});
  };

  // 检查是否触发了阈值条件
  const isTemperatureTrigger = currentValues.temperature > settings.temperature;
  const isHumidityTrigger = currentValues.humidity > settings.humidity;
  const isLightTrigger = currentValues.light < settings.light;
  const isSoilMoistureTrigger = currentValues.soilMoisture < settings.soilMoisture;
  const isSoilECTrigger = currentValues.soilEC <= settings.soilEC;

  return (
    <div className="auto-control-settings-page">
      <div className="auto-control-settings-header">
        <Link to="/" className="back-button">
          <span className="back-icon">&#8592;</span> 返回主页
        </Link>
        <h1>自动控制设置</h1>
        <p className="auto-control-desc">
          配置系统的自动控制阈值和设备运行参数，当环境数据超过阈值时，系统将自动控制设备运行。
        </p>
      </div>
      
      {(saveSuccess || saveError) && (
        <div className={`settings-notification ${saveSuccess ? 'success' : 'error'}`}>
          {saveSuccess ? '设置已成功保存！' : saveError}
        </div>
      )}

      <div className="auto-control-settings-container">
        <div className="auto-control-toggle">
          <label className="toggle-switch">
            <input 
              type="checkbox" 
              checked={autoControlEnabled}
              onChange={(e) => onToggleAutoControl(e.target.checked)}
            />
            <span className="slider"></span>
          </label>
          <span>{autoControlEnabled ? '自动控制已启用' : '自动控制已禁用'}</span>
        </div>

          <div className="settings-group">
          <h3>温度控制设置</h3>
          <div className={`setting-item ${validationErrors.temperature ? 'has-error' : ''}`}>
            <label>
              温度阈值 (°C)
              <small>当温度高于阈值时，将启动风扇降温</small>
            </label>
              <input 
                type="number" 
              min="1"
              max="40"
              step="0.1"
              value={settings.temperature}
              onChange={(e) => {
                const val = e.target.value;
                handleInputChange('temperature', val === '' ? (undefined as unknown as number) : parseFloat(val));
              }}
              disabled={!autoControlEnabled}
              />
            <div className="setting-desc">设置温度超过此值时，将自动启动风扇。建议设置为20-30°C之间。</div>
            
            <div className="current-value">
              <span>当前温度: <strong>{currentValues.temperature}°C</strong></span>
              <span className={`trigger-indicator ${isTemperatureTrigger && autoControlEnabled ? 'active' : 'inactive'}`}>
                {isTemperatureTrigger && autoControlEnabled ? '已触发' : '未触发'}
              </span>
            </div>
            
            {validationErrors.temperature && (
              <div className="validation-error">{validationErrors.temperature}</div>
            )}
          </div>
          
            <div className="setting-item">
            <label>风扇转速</label>
              <input 
              type="range"
              min="1"
              max="10"
              value={fanSpeed}
              onChange={(e) => handleSpeedChange(setFanSpeed, parseInt(e.target.value))}
              disabled={!autoControlEnabled}
              />
            <div className="speed-value">{fanSpeed} / 10</div>
            <div className="setting-desc">风扇转速越高，降温效果越好，但噪音也会增大。</div>
          </div>
            </div>
        
        <div className="settings-group">
          <h3>湿度控制设置</h3>
          <div className={`setting-item ${validationErrors.humidity ? 'has-error' : ''}`}>
            <label>
              湿度阈值 (%)
              <small>当湿度高于阈值时，将启动除湿功能</small>
            </label>
              <input 
                type="number" 
              min="20"
              max="90"
              step="1"
              value={settings.humidity}
              onChange={(e) => {
                const val = e.target.value;
                handleInputChange('humidity', val === '' ? (undefined as unknown as number) : parseFloat(val));
              }}
              disabled={!autoControlEnabled}
              />
            <div className="setting-desc">设置湿度超过此值时，将自动启动除湿功能。建议设置为60-70%之间。</div>
            
            <div className="current-value">
              <span>当前湿度: <strong>{currentValues.humidity}%</strong></span>
              <span className={`trigger-indicator ${isHumidityTrigger && autoControlEnabled ? 'active' : 'inactive'}`}>
                {isHumidityTrigger && autoControlEnabled ? '已触发' : '未触发'}
              </span>
            </div>
            
            {validationErrors.humidity && (
              <div className="validation-error">{validationErrors.humidity}</div>
            )}
            </div>
          </div>

          <div className="settings-group">
          <h3>光照控制设置</h3>
          <div className={`setting-item ${validationErrors.light ? 'has-error' : ''}`}>
            <label>
              光照阈值 (lux)
              <small>当光照低于阈值时，将开启LED灯</small>
            </label>
              <input 
                type="number" 
                min="0" 
              max="100000"
              step="100"
              value={settings.light}
              onChange={(e) => {
                const val = e.target.value;
                handleInputChange('light', val === '' ? (undefined as unknown as number) : parseFloat(val));
              }}
              disabled={!autoControlEnabled}
              />
            <div className="setting-desc">设置光照低于此值时，将自动开启LED灯。建议设置为5000-10000 lux之间。</div>
            
            <div className="current-value">
              <span>当前光照: <strong>{currentValues.light} lux</strong></span>
              <span className={`trigger-indicator ${isLightTrigger && autoControlEnabled ? 'active' : 'inactive'}`}>
                {isLightTrigger && autoControlEnabled ? '已触发' : '未触发'}
              </span>
            </div>
            
            {validationErrors.light && (
              <div className="validation-error">{validationErrors.light}</div>
            )}
          </div>

            <div className="setting-item">
            <label>LED亮度</label>
              <input 
              type="range"
              min="1"
              max="10"
              value={ledIntensity}
              onChange={(e) => handleSpeedChange(setLedIntensity, parseInt(e.target.value))}
              disabled={!autoControlEnabled}
              />
            <div className="speed-value">{ledIntensity} / 10</div>
            <div className="setting-desc">LED亮度越高，光照效果越强，但会增加能耗。</div>
            </div>
            </div>
        
        <div className="settings-group">
          <h3>土壤湿度控制设置</h3>
          <div className={`setting-item ${validationErrors.soilMoisture ? 'has-error' : ''}`}>
            <label>
              土壤湿度阈值 (%)
              <small>当土壤湿度低于阈值时，将启动水泵浇水</small>
            </label>
              <input 
                type="number" 
                min="0" 
                max="100"
              step="1"
              value={settings.soilMoisture}
              onChange={(e) => {
                const val = e.target.value;
                handleInputChange('soilMoisture', val === '' ? (undefined as unknown as number) : parseFloat(val));
              }}
              disabled={!autoControlEnabled}
            />
            <div className="setting-desc">设置土壤湿度低于此值时，将自动启动水泵浇水。建议设置为30-50%之间。</div>
            
            <div className="current-value">
              <span>当前土壤湿度: <strong>{currentValues.soilMoisture}%</strong></span>
              <span className={`trigger-indicator ${isSoilMoistureTrigger && autoControlEnabled ? 'active' : 'inactive'}`}>
                {isSoilMoistureTrigger && autoControlEnabled ? '已触发' : '未触发'}
              </span>
            </div>
            
            {validationErrors.soilMoisture && (
              <div className="validation-error">{validationErrors.soilMoisture}</div>
            )}
          </div>
          
          <div className="setting-item">
            <label>水泵速度</label>
            <input
              type="range"
              min="1"
              max="10"
              value={pumpSpeed}
              onChange={(e) => handleSpeedChange(setPumpSpeed, parseInt(e.target.value))}
              disabled={!autoControlEnabled}
              />
            <div className="speed-value">{pumpSpeed} / 10</div>
            <div className="setting-desc">水泵速度越高，浇水速度越快，但可能会造成过度浇水。</div>
            </div>
          </div>

          <div className="settings-group">
          <h3>土壤盐度控制设置</h3>
          <div className={`setting-item ${validationErrors.soilEC ? 'has-error' : ''}`}>
            <label>
              土壤盐度阈值 (ms/cm)
              <small>当土壤盐度等于或低于阈值时，将关闭水泵</small>
            </label>
            <input
              type="number"
              min="0"
              max="10"
              step="0.1"
              value={settings.soilEC}
              onChange={(e) => {
                const val = e.target.value;
                handleInputChange('soilEC', val === '' ? (undefined as unknown as number) : parseFloat(val));
              }}
              disabled={!autoControlEnabled}
            />
            <div className="setting-desc">设置土壤盐度等于或低于此值时，将自动关闭水泵。建议设置为1.0-3.0 ms/cm之间。</div>
            
            <div className="current-value">
              <span>当前土壤盐度: <strong>{currentValues.soilEC} ms/cm</strong></span>
              <span className={`trigger-indicator ${isSoilECTrigger && autoControlEnabled ? 'active' : 'inactive'}`}>
                {isSoilECTrigger && autoControlEnabled ? '已触发' : '未触发'}
              </span>
            </div>
            
            {validationErrors.soilEC && (
              <div className="validation-error">{validationErrors.soilEC}</div>
            )}
          </div>
        </div>
        
        <div className="settings-actions">
          <button 
            className="reset-settings-btn" 
            onClick={handleReset}
            disabled={!autoControlEnabled}
          >
            重置设置
          </button>
          <button 
            className={`save-settings-btn ${isSaving || !autoControlEnabled ? 'disabled' : ''}`} 
            onClick={handleSave}
            disabled={isSaving || !autoControlEnabled}
          >
            {isSaving ? '保存中...' : '保存设置'}
          </button>
        </div>
        
        <div className="auto-control-info">
          <p><strong>自动控制说明：</strong></p>
          <ul>
            <li>当<strong>温度高于</strong>设定阈值时，将自动启动风扇进行降温。</li>
            <li>当<strong>湿度高于</strong>设定阈值时，将自动启动除湿功能。</li>
            <li>当<strong>光照低于</strong>设定阈值时，将自动开启LED灯补光。</li>
            <li>当<strong>土壤湿度低于</strong>设定阈值时，将自动启动水泵浇水。</li>
            <li>当<strong>土壤盐度等于或低于</strong>设定阈值时，将自动关闭水泵。</li>
          </ul>
          <div className="note">注意：自动控制功能需要确保硬件设备正常连接并可用，若设备离线或故障，自动控制可能无法正常工作。</div>
        </div>
      </div>
    </div>
  );
};

export default AutoControlSettings; 