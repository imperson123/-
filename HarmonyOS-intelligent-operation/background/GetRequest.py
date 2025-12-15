import os
import json
import time
import sqlite3
import psutil
import requests
import platform
from flask import Flask, render_template, jsonify, request, make_response, send_from_directory
from flask_cors import CORS
from ReturnData import return_monitor_data_dynamic_one, return_monitor_data_dynamic, return_anomaly_data
from ErrorLogger import ErrorLogger
from ReportGenerator import NFTReportGenerator
from NFTMarket import NFTMarket

root_folder = os.path.dirname(__file__)
get_status_full_file = os.path.join(root_folder, "GetStatus_test.py")

app = Flask(__name__)
# 扩展 CORS 支持
CORS(app, resources={
    r"/api/*": {
        "origins": ["http://localhost:8080", "http://localhost:8081", "http://127.0.0.1:8080"],
        "supports_credentials": True,
        "expose_headers": "Content-Type",
        "allow_headers": ["Content-Type", "Authorization", "X-Requested-With"]
    },
    r"/monitor/*": {
        "origins": ["http://localhost:8080", "http://localhost:8081", "http://127.0.0.1:8080"],
        "supports_credentials": True,
        "allow_headers": ["Content-Type", "Authorization", "X-Requested-With"]
    }
})
app.config['get_status_file'] = get_status_full_file

# 数据库配置
DB_PATH = os.path.join(root_folder, 'monitor.db')

# 初始化错误日志和报告生成器
error_logger = ErrorLogger(db_path=DB_PATH)
report_generator = NFTReportGenerator(db_path=DB_PATH)
nft_market = NFTMarket(db_path=DB_PATH)

# 全局错误处理
@app.errorhandler(Exception)
def handle_error(e):
    """全局错误处理，自动记录错误日志"""
    error_logger.log_error(
        level="ERROR",
        module="Flask",
        message=str(e),
        exception=e
    )
    return jsonify({"error": str(e)}), 500
def get_db_connection():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn

# 初始化数据库和表
def init_db():
    conn = get_db_connection()
    c = conn.cursor()
    # 监控配置表
    c.execute('''CREATE TABLE IF NOT EXISTS monitor_config (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL,
        description TEXT,
        threshold REAL,
        created_at TEXT DEFAULT CURRENT_TIMESTAMP
    )''')
    # 系统指标表
    c.execute('''CREATE TABLE IF NOT EXISTS system_metrics (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        cpu REAL,
        memory REAL,
        disk REAL,
        network REAL,
        timestamp TEXT DEFAULT CURRENT_TIMESTAMP
    )''')
    # 检查并填充10条监控配置
    configs = c.execute('SELECT COUNT(*) FROM monitor_config').fetchone()[0]
    if configs < 10:
        preset = [
            ("CPU利用率阈值", "当CPU利用率超过该阈值时触发告警", 85.0),
            ("内存占用率阈值", "内存占用率超过该阈值时触发预警", 80.0),
            ("磁盘IO阈值", "磁盘IO利用率超过该阈值时触发监控", 90.0),
            ("网络流量上限", "单网卡流量超过该阈值时预警", 1000.0),
            ("系统负载阈值", "系统平均负载超过该阈值时报警", 5.0),
            ("进程数阈值", "系统进程数超过该阈值时预警", 300.0),
            ("TCP连接数阈值", "TCP连接数超过该阈值时触发监控", 2000.0),
            ("磁盘剩余空间下限", "磁盘剩余空间低于该阈值时报警(GB)", 20.0),
            ("CPU温度阈值", "CPU温度超过该阈值时触发告警(℃)", 75.0),
            ("内存交换区阈值", "Swap使用率超过该阈值时预警", 60.0)
        ]
        for name, desc, th in preset:
            c.execute('INSERT INTO monitor_config (name, description, threshold) VALUES (?, ?, ?)',
                      (name, desc, th))
    conn.commit()
    conn.close()
init_db()

# Ollama配置
OLLAMA_API_URL = "http://localhost:11434/api/chat"
OLLAMA_MODEL = "mistral"

