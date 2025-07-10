import React, { useState } from 'react';
import { Link } from 'react-router-dom';
import { formatTime } from '../utils/timeFormatter';

// 数据阈值设置接口
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

interface ThresholdSettingsProps {
  dataThresholds: DataThresholdSettings;
  setDataThresholds: React.Dispatch<React.SetStateAction<DataThresholdSettings>>;
  dns: DNS[];
  dns2: DNS_2[];
}

const ThresholdSettings: React.FC<ThresholdSettingsProps> = ({ 
  dataThresholds, 
  setDataThresholds,
  dns,
  dns2
}) => {
  // 当前编辑的阈值类别
  const [currentCategory, setCurrentCategory] = useState<string>("环境数据");

  return (
    <div className="threshold-settings-page">
      <Link to="/" className="back-button">
        <span className="back-icon">&#8592;</span> 返回主页
      </Link>

      <div className="threshold-settings-header">
        <h1>数据阈值设置</h1>
        <p className="threshold-desc">设置各项数据的最佳状态和最差状态，用于进度条显示</p>
      </div>

      <div className="threshold-settings-container">
        <div className="category-tabs">
          <button 
            className={`category-tab ${currentCategory === "环境数据" ? "active" : ""}`}
            onClick={() => setCurrentCategory("环境数据")}
          >
            环境数据
          </button>
          <button 
            className={`category-tab ${currentCategory === "土壤数据" ? "active" : ""}`}
            onClick={() => setCurrentCategory("土壤数据")}
          >
            土壤数据
          </button>
        </div>
        
        {currentCategory === "环境数据" && (
          <div className="threshold-settings-group">
            <div className="threshold-setting-item">
              <div className="threshold-setting-header">
                <span className="threshold-label">室内温度 (°C)</span>
                <div className="threshold-preview">
                  <span className="worst-value">{dataThresholds.室内温度.worst}</span>
                  <div className="preview-bar">
                    <div className="preview-indicator" style={{
                      left: `${((parseFloat(dns[0]?.室内温度 || "0") - dataThresholds.室内温度.worst) / 
                      (dataThresholds.室内温度.best - dataThresholds.室内温度.worst)) * 100}%`
                    }}></div>
                  </div>
                  <span className="best-value">{dataThresholds.室内温度.best}</span>
                </div>
              </div>
              <div className="threshold-inputs">
                <div className="input-group">
                  <label>最佳值:</label>
                  <input 
                    type="number"
                    value={dataThresholds.室内温度.best}
                    onChange={(e) => setDataThresholds({
                      ...dataThresholds,
                      室内温度: {
                        ...dataThresholds.室内温度,
                        best: parseFloat(e.target.value)
                      }
                    })}
                  />
                </div>
                <div className="input-group">
                  <label>最差值:</label>
                  <input 
                    type="number"
                    value={dataThresholds.室内温度.worst}
                    onChange={(e) => setDataThresholds({
                      ...dataThresholds,
                      室内温度: {
                        ...dataThresholds.室内温度,
                        worst: parseFloat(e.target.value)
                      }
                    })}
                  />
                </div>
              </div>
            </div>

            <div className="threshold-setting-item">
              <div className="threshold-setting-header">
                <span className="threshold-label">室内湿度 (%)</span>
                <div className="threshold-preview">
                  <span className="worst-value">{dataThresholds.室内湿度.worst}</span>
                  <div className="preview-bar">
                    <div className="preview-indicator" style={{
                      left: `${((parseFloat(dns[0]?.室内湿度 || "0") - dataThresholds.室内湿度.worst) / 
                      (dataThresholds.室内湿度.best - dataThresholds.室内湿度.worst)) * 100}%`
                    }}></div>
                  </div>
                  <span className="best-value">{dataThresholds.室内湿度.best}</span>
                </div>
              </div>
              <div className="threshold-inputs">
                <div className="input-group">
                  <label>最佳值:</label>
                  <input 
                    type="number"
                    value={dataThresholds.室内湿度.best}
                    onChange={(e) => setDataThresholds({
                      ...dataThresholds,
                      室内湿度: {
                        ...dataThresholds.室内湿度,
                        best: parseFloat(e.target.value)
                      }
                    })}
                  />
                </div>
                <div className="input-group">
                  <label>最差值:</label>
                  <input 
                    type="number"
                    value={dataThresholds.室内湿度.worst}
                    onChange={(e) => setDataThresholds({
                      ...dataThresholds,
                      室内湿度: {
                        ...dataThresholds.室内湿度,
                        worst: parseFloat(e.target.value)
                      }
                    })}
                  />
                </div>
              </div>
            </div>

            <div className="threshold-setting-item">
              <div className="threshold-setting-header">
                <span className="threshold-label">光照强度 (lx)</span>
                <div className="threshold-preview">
                  <span className="worst-value">{dataThresholds.光照强度.worst}</span>
                  <div className="preview-bar">
                    <div className="preview-indicator" style={{
                      left: `${((parseFloat(dns[0]?.光照强度 || "0") - dataThresholds.光照强度.worst) / 
                      (dataThresholds.光照强度.best - dataThresholds.光照强度.worst)) * 100}%`
                    }}></div>
                  </div>
                  <span className="best-value">{dataThresholds.光照强度.best}</span>
                </div>
              </div>
              <div className="threshold-inputs">
                <div className="input-group">
                  <label>最佳值:</label>
                  <input 
                    type="number"
                    value={dataThresholds.光照强度.best}
                    onChange={(e) => setDataThresholds({
                      ...dataThresholds,
                      光照强度: {
                        ...dataThresholds.光照强度,
                        best: parseFloat(e.target.value)
                      }
                    })}
                  />
                </div>
                <div className="input-group">
                  <label>最差值:</label>
                  <input 
                    type="number"
                    value={dataThresholds.光照强度.worst}
                    onChange={(e) => setDataThresholds({
                      ...dataThresholds,
                      光照强度: {
                        ...dataThresholds.光照强度,
                        worst: parseFloat(e.target.value)
                      }
                    })}
                  />
                </div>
              </div>
            </div>

            <div className="threshold-setting-item">
              <div className="threshold-setting-header">
                <span className="threshold-label">挥发性有机化合物浓度 (ppm)</span>
                <div className="threshold-preview">
                  <span className="worst-value">{dataThresholds.挥发性有机化合物浓度.worst}</span>
                  <div className="preview-bar">
                    <div className="preview-indicator" style={{
                      left: `${((parseFloat(dns[0]?.挥发性有机化合物浓度 || "0") - dataThresholds.挥发性有机化合物浓度.worst) / 
                      (dataThresholds.挥发性有机化合物浓度.best - dataThresholds.挥发性有机化合物浓度.worst)) * 100}%`
                    }}></div>
                  </div>
                  <span className="best-value">{dataThresholds.挥发性有机化合物浓度.best}</span>
                </div>
              </div>
              <div className="threshold-inputs">
                <div className="input-group">
                  <label>最佳值:</label>
                  <input 
                    type="number"
                    value={dataThresholds.挥发性有机化合物浓度.best}
                    onChange={(e) => setDataThresholds({
                      ...dataThresholds,
                      挥发性有机化合物浓度: {
                        ...dataThresholds.挥发性有机化合物浓度,
                        best: parseFloat(e.target.value)
                      }
                    })}
                  />
                </div>
                <div className="input-group">
                  <label>最差值:</label>
                  <input 
                    type="number"
                    value={dataThresholds.挥发性有机化合物浓度.worst}
                    onChange={(e) => setDataThresholds({
                      ...dataThresholds,
                      挥发性有机化合物浓度: {
                        ...dataThresholds.挥发性有机化合物浓度,
                        worst: parseFloat(e.target.value)
                      }
                    })}
                  />
                </div>
              </div>
            </div>

            <div className="threshold-setting-item">
              <div className="threshold-setting-header">
                <span className="threshold-label">二氧化碳浓度 (ppm)</span>
                <div className="threshold-preview">
                  <span className="worst-value">{dataThresholds.二氧化碳浓度.worst}</span>
                  <div className="preview-bar">
                    <div className="preview-indicator" style={{
                      left: `${((parseFloat(dns[0]?.二氧化碳浓度 || "0") - dataThresholds.二氧化碳浓度.worst) / 
                      (dataThresholds.二氧化碳浓度.best - dataThresholds.二氧化碳浓度.worst)) * 100}%`
                    }}></div>
                  </div>
                  <span className="best-value">{dataThresholds.二氧化碳浓度.best}</span>
                </div>
              </div>
              <div className="threshold-inputs">
                <div className="input-group">
                  <label>最佳值:</label>
                  <input 
                    type="number"
                    value={dataThresholds.二氧化碳浓度.best}
                    onChange={(e) => setDataThresholds({
                      ...dataThresholds,
                      二氧化碳浓度: {
                        ...dataThresholds.二氧化碳浓度,
                        best: parseFloat(e.target.value)
                      }
                    })}
                  />
                </div>
                <div className="input-group">
                  <label>最差值:</label>
                  <input 
                    type="number"
                    value={dataThresholds.二氧化碳浓度.worst}
                    onChange={(e) => setDataThresholds({
                      ...dataThresholds,
                      二氧化碳浓度: {
                        ...dataThresholds.二氧化碳浓度,
                        worst: parseFloat(e.target.value)
                      }
                    })}
                  />
                </div>
              </div>
            </div>
          </div>
        )}
        
        {currentCategory === "土壤数据" && (
          <div className="threshold-settings-group">
            <div className="threshold-setting-item">
              <div className="threshold-setting-header">
                <span className="threshold-label">土壤湿度 (%)</span>
                <div className="threshold-preview">
                  <span className="worst-value">{dataThresholds.土壤湿度.worst}</span>
                  <div className="preview-bar">
                    <div className="preview-indicator" style={{
                      left: `${((parseFloat(dns[0]?.土壤湿度 || "0") - dataThresholds.土壤湿度.worst) / 
                      (dataThresholds.土壤湿度.best - dataThresholds.土壤湿度.worst)) * 100}%`
                    }}></div>
                  </div>
                  <span className="best-value">{dataThresholds.土壤湿度.best}</span>
                </div>
              </div>
              <div className="threshold-inputs">
                <div className="input-group">
                  <label>最佳值:</label>
                  <input 
                    type="number"
                    value={dataThresholds.土壤湿度.best}
                    onChange={(e) => setDataThresholds({
                      ...dataThresholds,
                      土壤湿度: {
                        ...dataThresholds.土壤湿度,
                        best: parseFloat(e.target.value)
                      }
                    })}
                  />
                </div>
                <div className="input-group">
                  <label>最差值:</label>
                  <input 
                    type="number"
                    value={dataThresholds.土壤湿度.worst}
                    onChange={(e) => setDataThresholds({
                      ...dataThresholds,
                      土壤湿度: {
                        ...dataThresholds.土壤湿度,
                        worst: parseFloat(e.target.value)
                      }
                    })}
                  />
                </div>
              </div>
            </div>

            <div className="threshold-setting-item">
              <div className="threshold-setting-header">
                <span className="threshold-label">土壤温度 (°C)</span>
                <div className="threshold-preview">
                  <span className="worst-value">{dataThresholds.土壤温度.worst}</span>
                  <div className="preview-bar">
                    <div className="preview-indicator" style={{
                      left: `${((parseFloat(dns[0]?.土壤温度 || "0") - dataThresholds.土壤温度.worst) / 
                      (dataThresholds.土壤温度.best - dataThresholds.土壤温度.worst)) * 100}%`
                    }}></div>
                  </div>
                  <span className="best-value">{dataThresholds.土壤温度.best}</span>
                </div>
              </div>
              <div className="threshold-inputs">
                <div className="input-group">
                  <label>最佳值:</label>
                  <input 
                    type="number"
                    value={dataThresholds.土壤温度.best}
                    onChange={(e) => setDataThresholds({
                      ...dataThresholds,
                      土壤温度: {
                        ...dataThresholds.土壤温度,
                        best: parseFloat(e.target.value)
                      }
                    })}
                  />
                </div>
                <div className="input-group">
                  <label>最差值:</label>
                  <input 
                    type="number"
                    value={dataThresholds.土壤温度.worst}
                    onChange={(e) => setDataThresholds({
                      ...dataThresholds,
                      土壤温度: {
                        ...dataThresholds.土壤温度,
                        worst: parseFloat(e.target.value)
                      }
                    })}
                  />
                </div>
              </div>
            </div>

            <div className="threshold-setting-item">
              <div className="threshold-setting-header">
                <span className="threshold-label">土壤盐度 (g/kg)</span>
                <div className="threshold-preview">
                  <span className="worst-value">{dataThresholds.土壤盐度.worst}</span>
                  <div className="preview-bar">
                    <div className="preview-indicator" style={{
                      left: `${((parseFloat(dns2[0]?.土壤盐度 || "0") - dataThresholds.土壤盐度.worst) / 
                      (dataThresholds.土壤盐度.best - dataThresholds.土壤盐度.worst)) * 100}%`
                    }}></div>
                  </div>
                  <span className="best-value">{dataThresholds.土壤盐度.best}</span>
                </div>
              </div>
              <div className="threshold-inputs">
                <div className="input-group">
                  <label>最佳值:</label>
                  <input 
                    type="number"
                    value={dataThresholds.土壤盐度.best}
                    onChange={(e) => setDataThresholds({
                      ...dataThresholds,
                      土壤盐度: {
                        ...dataThresholds.土壤盐度,
                        best: parseFloat(e.target.value)
                      }
                    })}
                  />
                </div>
                <div className="input-group">
                  <label>最差值:</label>
                  <input 
                    type="number"
                    value={dataThresholds.土壤盐度.worst}
                    onChange={(e) => setDataThresholds({
                      ...dataThresholds,
                      土壤盐度: {
                        ...dataThresholds.土壤盐度,
                        worst: parseFloat(e.target.value)
                      }
                    })}
                  />
                </div>
              </div>
            </div>

            <div className="threshold-setting-item">
              <div className="threshold-setting-header">
                <span className="threshold-label">土壤导电率 (mS/cm)</span>
                <div className="threshold-preview">
                  <span className="worst-value">{dataThresholds.土壤导电率.worst}</span>
                  <div className="preview-bar">
                    <div className="preview-indicator" style={{
                      left: `${((parseFloat(dns2[0]?.土壤导电率 || "0") - dataThresholds.土壤导电率.worst) / 
                      (dataThresholds.土壤导电率.best - dataThresholds.土壤导电率.worst)) * 100}%`
                    }}></div>
                  </div>
                  <span className="best-value">{dataThresholds.土壤导电率.best}</span>
                </div>
              </div>
              <div className="threshold-inputs">
                <div className="input-group">
                  <label>最佳值:</label>
                  <input 
                    type="number"
                    value={dataThresholds.土壤导电率.best}
                    onChange={(e) => setDataThresholds({
                      ...dataThresholds,
                      土壤导电率: {
                        ...dataThresholds.土壤导电率,
                        best: parseFloat(e.target.value)
                      }
                    })}
                  />
                </div>
                <div className="input-group">
                  <label>最差值:</label>
                  <input 
                    type="number"
                    value={dataThresholds.土壤导电率.worst}
                    onChange={(e) => setDataThresholds({
                      ...dataThresholds,
                      土壤导电率: {
                        ...dataThresholds.土壤导电率,
                        worst: parseFloat(e.target.value)
                      }
                    })}
                  />
                </div>
              </div>
            </div>

            <div className="threshold-setting-item">
              <div className="threshold-setting-header">
                <span className="threshold-label">土壤PH值</span>
                <div className="threshold-preview">
                  <span className="worst-value">{dataThresholds.土壤PH值.worst}</span>
                  <div className="preview-bar">
                    <div className="preview-indicator" style={{
                      left: `${((parseFloat(dns2[0]?.土壤PH值 || "0") - dataThresholds.土壤PH值.worst) / 
                      (dataThresholds.土壤PH值.best - dataThresholds.土壤PH值.worst)) * 100}%`
                    }}></div>
                  </div>
                  <span className="best-value">{dataThresholds.土壤PH值.best}</span>
                </div>
              </div>
              <div className="threshold-inputs">
                <div className="input-group">
                  <label>最佳值:</label>
                  <input 
                    type="number"
                    value={dataThresholds.土壤PH值.best}
                    onChange={(e) => setDataThresholds({
                      ...dataThresholds,
                      土壤PH值: {
                        ...dataThresholds.土壤PH值,
                        best: parseFloat(e.target.value)
                      }
                    })}
                  />
                </div>
                <div className="input-group">
                  <label>最差值:</label>
                  <input 
                    type="number"
                    value={dataThresholds.土壤PH值.worst}
                    onChange={(e) => setDataThresholds({
                      ...dataThresholds,
                      土壤PH值: {
                        ...dataThresholds.土壤PH值,
                        worst: parseFloat(e.target.value)
                      }
                    })}
                  />
                </div>
              </div>
            </div>
          </div>
        )}
      </div>
    </div>
  );
};

export default ThresholdSettings; 