import numpy as np
from sklearn.preprocessing import MinMaxScaler
from keras.models import Sequential
from keras.layers import LSTM, Dense
from tcn import TCN

# 加载历史数据
cpu_list = [0.5, 0.6, 0.7, 0.8, 0.65, 0.7, 0.75, 0.8, 0.85, 0.9, 0.88, 0.86, 0.84, 0.82, 0.8, 0.78, 0.76, 0.74, 0.72, 0.7,
            0.68, 0.66, 0.64, 0.62, 0.6, 0.58, 0.56, 0.54, 0.52, 0.5, 0.48, 0.46, 0.44, 0.42, 0.4, 0.38, 0.36, 0.34, 0.32, 0.3]

# 数据归一化
scaler = MinMaxScaler(feature_range=(0, 1))
cpu_arr = np.array(cpu_list).reshape(-1, 1)
cpu_norm = scaler.fit_transform(cpu_arr).flatten()

# 构造时序数据集
def create_dataset(series, window_size=20):
    X, y = [], []
    for i in range(len(series) - window_size):
        X.append(series[i:i+window_size])
        y.append(series[i+window_size])
    X = np.array(X).reshape(-1, window_size, 1)
    y = np.array(y)
    return X, y

window_size = 20
X, y = create_dataset(cpu_norm, window_size)

# 训练 RNN (LSTM) 模型
rnn_model = Sequential([
    LSTM(32, input_shape=(window_size, 1)),
    Dense(1)
])
rnn_model.compile(optimizer='adam', loss='mse')
rnn_model.fit(X, y, epochs=30, batch_size=8, validation_split=0.1)
rnn_model.save('rnn_model.h5')
print('RNN 模型已保存为 rnn_model.h5')

# 训练 TCN 模型
# pip install keras-tcn
# from tcn import TCN

tcn_model = Sequential([
    TCN(input_shape=(window_size, 1)),
    Dense(1)
])
tcn_model.compile(optimizer='adam', loss='mse')
tcn_model.fit(X, y, epochs=30, batch_size=8, validation_split=0.1)
tcn_model.save('tcn_model.h5')
print('TCN 模型已保存为 tcn_model.h5')

# 保存归一化器
import joblib
joblib.dump(scaler, 'cpu_scaler.save')
print('归一化器已保存为 cpu_scaler.save')

# 推理示例（预测未来10步并反归一化）
from keras.models import load_model

def multi_step_predict(model_path, history, steps, scaler, window_size=20):
    model = load_model(model_path)
    input_seq = np.array(history[-window_size:]).reshape((1, window_size, 1))
    preds = []
    for _ in range(steps):
        pred = model.predict(input_seq)[0,0]
        preds.append(pred)
        input_seq = np.roll(input_seq, -1)
        input_seq[0, -1, 0] = pred
    # 反归一化
    preds_real = scaler.inverse_transform(np.array(preds).reshape(-1,1)).flatten()
    return preds_real.tolist()

# 示例：用归一化历史数据预测未来10步
rnn_preds = multi_step_predict('rnn_model.h5', cpu_norm, 10, scaler, window_size)
tcn_preds = multi_step_predict('tcn_model.h5', cpu_norm, 10, scaler, window_size)
print('RNN 未来10步预测:', rnn_preds)
print('TCN 未来10步预测:', tcn_preds)