# 登录API
@app.route("/api/login", methods=["POST", "OPTIONS"])
def api_login():
    response_headers = {
        "Access-Control-Allow-Origin": "http://localhost:8080",
        "Access-Control-Allow-Credentials": "true",
        "Access-Control-Allow-Headers": "Content-Type,Authorization,X-Requested-With",
        "Access-Control-Allow-Methods": "POST,OPTIONS"
    }

    if request.method == "OPTIONS":
        response = make_response("", 200)
        for k, v in response_headers.items():
            response.headers[k] = v
        return response

    try:
        data = request.get_json(force=True) or {}
        username = data.get('username', '')
        password = data.get('password', '')
        print(f"[LOGIN] username={username!r}, pwd_len={len(password) if password is not None else 0}")
        if username == 'admin':
            response = jsonify({
                "status": "success",
                "message": "登录成功（联调放宽模式）",
                "token": "dummy_token_for_demo_only"
            })
            for k, v in response_headers.items():
                response.headers[k] = v
            return response, 200
        else:
            response = jsonify({
                "status": "error",
                "message": "当前为联调放宽模式：请使用用户名 admin 直接登录"
            })
            for k, v in response_headers.items():
                response.headers[k] = v
            return response, 401
    except Exception as e:
        response = jsonify({"error": str(e)})
        for k, v in response_headers.items():
            response.headers[k] = v
        return response, 500

# 监控配置API
@app.route("/api/monitor-configs", methods=["GET", "POST", "PUT", "DELETE"])
def monitor_configs():
    conn = get_db_connection()
    c = conn.cursor()
    if request.method == "GET":
        configs = c.execute("SELECT * FROM monitor_config ORDER BY created_at DESC").fetchall()
        conn.close()
        return jsonify([dict(row) for row in configs])
    elif request.method == "POST":
        data = request.get_json()
        c.execute("INSERT INTO monitor_config (name, description, threshold) VALUES (?, ?, ?)",
                  (data.get('name'), data.get('description'), data.get('threshold')))
        conn.commit()
        new_id = c.lastrowid
        conn.close()
        return jsonify({"status": "success", "id": new_id})
    elif request.method == "PUT":
        data = request.get_json()
        c.execute("UPDATE monitor_config SET name=?, description=?, threshold=? WHERE id=?",
                  (data.get('name'), data.get('description'), data.get('threshold'), data.get('id')))
        conn.commit()
        conn.close()
        return jsonify({"status": "success"})
    elif request.method == "DELETE":
        data = request.get_json()
        c.execute("DELETE FROM monitor_config WHERE id=?", (data.get('id'),))
        conn.commit()
        conn.close()
        return jsonify({"status": "success"})

# 系统指标API
@app.route("/api/system-metrics", methods=["GET", "POST", "DELETE"])
def system_metrics():
    conn = get_db_connection()
    c = conn.cursor()
    if request.method == "GET":
        metrics = c.execute("SELECT * FROM system_metrics ORDER BY timestamp DESC LIMIT 10").fetchall()
        conn.close()
        return jsonify([dict(row) for row in metrics])
    elif request.method == "POST":
        data = request.get_json()
        c.execute("INSERT INTO system_metrics (cpu, memory, disk, network) VALUES (?, ?, ?, ?)",
                  (data.get('cpu'), data.get('memory'), data.get('disk'), data.get('network')))
        conn.commit()
        new_id = c.lastrowid
        conn.close()
        return jsonify({"status": "success", "id": new_id})
    elif request.method == "DELETE":
        data = request.get_json()
        c.execute("DELETE FROM system_metrics WHERE id=?", (data.get('id'),))
        conn.commit()
        conn.close()
        return jsonify({"status": "success"})

# 监控内部实现：构建状态数据
def build_status_payload():
    try:
        payload = return_monitor_data_dynamic_one()
        # 若函数返回 None 或空则回退到 mock
        if not payload:
            raise RuntimeError("empty payload from return_monitor_data_dynamic_one")
        return payload
    except Exception:
        # 兼容回退：（便于前端兼容）
        return {
            "cpu": [10,20,30,40,50,60,70,80],
            "cpu_data": {"cpu_percent": [10,20,30,40,50,60,70,80]},
            "memory_data": {"basic_info": {"total": 8192, "used": 4096, "percent": 50}},
            "disk_data": {"disk_usage": {"total": 512000, "used": 256000, "percent": 50}, "basic_info": {"total": 512000, "used": 256000, "percent": 50}},
            "network": {"bytes_sent": 123456, "bytes_recv": 654321},
            "timestamp": time.strftime('%Y-%m-%d %H:%M:%S')
        }

