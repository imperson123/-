
import os
import json
import glob
import argparse
import numpy as np
import joblib
import tensorflow as tf
from tensorflow.keras import layers, models, callbacks, optimizers
from sklearn.preprocessing import MinMaxScaler
from datetime import datetime
import random

# 固定随机种子，便于复现
SEED = 42
np.random.seed(SEED)
tf.random.set_seed(SEED)
random.seed(SEED)

def load_series_from_json(path):
    """从单个 JSON 文件加载数值型时间序列（支持 array 或 dict -> values）"""
    with open(path, 'r', encoding='utf-8') as f:
        data = json.load(f)
    if isinstance(data, dict):
        # 尝试常见键
        for k in ('values', 'data', 'series', 'y', 'cpu'):
            if k in data and isinstance(data[k], list):
                return np.array(data[k], dtype=float)
        # 否则提取所有数值型元素
        vals = []
        for v in data.values():
            if isinstance(v, (int, float)):
                vals.append(v)
        if vals:
            return np.array(vals, dtype=float)
        raise ValueError(f"无法解析 JSON 数据: {path}")
    elif isinstance(data, list):
        return np.array(data, dtype=float)
    else:
        raise ValueError(f"不支持的数据格式: {path}")

def gather_time_series(data_dir, pattern="*.json"):
    """收集目录下所有 JSON 文件并返回一个合并的时间序列（简单拼接或取第一个）"""
    files = sorted(glob.glob(os.path.join(data_dir, pattern)))
    if not files:
        raise FileNotFoundError(f"未找到数据文件: {data_dir}")
    # 优先选取明确指标文件（如 cpu_utilization.json），若只有多个文件则拼接
    series_list = []
    for f in files:
        try:
            s = load_series_from_json(f)
            if s.size > 5:  # 略过过短序列
                series_list.append(s)
        except Exception:
            continue
    if not series_list:
        raise ValueError("未能从数据文件中解析出有效序列")
    # 若有多个序列，按时间拼接（保守做法）
    return np.concatenate(series_list)

def create_dataset(series, window_size=32, stride=1):
    X, y = [], []
    for i in range(0, len(series) - window_size, stride):
        X.append(series[i:i+window_size])
        y.append(series[i+window_size])
    X = np.array(X)
    y = np.array(y)
    # reshape 为 (samples, timesteps, features)
    return X.reshape((-1, X.shape[1], 1)), y

class AttentionLayer(layers.Layer):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    def build(self, input_shape):
        self.W = self.add_weight(name='att_weight', shape=(input_shape[-1],), initializer='random_normal', trainable=True)
        super().build(input_shape)

    def call(self, inputs):
        # inputs: (batch, time, features)
        e = tf.tensordot(inputs, self.W, axes=1)  # (batch, time)
        alpha = tf.nn.softmax(e, axis=1)  # (batch, time)
        alpha = tf.expand_dims(alpha, axis=-1)  # (batch, time, 1)
        out = tf.reduce_sum(inputs * alpha, axis=1)  # (batch, features)
        return out

def build_light_haydn_model(input_shape):
    """轻量模型：Conv1D -> BiGRU -> Attention -> Dense"""
    inp = layers.Input(shape=input_shape)
    x = layers.Conv1D(32, kernel_size=3, padding='causal', activation='relu')(inp)
    x = layers.BatchNormalization()(x)
    x = layers.Dropout(0.15)(x)
    x = layers.Bidirectional(layers.GRU(32, return_sequences=True))(x)
    x = layers.Dropout(0.15)(x)
    x = AttentionLayer()(x)  # 输出 (batch, features)
    x = layers.Dense(16, activation='relu')(x)
    x = layers.Dropout(0.1)(x)
    out = layers.Dense(1, activation='linear')(x)
    model = models.Model(inputs=inp, outputs=out)
    model.compile(optimizer=optimizers.Adam(learning_rate=1e-3), loss='mse', metrics=['mae'])
    return model

