const core = require('@huaweicloud/huaweicloud-sdk-core');
const iotda = require("@huaweicloud/huaweicloud-sdk-iotda/v5/public-api");

// 认证信息
const ak = "HPUASH7FTUYMZRITJ9VR";
const sk = "HCPnmgSO8q19dhovBBIE7c2bZyccYHRpk2baveoq";
const endpoint = "https://iotda.cn-north-4.myhuaweicloud.com";
const project_id = "b109a4cea1314b5f879aaaaa9b36cc3c";

const credentials = new core.BasicCredentials()
                     .withAk(ak)
                     .withSk(sk)
                     .withProjectId(project_id);
                     
const client = iotda.IoTDAClient.newBuilder()
                            .withCredential(credentials)
                            .withEndpoint(endpoint)
                            .build();
                            
const request = new iotda.CreateCommandRequest();
request.deviceId = "you device_id";
request.instanceId = "you instance_id";

// 修改这里：将参数改为对象格式
const body = new iotda.DeviceCommandRequest();
body.withCommandName("setFanStatus");
// 参数应该是一个对象，而不是字符串
body.withParas({
    "status": true  // 或使用 "status": "TRUE"，取决于设备期望的格式
});

request.withBody(body);

const result = client.createCommand(request);
result.then(result => {
    console.log("JSON.stringify(result)::" + JSON.stringify(result));
}).catch(ex => {
    console.log("exception:" + JSON.stringify(ex));
});
