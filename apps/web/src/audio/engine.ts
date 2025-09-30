/* eslint-disable no-console */
export type AudioWorkletModuleId = keyof typeof WORKLET_MANIFEST;

const WORKLET_MANIFEST = {
  eq: './worklets/eq-processor.ts',
} as const satisfies Record<string, string>;

const WORKLET_URLS: Record<AudioWorkletModuleId, string> = Object.fromEntries(
  Object.entries(WORKLET_MANIFEST).map(([moduleId, relativePath]) => {
    const normalizedPath = relativePath.endsWith('?worker&url')
      ? relativePath
      : `${relativePath}?worker&url`;

    return [moduleId, new URL(normalizedPath, import.meta.url).href];
  }),
) as Record<AudioWorkletModuleId, string>;

export class AudioEngine {
  constructor(private readonly context: AudioContext) {
    if (!('audioWorklet' in context)) {
      throw new Error('AudioWorklet is not available in this AudioContext.');
    }
  }

  async addModule(moduleId: AudioWorkletModuleId): Promise<void> {
    const moduleUrl = WORKLET_URLS[moduleId];

    if (!moduleUrl) {
      throw new Error(`AudioWorklet module "${moduleId}" is not registered in the manifest.`);
    }

    await this.context.audioWorklet.addModule(moduleUrl);
  }

  async addAllModules(): Promise<void> {
    await Promise.all(
      (Object.keys(WORKLET_URLS) as AudioWorkletModuleId[]).map((moduleId) => this.addModule(moduleId)),
    );
  }
}

export const AUDIO_WORKLET_URLS = WORKLET_URLS;