def train(data_dir, metric_file=None, window_size=32, epochs=100, batch_size=32, save_dir='models'):
    os.makedirs(save_dir, exist_ok=True)
    # 选择数据文件
    if metric_file:
        metric_path = os.path.join(data_dir, metric_file)
        series = load_series_from_json(metric_path)
    else:
        series = gather_time_series(data_dir, pattern="*.json")

    # 简单去除 nan 与极端值（中位数裁剪）
    series = np.array(series, dtype=float)
    series = np.nan_to_num(series, nan=np.nanmedian(series))
    med = np.nanmedian(series)
    std = np.nanstd(series)
    series = np.clip(series, med - 6*std, med + 6*std)

    # 归一化
    scaler = MinMaxScaler(feature_range=(0, 1))
    series_norm = scaler.fit_transform(series.reshape(-1, 1)).flatten()

    X, y = create_dataset(series_norm, window_size=window_size)
    if len(X) < 50:
        raise ValueError("样本量太少，请提供更长的时间序列或减小 window_size")

    # 随机打乱
    idx = np.arange(len(X))
    np.random.shuffle(idx)
    X = X[idx]
    y = y[idx]

    # train/val split
    val_split = int(len(X) * 0.1)
    X_train, X_val = X[val_split:], X[:val_split]
    y_train, y_val = y[val_split:], y[:val_split]

    model = build_light_haydn_model(input_shape=(window_size, 1))
    model.summary()

    # 回调
    now = datetime.now().strftime('%Y%m%d_%H%M%S')
    chk_path = os.path.join(save_dir, f'haydn_model_{now}.h5')
    cb = [
        callbacks.EarlyStopping(monitor='val_loss', patience=10, restore_best_weights=True, verbose=1),
        callbacks.ReduceLROnPlateau(monitor='val_loss', factor=0.5, patience=5, min_lr=1e-6, verbose=1),
        callbacks.ModelCheckpoint(chk_path, monitor='val_loss', save_best_only=True, verbose=1)
    ]

    history = model.fit(
        X_train, y_train,
        validation_data=(X_val, y_val),
        epochs=epochs,
        batch_size=batch_size,
        callbacks=cb,
        verbose=2
    )

    # 保存模型与归一化器
    final_model_path = chk_path  # ModelCheckpoint 已保存最佳模型
    scaler_path = os.path.join(save_dir, f'haydn_scaler_{now}.save')
    joblib.dump(scaler, scaler_path)

    print("模型与归一化器已保存：", final_model_path, scaler_path)
    return final_model_path, scaler_path, history

def predict_multi_step(model_path, scaler_path, history_series, steps=10, window_size=32):
    scaler = joblib.load(scaler_path)
    model = tf.keras.models.load_model(model_path, custom_objects={'AttentionLayer': AttentionLayer})
    hist = np.array(history_series, dtype=float)
    hist = np.nan_to_num(hist, nan=np.nanmedian(hist))
    hist_norm = scaler.transform(hist.reshape(-1,1)).flatten()
    seq = list(hist_norm[-window_size:])
    preds = []
    for _ in range(steps):
        x = np.array(seq[-window_size:]).reshape(1, window_size, 1)
        p = model.predict(x, verbose=0)[0,0]
        preds.append(p)
        seq.append(p)
    preds_real = scaler.inverse_transform(np.array(preds).reshape(-1,1)).flatten().tolist()
    return preds_real

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='训练轻量 Haydn 模型')
    parser.add_argument('--data-dir', default=os.path.join(os.path.dirname(__file__), '..', 'src', 'data', 'check', 'HAYDN'),
                        help='HAYDN 数据目录（默认项目内 src/data/check/HAYDN）')
    parser.add_argument('--metric', default='cpu_utilization.json', help='使用的 JSON 文件名（可选）')
    parser.add_argument('--window', type=int, default=32, help='时间窗口大小')
    parser.add_argument('--epochs', type=int, default=60, help='最大训练轮数')
    parser.add_argument('--batch', type=int, default=32, help='批大小')
    parser.add_argument('--save-dir', default=os.path.join(os.path.dirname(__file__), 'models'), help='模型保存目录')
    args = parser.parse_args()

    model_path, scaler_path, hist = train(
        data_dir=args.data_dir,
        metric_file=args.metric,
        window_size=args.window,
        epochs=args.epochs,
        batch_size=args.batch,
        save_dir=args.save_dir
    )
    print("训练完成，模型:", model_path)