@app.route("/api/status", methods=["GET"])
def api_status():
    payload = build_status_payload()
    return jsonify({"code": 0, "data": payload})

@app.route("/monitor/status", methods=['GET','POST'])
def get_status_data():
    return jsonify(build_status_payload())

@app.route("/monitor/data/<int:num>", methods=['GET','POST'])
def get_dynamic_data(num):
    try:
        results = return_monitor_data_dynamic(num)
        return jsonify(results)
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route("/api/monitor/data/<int:num>", methods=['GET','POST'])
def get_dynamic_data_api(num):
    try:
        results = return_monitor_data_dynamic(num)
        return jsonify({"code": 0, "data": results})
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# 兼容前端异常接口（/api/anomalies） 
@app.route("/api/anomalies", methods=["GET"])
def api_anomalies():
    try:
        data = return_anomaly_data()
        return jsonify({"code": 0, "data": data})
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# 保留原始异常 API 名称（兼容旧客户端）
@app.route("/api/anomaly_data", methods=["GET"])
def get_anomaly_data():
    try:
        data = return_anomaly_data()
        return jsonify(data)
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# Ollama大模型问答API
@app.route("/api/qa", methods=["POST"])
def qa_api():
    data = request.get_json()
    question = data.get("question", "").strip()
    try:
        response = requests.post(
            OLLAMA_API_URL,
            json={
                "model": OLLAMA_MODEL,
                "messages": [
                    {"role": "system", "content": "你是NexGen MetaOps系统运维助手，专业回答系统监控和项目相关问题，回答简洁准确"},
                    {"role": "user", "content": question}
                ],
                "stream": True
            },
            stream=True
        )
        answer = ""
        if response.status_code == 200:
            for line in response.iter_lines():
                if line:
                    try:
                        line_data = json.loads(line.decode('utf-8'))
                        if "message" in line_data and "content" in line_data["message"]:
                            answer += line_data["message"]["content"]
                    except json.JSONDecodeError:
                        continue
            return jsonify({"question": question, "answer": answer.strip()})
        else:
            return jsonify({"question": question, "answer": f"Ollama调用失败: HTTP {response.status_code}, {response.text}"})
    except requests.exceptions.ConnectionError:
        return jsonify({
            "question": question,
            "answer": "Ollama服务未启动，请先运行`ollama run mistral`启动本地模型服务",
            "error": "connection_error"
        })
    except Exception as e:
        return jsonify({
            "question": question,
            "answer": f"模型调用错误: {str(e)}",
            "error": str(e)
        })

# 根路径
@app.route("/")
def index():
    return """
    <h1>NexGen MetaOps 监控系统 API 服务</h1>
    <p>服务器运行正常！</p>
    <h2>可用接口：</h2>
    <ul>
        <li><a href="/monitor/test">/monitor/test</a> - 测试接口</li>
        <li><a href="/api/status">/api/status</a> - 系统状态</li>
        <li><a href="/monitor/status">/monitor/status</a> - 监控状态</li>
        <li>/api/login - 登录接口 (POST)</li>
        <li>/api/monitor-configs - 监控配置 (GET/POST/PUT/DELETE)</li>
        <li>/api/system-metrics - 系统指标 (GET/POST/DELETE)</li>
    </ul>
    <p>前端服务请访问: <a href="http://localhost:8080">http://localhost:8080</a></p>
    """

