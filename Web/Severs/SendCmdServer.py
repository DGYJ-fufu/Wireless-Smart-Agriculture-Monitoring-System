from bottle import route, run
import os
from huaweicloudsdkcore.auth.credentials import BasicCredentials
from huaweicloudsdkcore.auth.credentials import DerivedCredentials
from huaweicloudsdkcore.region.region import Region as coreRegion
from huaweicloudsdkcore.exceptions import exceptions
from huaweicloudsdkiotda.v5 import *
from bottle import request, response
import json
from huaweicloudsdkcore.http.http_config import HttpConfig
from concurrent.futures import ThreadPoolExecutor, TimeoutError as FuturesTimeoutError

# 添加CORS支持
def enable_cors(fn):
    def _enable_cors(*args, **kwargs):
        # 设置响应头
        response.headers['Access-Control-Allow-Origin'] = '*'
        response.headers['Access-Control-Allow-Methods'] = 'GET, POST, PUT, OPTIONS'
        response.headers['Access-Control-Allow-Headers'] = 'Origin, Accept, Content-Type, X-Requested-With, X-CSRF-Token, X-Auto-Control'
        response.headers['Access-Control-Expose-Headers'] = 'Content-Type, X-Auto-Control'
        response.headers['Access-Control-Max-Age'] = '86400'  # 预检请求缓存24小时

        if request.method != 'OPTIONS':
            # 实际请求
            return fn(*args, **kwargs)
        else:
            # 预检请求
            return {}

    return _enable_cors

#华为云鉴权
ak = "YOUXXXXXXXXX"
sk = "YOUXXXXXXXXX"
# ENDPOINT：请在控制台的"总览"界面的"平台接入地址"中查看"应用侧"的https接入地址，下面创建Client时需要使用自行创建的Region对象，基础版：请选择IoTDAClient中的Region对象 如： IoTDAClient.new_builder().with_region(IoTDARegion.CN_NORTH_4)
endpoint = "YOUXXXXXXXXX";
# === 新增: 配置 Http 超时与重试 ===
http_config = HttpConfig.get_default_config()
# 单位：秒，连接超时 + 读取超时，视业务需要自行调整
http_config.timeout = 5

credentials = BasicCredentials(ak, sk).with_derived_predicate(DerivedCredentials.get_default_derived_predicate())
client = IoTDAClient.new_builder() \
    .with_http_config(http_config) \
    .with_credentials(credentials) \
    .with_region(coreRegion(id="cn-north-4", endpoint=endpoint)) \
    .build()

# === 新增: 使用线程池异步下发指令，避免阻塞 Bottle 主线程 ===
executor = ThreadPoolExecutor(max_workers=8)

def send_command(paras: str, command_name: str, service_id: str = "control", timeout: int = 5):
    """异步发送设备指令，timeout 秒未完成即返回失败"""
    req = CreateCommandRequest()
    req.device_id = "you device_id"
    req.instance_id = "you instance_id"
    req.body = DeviceCommandRequest(
        paras=paras,
        command_name=command_name,
        service_id=service_id
    )
    future = executor.submit(client.create_command, req)
    try:
        resp = future.result(timeout=timeout)
        return True, resp
    except FuturesTimeoutError:
        future.cancel()
        return False, "请求超时"
    except exceptions.ClientRequestException as e:
        return False, e.error_msg

# 配置MainController类
class MainController:
    def index(self):
        return "Welcome to the IoT Device Control Server!"
        
    def handle_param_setFanSpeed(self, param):
        """设置风扇速度"""
        param = str(param)
        success, result = send_command(f"{{\"speed\":{param}}}", "setFanSpeed")
        if success:
            print(f"设置风扇速度为: {param}")
            return json.dumps({"status": "success", "message": f"设置风扇速度为: {param}"})
        else:
            print(result)
            return json.dumps({"status": "error", "message": str(result)})
            
    def handle_param_setPumpSpeed(self, param):
        """设置水泵速度"""
        param = str(param)
        success, result = send_command(f"{{\"speed\":{param}}}", "setPumpSpeed")
        if success:
            print(f"设置水泵速度为: {param}")
            return json.dumps({"status": "success", "message": f"设置水泵速度为: {param}"})
        else:
            print(result)
            return json.dumps({"status": "error", "message": str(result)})
            
