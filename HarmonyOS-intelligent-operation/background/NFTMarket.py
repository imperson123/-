"""
NFT市场模块 - 处理NFT交易和用户上传的运维方法
"""
import os
import json
import sqlite3
from datetime import datetime
from pathlib import Path

class NFTMarket:
    """NFT市场管理器"""
    
    def __init__(self, db_path=None):
        self.db_path = db_path or os.path.join(os.path.dirname(__file__), 'monitor.db')
        self._init_db()
    
    def _init_db(self):
        """初始化数据库表"""
        conn = sqlite3.connect(self.db_path)
        c = conn.cursor()
        
        # NFT商品表
        c.execute('''CREATE TABLE IF NOT EXISTS nft_products (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            product_id TEXT UNIQUE NOT NULL,
            title TEXT NOT NULL,
            description TEXT,
            price REAL NOT NULL,
            category TEXT,
            seller_id TEXT,
            seller_name TEXT,
            created_at TEXT DEFAULT CURRENT_TIMESTAMP,
            status TEXT DEFAULT 'active',
            purchase_count INTEGER DEFAULT 0,
            rating REAL DEFAULT 0,
            image_url TEXT,
            script_content TEXT,
            os_support TEXT
        )''')
        
        # 订单表
        c.execute('''CREATE TABLE IF NOT EXISTS nft_orders (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            order_id TEXT UNIQUE NOT NULL,
            product_id TEXT NOT NULL,
            buyer_id TEXT,
            buyer_name TEXT,
            price REAL NOT NULL,
            payment_method TEXT,
            payment_status TEXT DEFAULT 'pending',
            created_at TEXT DEFAULT CURRENT_TIMESTAMP,
            paid_at TEXT,
            FOREIGN KEY (product_id) REFERENCES nft_products(product_id)
        )''')
        
        # 用户上传的运维方法表
        c.execute('''CREATE TABLE IF NOT EXISTS user_methods (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            method_id TEXT UNIQUE NOT NULL,
            title TEXT NOT NULL,
            description TEXT,
            category TEXT,
            author_id TEXT NOT NULL,
            author_name TEXT NOT NULL,
            price REAL DEFAULT 0,
            script_content TEXT,
            os_support TEXT,
            created_at TEXT DEFAULT CURRENT_TIMESTAMP,
            status TEXT DEFAULT 'pending',
            purchase_count INTEGER DEFAULT 0,
            rating REAL DEFAULT 0
        )''')
        
        conn.commit()
        conn.close()
        
        # 初始化一些示例NFT产品
        self._init_sample_products()
    
    def _init_sample_products(self):
        """初始化示例NFT产品"""
        conn = sqlite3.connect(self.db_path)
        c = conn.cursor()
        
        # 检查是否已有产品
        c.execute('SELECT COUNT(*) FROM nft_products')
        if c.fetchone()[0] > 0:
            conn.close()
            return
        
        sample_products = [
            {
                "product_id": "NFT-001",
                "title": "跨OS数据同步故障修复方案",
                "description": "解决麒麟终端与Linux数据不同步问题，支持自动修复脚本",
                "price": 500.0,
                "category": "数据同步",
                "seller_id": "system",
                "seller_name": "NexGen MetaOps官方",
                "os_support": "麒麟,Linux,Windows",
                "script_content": "#!/bin/bash\n# 跨OS数据同步修复脚本\necho '修复中...'"
            },
            {
                "product_id": "NFT-002",
                "title": "跨OS合规审计NFT套餐",
                "description": "包含麒麟补丁安装、多系统日志留存脚本，满足信创验收要求",
                "price": 2000.0,
                "category": "合规审计",
                "seller_id": "system",
                "seller_name": "NexGen MetaOps官方",
                "os_support": "麒麟,统信,Windows",
                "script_content": "#!/bin/bash\n# 合规审计脚本\necho '审计中...'"
            },
            {
                "product_id": "NFT-003",
                "title": "数据传输故障修复NFT",
                "description": "解决门店Windows收银系统与总部Linux管理系统数据不通畅问题",
                "price": 2000.0,
                "category": "数据传输",
                "seller_id": "system",
                "seller_name": "NexGen MetaOps官方",
                "os_support": "Windows,Linux,麒麟",
                "script_content": "#!/bin/bash\n# 数据传输修复脚本\necho '修复中...'"
            }
        ]
        
        for product in sample_products:
            try:
                c.execute('''INSERT INTO nft_products 
                    (product_id, title, description, price, category, seller_id, seller_name, os_support, script_content)
                    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)''',
                    (product['product_id'], product['title'], product['description'], 
                     product['price'], product['category'], product['seller_id'], 
                     product['seller_name'], product['os_support'], product['script_content']))
            except sqlite3.IntegrityError:
                pass  # 已存在，跳过
        
        conn.commit()
        conn.close()
    
    def get_products(self, category=None, limit=50):
        """获取NFT产品列表"""
        conn = sqlite3.connect(self.db_path)
        conn.row_factory = sqlite3.Row
        c = conn.cursor()
        
        if category:
            c.execute('''SELECT * FROM nft_products 
                WHERE status = 'active' AND category = ? 
                ORDER BY created_at DESC LIMIT ?''', (category, limit))
        else:
            c.execute('''SELECT * FROM nft_products 
                WHERE status = 'active' 
                ORDER BY created_at DESC LIMIT ?''', (limit,))
        
        rows = c.fetchall()
        conn.close()
        return [dict(row) for row in rows]
    
    def get_product(self, product_id):
        """获取单个NFT产品详情"""
        conn = sqlite3.connect(self.db_path)
        conn.row_factory = sqlite3.Row
        c = conn.cursor()
        c.execute('SELECT * FROM nft_products WHERE product_id = ?', (product_id,))
        row = c.fetchone()
        conn.close()
        return dict(row) if row else None
    
    def create_order(self, product_id, buyer_id, buyer_name, payment_method='wechat'):
        """创建订单"""
        product = self.get_product(product_id)
        if not product:
            return None
        
        order_id = f"ORDER-{datetime.now().strftime('%Y%m%d%H%M%S')}-{product_id}"
        
        conn = sqlite3.connect(self.db_path)
        c = conn.cursor()
        c.execute('''INSERT INTO nft_orders 
            (order_id, product_id, buyer_id, buyer_name, price, payment_method)
            VALUES (?, ?, ?, ?, ?, ?)''',
            (order_id, product_id, buyer_id, buyer_name, product['price'], payment_method))
        conn.commit()
        conn.close()
        
        return order_id
    
    def complete_payment(self, order_id):
        """完成支付"""
        conn = sqlite3.connect(self.db_path)
        c = conn.cursor()
        
        # 更新订单状态
        c.execute('''UPDATE nft_orders 
            SET payment_status = 'paid', paid_at = ? 
            WHERE order_id = ?''',
            (datetime.now().strftime('%Y-%m-%d %H:%M:%S'), order_id))
        
        # 获取订单信息
        c.execute('SELECT product_id FROM nft_orders WHERE order_id = ?', (order_id,))
        result = c.fetchone()
        if result:
            product_id = result[0]
            # 增加产品购买次数
            c.execute('''UPDATE nft_products 
                SET purchase_count = purchase_count + 1 
                WHERE product_id = ?''', (product_id,))
        
        conn.commit()
        conn.close()
        return True
    
    def submit_user_method(self, method_data):
        """用户提交运维方法"""
        method_id = f"METHOD-{datetime.now().strftime('%Y%m%d%H%M%S')}"
        
        conn = sqlite3.connect(self.db_path)
        c = conn.cursor()
        c.execute('''INSERT INTO user_methods 
            (method_id, title, description, category, author_id, author_name, 
             price, script_content, os_support)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)''',
            (method_id, method_data.get('title'), method_data.get('description'),
             method_data.get('category'), method_data.get('author_id'),
             method_data.get('author_name'), method_data.get('price', 0),
             method_data.get('script_content'), method_data.get('os_support', '')))
        conn.commit()
        conn.close()
        
        return method_id
    
    def get_user_methods(self, author_id=None, limit=50):
        """获取用户上传的运维方法"""
        conn = sqlite3.connect(self.db_path)
        conn.row_factory = sqlite3.Row
        c = conn.cursor()
        
        if author_id:
            c.execute('''SELECT * FROM user_methods 
                WHERE author_id = ? AND status = 'approved'
                ORDER BY created_at DESC LIMIT ?''', (author_id, limit))
        else:
            c.execute('''SELECT * FROM user_methods 
                WHERE status = 'approved'
                ORDER BY created_at DESC LIMIT ?''', (limit,))
        
        rows = c.fetchall()
        conn.close()
        return [dict(row) for row in rows]