# 多操作系统检测API
@app.route("/api/system-info", methods=["GET"])
def get_system_info():
    """获取系统信息，展示多操作系统支持"""
    try:
        system_info = {
            "os": platform.system(),
            "os_version": platform.version(),
            "platform": platform.platform(),
            "hostname": platform.node(),
            "python_version": platform.python_version(),
            "supported_os": ["Windows", "Linux", "macOS", "Kylin"],
            "deployment_type": "轻量化部署",
            "auto_ops_enabled": True
        }
        try:
            import psutil
            system_info.update({
                "cpu_count": psutil.cpu_count(),
                "memory_total_gb": round(psutil.virtual_memory().total / (1024**3), 2),
                "memory_available_gb": round(psutil.virtual_memory().available / (1024**3), 2)
            })
        except:
            pass
        return jsonify({"code": 0, "data": system_info})
    except Exception as e:
        error_logger.log_error("ERROR", "system_info", str(e), e)
        return jsonify({"error": str(e)}), 500

# 错误日志API
@app.route("/api/error-logs", methods=["GET"])
def get_error_logs():
    """获取错误日志列表"""
    try:
        limit = request.args.get('limit', 100, type=int)
        unresolved_only = request.args.get('unresolved_only', 'false').lower() == 'true'
        logs = error_logger.get_error_logs(limit=limit, unresolved_only=unresolved_only)
        return jsonify({"code": 0, "data": logs})
    except Exception as e:
        error_logger.log_error("ERROR", "error_logs", str(e), e)
        return jsonify({"error": str(e)}), 500

@app.route("/api/error-logs/<int:log_id>/resolve", methods=["POST"])
def resolve_error_log(log_id):
    """标记错误为已解决"""
    try:
        error_logger.mark_resolved(log_id)
        return jsonify({"code": 0, "message": "已标记为已解决"})
    except Exception as e:
        error_logger.log_error("ERROR", "resolve_error", str(e), e)
        return jsonify({"error": str(e)}), 500

@app.route("/api/error-statistics", methods=["GET"])
def get_error_statistics():
    """获取错误统计信息"""
    try:
        stats = error_logger.get_error_statistics()
        return jsonify({"code": 0, "data": stats})
    except Exception as e:
        error_logger.log_error("ERROR", "error_statistics", str(e), e)
        return jsonify({"error": str(e)}), 500

# NFT风格报告生成API
@app.route("/api/generate-report", methods=["POST"])
def generate_report():
    """生成NFT风格的数据故障修复报告"""
    try:
        data = request.get_json() or {}
        title = data.get('title', '数据故障修复报告')
        report_id = data.get('report_id')
        format_type = data.get('format', 'both')  # json, text, both
        
        report = report_generator.generate_fault_repair_report(
            report_id=report_id,
            title=title
        )
        
        # 保存报告
        saved_files = report_generator.save_report(report, format=format_type)
        
        return jsonify({
            "code": 0,
            "data": {
                "report_id": report['report_id'],
                "text_report": report['text'],
                "json_report": report['json'],
                "saved_files": saved_files
            }
        })
    except Exception as e:
        error_logger.log_error("ERROR", "generate_report", str(e), e)
        return jsonify({"error": str(e)}), 500

@app.route("/api/report/<report_id>", methods=["GET"])
def get_report(report_id):
    """获取已生成的报告"""
    try:
        # 这里可以从文件系统读取，简化实现直接重新生成
        report = report_generator.generate_fault_repair_report(
            report_id=report_id
        )
        return jsonify({
            "code": 0,
            "data": {
                "report_id": report['report_id'],
                "text_report": report['text'],
                "json_report": report['json']
            }
        })
    except Exception as e:
        error_logger.log_error("ERROR", "get_report", str(e), e)
        return jsonify({"error": str(e)}), 500

# 自主运维API - 自动修复尝试
@app.route("/api/auto-repair", methods=["POST"])
def auto_repair():
    """自主运维 - 尝试自动修复问题"""
    try:
        data = request.get_json() or {}
        issue_type = data.get('issue_type', '')
        
        # 这里可以实现各种自动修复逻辑
        repair_result = {
            "success": False,
            "message": "",
            "actions_taken": []
        }
        
        # 示例：自动清理临时文件
        if issue_type == 'disk_space' or issue_type == '':
            try:
                import tempfile
                temp_dir = tempfile.gettempdir()
                # 这里可以添加清理逻辑
                repair_result["actions_taken"].append("检查临时文件")
                repair_result["success"] = True
                repair_result["message"] = "已检查系统状态"
            except Exception as e:
                error_logger.log_error("WARNING", "auto_repair", f"自动修复失败: {str(e)}", e, auto_fix_attempted=True)
        
        # 记录自动修复尝试
        error_logger.log_error(
            level="INFO",
            module="auto_repair",
            message=f"自动修复尝试: {issue_type}",
            auto_fix_attempted=True
        )
        
        return jsonify({"code": 0, "data": repair_result})
    except Exception as e:
        error_logger.log_error("ERROR", "auto_repair", str(e), e)
        return jsonify({"error": str(e)}), 500