controller = MainController()

@route('/stff', method=["POST", "OPTIONS"])
@enable_cors
def setFanStatusFlase():
    success, result = send_command("{\"status\":false}", "setFanStatus")
    if success:
        print("设置风扇状态为关闭")
        return json.dumps({"status": "success", "message": "设置风扇状态为关闭"})
    else:
        print(result)
        return json.dumps({"status": "error", "message": str(result)})
    
@route('/stft', method=["POST", "OPTIONS"])
@enable_cors
def setFanStatusTrue():
    success, result = send_command("{\"status\":true}", "setFanStatus")
    if success:
        print("设置风扇状态为开启")
        return json.dumps({"status": "success", "message": "设置风扇状态为开启"})
    else:
        print(result)
        return json.dumps({"status": "error", "message": str(result)})

#setGrowLightStatus TRUE/FLASE 生长灯状态
@route('/stgf', method=["POST", "OPTIONS"])
@enable_cors
def setGrowLightStatusFlase():
    success, result = send_command("{\"status\":false}", "setGrowLightStatus")
    if success:
        print("设置生长灯状态为关闭")
        return json.dumps({"status": "success", "message": "设置生长灯状态为关闭"})
    else:
        print(result)
        return json.dumps({"status": "error", "message": str(result)})
    
@route('/stgt', method=["POST", "OPTIONS"])
@enable_cors
def setGrowLightStatusTrue():
    success, result = send_command("{\"status\":true}", "setGrowLightStatus")
    if success:
        print("设置生长灯状态为开启")
        return json.dumps({"status": "success", "message": "设置生长灯状态为开启"})
    else:
        print(result)
        return json.dumps({"status": "error", "message": str(result)})

#setPumpStatus TRUE/FLASE 水泵状态
@route('/stpf', method=["POST", "OPTIONS"])
@enable_cors
def setPumpStatusFlase():
    success, result = send_command("{\"status\":false}", "setPumpStatus")
    if success:
        print("设置水泵状态为关闭")
        return json.dumps({"status": "success", "message": "设置水泵状态为关闭"})
    else:
        print(result)
        return json.dumps({"status": "error", "message": str(result)})
    
@route('/stpt', method=["POST", "OPTIONS"])
@enable_cors
def setPumpStatusTrue():
    success, result = send_command("{\"status\":true}", "setPumpStatus")
    if success:
        print("设置水泵状态为开启")
        return json.dumps({"status": "success", "message": "设置水泵状态为开启"})
    else:
        print(result)
        return json.dumps({"status": "error", "message": str(result)})

#setFanSpeed int(0-100)
@route('/setFanSpeed/<param>', method=["POST", "OPTIONS"])
@enable_cors
def setFanSpeed(param):
    return controller.handle_param_setFanSpeed(param)

#setPumpSpeed int(0-100)
@route('/setPumpSpeed/<param>', method=["POST", "OPTIONS"])
@enable_cors
def setPumpSpeed(param):
    return controller.handle_param_setPumpSpeed(param)

# 添加根路由，用于测试服务是否正常运行
@route('/', method=["GET", "OPTIONS"])
@enable_cors
def index():
    return controller.index()
    
if __name__ == '__main__':
    try:
        print("="*50)
        print("IoT Device Control Server starting on http://localhost:9600")
        print("支持的接口:")
        print("  - 风扇控制: /stft (开启), /stff (关闭), /setFanSpeed/<速度值> (设置风扇速度)")
        print("  - 生长灯控制: /stgt (开启), /stgf (关闭)")
        print("  - 水泵控制: /stpt (开启), /stpf (关闭), /setPumpSpeed/<速度值> (设置水泵速度)")
        print("所有请求均支持CORS跨域访问")
        print("="*50)
        run(host='localhost', port=9600, debug=True)
    except Exception as e:
        print(f"服务器启动失败: {str(e)}")
        print("请检查端口9600是否被占用，或者尝试修改代码中的端口号")