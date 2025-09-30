/**
 * Simple equalizer AudioWorklet processor placeholder.
 *
 * This pass-through processor exists so the Vite build emits a
 * bundled worklet file that can be loaded by the audio engine.
 */
class EqProcessor extends AudioWorkletProcessor {
  process(inputs: Float32Array[][], outputs: Float32Array[][]): boolean {
    const inputChannelData = inputs[0];
    const outputChannelData = outputs[0];

    if (!inputChannelData || !outputChannelData) {
      return true;
    }

    for (let channel = 0; channel < inputChannelData.length; channel += 1) {
      const input = inputChannelData[channel];
      const output = outputChannelData[channel] ?? new Float32Array(input.length);

      if (outputChannelData[channel] !== output) {
        outputChannelData[channel] = output;
      }

      output.set(input);
    }

    return true;
  }
}

registerProcessor('eq-processor', EqProcessor);
export {};