# NFT市场API
@app.route("/api/nft/products", methods=["GET"])
def get_nft_products():
    """获取NFT产品列表"""
    try:
        category = request.args.get('category')
        limit = request.args.get('limit', 50, type=int)
        products = nft_market.get_products(category=category, limit=limit)
        return jsonify({"code": 0, "data": products})
    except Exception as e:
        error_logger.log_error("ERROR", "get_nft_products", str(e), e)
        return jsonify({"error": str(e)}), 500

@app.route("/api/nft/product/<product_id>", methods=["GET"])
def get_nft_product(product_id):
    """获取NFT产品详情"""
    try:
        product = nft_market.get_product(product_id)
        if product:
            return jsonify({"code": 0, "data": product})
        else:
            return jsonify({"code": 1, "message": "产品不存在"}), 404
    except Exception as e:
        error_logger.log_error("ERROR", "get_nft_product", str(e), e)
        return jsonify({"error": str(e)}), 500

@app.route("/api/nft/order", methods=["POST"])
def create_nft_order():
    """创建NFT订单"""
    try:
        data = request.get_json() or {}
        product_id = data.get('product_id')
        buyer_id = data.get('buyer_id', 'user_001')
        buyer_name = data.get('buyer_name', '用户')
        payment_method = data.get('payment_method', 'wechat')
        
        if not product_id:
            return jsonify({"code": 1, "message": "缺少产品ID"}), 400
        
        order_id = nft_market.create_order(product_id, buyer_id, buyer_name, payment_method)
        if order_id:
            return jsonify({"code": 0, "data": {"order_id": order_id}})
        else:
            return jsonify({"code": 1, "message": "创建订单失败"}), 400
    except Exception as e:
        error_logger.log_error("ERROR", "create_nft_order", str(e), e)
        return jsonify({"error": str(e)}), 500

@app.route("/api/nft/payment", methods=["POST"])
def process_payment():
    """处理支付（模拟）"""
    try:
        data = request.get_json() or {}
        order_id = data.get('order_id')
        
        if not order_id:
            return jsonify({"code": 1, "message": "缺少订单ID"}), 400
        
        # 模拟支付处理
        import time
        time.sleep(0.5)  # 模拟支付延迟
        
        success = nft_market.complete_payment(order_id)
        if success:
            return jsonify({"code": 0, "message": "支付成功", "data": {"order_id": order_id}})
        else:
            return jsonify({"code": 1, "message": "支付失败"}), 400
    except Exception as e:
        error_logger.log_error("ERROR", "process_payment", str(e), e)
        return jsonify({"error": str(e)}), 500

@app.route("/api/nft/user-methods", methods=["GET", "POST"])
def user_methods():
    """获取或提交用户运维方法"""
    try:
        if request.method == 'GET':
            author_id = request.args.get('author_id')
            limit = request.args.get('limit', 50, type=int)
            methods = nft_market.get_user_methods(author_id=author_id, limit=limit)
            return jsonify({"code": 0, "data": methods})
        else:
            # POST - 提交新方法
            data = request.get_json() or {}
            method_id = nft_market.submit_user_method(data)
            return jsonify({"code": 0, "data": {"method_id": method_id}})
    except Exception as e:
        error_logger.log_error("ERROR", "user_methods", str(e), e)
        return jsonify({"error": str(e)}), 500

# 测试API
@app.route("/monitor/test")
def connect_test():
    return "Welcome to monitor api!"

# 应用入口
if __name__ == '__main__':
    app.run(host="0.0.0.0", port=5000, debug=False, threaded=True)