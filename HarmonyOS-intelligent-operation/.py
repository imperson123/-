from keras.models import load_model
import numpy as np

def rnn_multi_step_predict(history, steps, model_path, scaler):
    model = load_model(model_path)
    input_seq = np.array(history[-20:]).reshape((1, 20, 1))
    preds = []
    for _ in range(steps):
        pred = model.predict(input_seq)[0,0]
        preds.append(pred)
        input_seq = np.roll(input_seq, -1)
        input_seq[0, -1, 0] = pred
    # 反归一化
    preds_real = inverse_transform(preds, scaler)
    return preds_real.tolist()