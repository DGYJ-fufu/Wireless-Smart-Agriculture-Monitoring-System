/**
 * 智慧农业物联网可视化平台 - 环境配置文件
 * 版权所有 (c) 2023-2025 
 * 开发者: 黄浩、王伟
 * 联系方式: 2768607063@163.com
 * 未经授权，禁止复制、修改或分发本代码
 */

interface Config {
  API_URL: string;         // 数据API接口地址
  CONTROL_API_URL: string; // 设备控制API接口地址，指向SendCmdServer.py服务
  APP_TITLE: string;       // 应用标题
  HISTORY_API_URL: string; // 历史数据API接口地址，指向server.ts服务
}

// 开发环境配置
const config: Config = {
  API_URL: 'http://localhost:5555/api/',
  CONTROL_API_URL: 'http://localhost:9600/', // SendCmdServer.py服务地址，所有设备控制命令都通过POST方法发送到此地址
  APP_TITLE: '智慧农业物联网可视化监控平台',
  HISTORY_API_URL: 'http://localhost:5555/api/' // 历史数据API接口地址，使用真实数据
};

// 注意：在生产环境部署时，需要修改以下配置
// 例如: API_URL: '/api/', CONTROL_API_URL: '/control/', HISTORY_API_URL: '/api/'

/**
 * 重要说明：
 * 
 * 所有设备控制功能（包括自动控制和手动控制）都通过向SendCmdServer.py服务发送POST请求实现。
 * 
 * 设备控制API包括：
 * - 风扇开关: POST /stft (开), POST /stff (关)
 * - 生长灯开关: POST /stgt (开), POST /stgf (关)
 * - 水泵开关: POST /stpt (开), POST /stpf (关)
 * - 风扇速度设置: POST /setFanSpeed/{value}
 * - 水泵速度设置: POST /setPumpSpeed/{value}
 * 
 * 自动控制和手动控制的区别通过请求头 X-Auto-Control 标识：
 * - 手动控制: X-Auto-Control: false
 * - 自动控制: X-Auto-Control: true
 * 
 * 历史数据获取：
 * - 大棚内检测数据: GET /AgriculturalTestingDatas
 * - 土壤成分数据: GET /AgriculturalTestingDatas_2
 * - 控制节点数据: GET /AgriculturalControlDatas
 * - 环境检测数据: GET /Agri-environmentDatas
 */

export default config; 