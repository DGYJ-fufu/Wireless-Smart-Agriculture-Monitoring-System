#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
智慧农业物联网可视化平台 - 华为云IoT设备接入模块
版权所有 (c) 2023-2025 
开发者: 黄浩、王伟
联系方式: 2768607063@163.com
未经授权，禁止复制、修改或分发本代码
"""

# coding: utf-8

import os
from huaweicloudsdkcore.auth.credentials import BasicCredentials
from huaweicloudsdkcore.auth.credentials import DerivedCredentials
from huaweicloudsdkcore.region.region import Region as coreRegion
from huaweicloudsdkcore.exceptions import exceptions
from huaweicloudsdkiotda.v5 import *

if __name__ == "__main__":
    # The AK and SK used for authentication are hard-coded or stored in plaintext, which has great security risks. It is recommended that the AK and SK be stored in ciphertext in configuration files or environment variables and decrypted during use to ensure security.
    # In this example, AK and SK are stored in environment variables for authentication. Before running this example, set environment variables CLOUD_SDK_AK and CLOUD_SDK_SK in the local environment
    ak = "HPUASOC2EX8AFLYY7JE8"
    sk = "UC8XGyQwLn0cY5gltSG2ufJyTZ2udG0ug3uctSHL"
    # ENDPOINT：请在控制台的"总览"界面的"平台接入地址"中查看“应用侧”的https接入地址，下面创建Client时需要使用自行创建的Region对象，基础版：请选择IoTDAClient中的Region对象 如： IoTDAClient.new_builder().with_region(IoTDARegion.CN_NORTH_4)
    endpoint = "e447071c94.st1.iotda-app.cn-north-4.myhuaweicloud.com";

    credentials = BasicCredentials(ak, sk).with_derived_predicate(DerivedCredentials.get_default_derived_predicate())

    client = IoTDAClient.new_builder() \
        .with_credentials(credentials) \
        .with_region(coreRegion(id="cn-north-4", endpoint=endpoint)) \
        .build()

    try:
        request = CreateCommandRequest()
        request.device_id = "you device_id"
        request.instance_id = "you instance_id"
        request.body = DeviceCommandRequest(
            paras="TRUE",
            command_name="setGrow"
        )
        response = client.create_command(request)
        print(response)
    except exceptions.ClientRequestException as e:
        print(e.status_code)
        print(e.request_id)
        print(e.error_code)
        print(e.error_msg)