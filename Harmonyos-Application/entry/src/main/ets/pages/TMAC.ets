import hilog from '@ohos.hilog';
import { BusinessError } from '@ohos.base';

// 加载C++共享库
const TMAC_LIB = 'libtmac_harmony.so';
@External('c', TMAC_LIB) declare function tmac_qgemm_lut(
    weight: Uint8Array,
    input: Float32Array,
    output: Float32Array,
    M: number,
    N: number,
    K: number
): number;

export class TMAC {
    private weight: Uint8Array;
    private M: number;
    private N: number;
    private K: number;

    constructor(weightData: Uint8Array, matrixShape: [number, number, number]) {
        this.weight = weightData;
        [this.M, this.N, this.K] = matrixShape;
        hilog.info(0x0000, 'TMAC', 'Initialized with shape: [%d, %d, %d]', this.M, this.N, this.K);
    }

    // 执行矩阵乘法
    public run(input: Float32Array): Float32Array {
        if (input.length !== this.M * this.K) {
            throw new Error(`Input shape mismatch: expected ${this.M*this.K} elements`);
        }

        const output = new Float32Array(this.M * this.N);
        const ret = tmac_qgemm_lut(this.weight, input, output, this.M, this.N, this.K);
        
        if (ret !== 0) {
            throw new Error(`TMAC execution failed with code: ${ret}`);
        }
        return output;
    }
}