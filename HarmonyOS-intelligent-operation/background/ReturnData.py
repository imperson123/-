import os
import time
import psutil
from datetime import datetime, timedelta
import random

def return_anomaly_data(dataset="nyc_taxi"):
    """根据数据集参数返回对应异常数据，nyc_taxi数据集返回2025年数据"""
    if dataset == "nyc_taxi":
        return [
            {
                "timestamp": "2025-08-15 09:00:00",
                "prediction": 5.313,
                "real_value": 2.431,
                "is_anomaly": 1
            },
            {
                "timestamp": "2025-08-15 09:05:00",
                "prediction": 5.657,
                "real_value": 3.039,
                "is_anomaly": 1
            },
            {
                "timestamp": "2025-08-15 09:10:00",
                "prediction": 6.315,
                "real_value": 4.001,
                "is_anomaly": 1
            }
        ]
    # 其他数据集 - 保留原有实时数据逻辑
    else:
        historical_data = [
            {"prediction": 6.490, "real_value": 4.402},
            {"prediction": 4.930, "real_value": 2.840},
            {"prediction": 5.617, "real_value": 3.322}
        ]
        real_time_data = []
        now = datetime.now()
        for i, item in enumerate(historical_data):
            delta = timedelta(minutes=25 - i*5)
            real_time = now - delta
            real_time_data.append({
                "timestamp": real_time.strftime("%Y-%m-%d %H:%M:%S"),
                "prediction": item["prediction"],
                "real_value": item["real_value"],
                "is_anomaly": 1
            })
        return real_time_data

# 恢复系统监控数据采集函数完整实现
def return_monitor_data_dynamic_one():
    """采集并返回系统实时监控数据"""
    try:
        current_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        
        # CPU数据
        cpu_percent = psutil.cpu_percent(interval=1, percpu=True)
        cpu_count = psutil.cpu_count(logical=True)
        
        # 磁盘数据（Windows系统适配）
        if os.name == 'nt':
            disk_partitions = psutil.disk_partitions()
            # 找到系统盘（通常是C盘）
            system_disk = next((p for p in disk_partitions if p.fstype and 'fixed' in p.opts), disk_partitions[0])
            disk_usage = psutil.disk_usage(system_disk.mountpoint)
        else:
            disk_usage = psutil.disk_usage('/')
        
        # 内存数据
        memory = psutil.virtual_memory()
        
        # 网络数据
        net_io = psutil.net_io_counters()
        net_data = {
            "bytes_recv": net_io.bytes_recv,
            "bytes_sent": net_io.bytes_sent,
            "packets_recv": net_io.packets_recv,
            "packets_sent": net_io.packets_sent
        }
        
        # 返回完整的系统监控数据
        return {
            "timestamp": current_time,
            "cpu_data": {
                "cpu_percent": cpu_percent,
                "cpu_count": cpu_count
            },
            "disk_data": {
                "disk_usage": {
                    "total": round(disk_usage.total / (1024**3), 2),  # 转换为GB
                    "used": round(disk_usage.used / (1024**3), 2),
                    "free": round(disk_usage.free / (1024**3), 2),
                    "percent": disk_usage.percent
                }
            },
            "memory_data": {
                "basic_info": {
                    "total": round(memory.total / (1024**3), 2),  # GB
                    "available": round(memory.available / (1024**3), 2),
                    "percent": memory.percent
                }
            },
            "net_data": net_data
        }
        
    except Exception as e:
        return {
            "error": f"数据采集错误: {str(e)}",
            "timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        }

def return_monitor_data_dynamic(num):
    """返回动态预测数据"""
    try:
        # 获取最新实时数据
        real_data = return_monitor_data_dynamic_one()
        if "error" in real_data:
            return real_data
            
        current_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        data_length = 30
        
        # 生成基于CPU使用率的预测数据
        cpu_avg = sum(real_data["cpu_data"]["cpu_percent"]) / len(real_data["cpu_data"]["cpu_percent"])
        real_values = [round(cpu_avg + random.uniform(-5, 5), 2) for _ in range(data_length)]
        predict_values = [round(val + random.uniform(-3, 3), 2) for val in real_values[-10:]]
        
        return {
            "timestamp": current_time,
            "real_values": real_values,
            "predict_values": predict_values,
            "num": num
        }
    except Exception as e:
        return {
            "error": f"预测数据错误: {str(e)}",
            "timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        }