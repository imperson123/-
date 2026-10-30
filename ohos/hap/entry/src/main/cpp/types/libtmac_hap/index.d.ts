/**
 * LUT-SA T-MAC native module (libtmac_hap.so) — LLM inference + LUT kernel self-test.
 *
 * Sync calls block the JS thread; use the *Async variants from the UI (model load and
 * generation take seconds-to-minutes on device).
 */
export const nativeVersion: () => string;

export const selfTest: () => string;
export const bench: (steps: number) => string;

export const loadModel: (
  modelPath: string,
  filesDir: string,
  threads?: number,
  nCtx?: number,
  useMmap?: boolean
) => string;
export const loadModelAsync: (
  modelPath: string,
  filesDir: string,
  threads?: number,
  nCtx?: number,
  useMmap?: boolean
) => Promise<string>;

export const generate: (prompt: string, nPredict?: number, temp?: number, topK?: number) => string;
export const generateAsync: (
  prompt: string,
  nPredict?: number,
  temp?: number,
  topK?: number
) => Promise<string>;

/**
 * Streaming generation: onToken(piece) fires on the JS thread for every generated token
 * piece (typewriter effect); the Promise then resolves with the full console-style report.
 */
export const generateStreamAsync: (
  prompt: string,
  nPredict: number,
  temp: number,
  topK: number,
  onToken: (piece: string) => void
) => Promise<string>;

export const release: () => string;

/** chmod the sandbox (so `hdc file send` can push a model into it) and report the exact path. */
export const prepareSandbox: (filesDir: string) => string;

/** Copy between two open file descriptors on a worker thread (never blocks the UI). */
export const copyFdAsync: (srcFd: number, dstFd: number) => Promise<string>;
