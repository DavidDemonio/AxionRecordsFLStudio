import { defineConfig } from 'vite';

export default defineConfig({
  worker: {
    format: 'es',
    rollupOptions: {
      output: {
        entryFileNames: (chunkInfo) => {
          if (chunkInfo.name?.includes('worklets/')) {
            return 'assets/[name].js';
          }

          return 'assets/[name]-[hash].js';
        },
      },
    },
  },
});
