import { defineConfig } from "vite";
import vue from "@vitejs/plugin-vue";
import { visualizer } from "rollup-plugin-visualizer"; 
import Components from 'unplugin-vue-components/vite'
import AutoImport from 'unplugin-auto-import/vite'
import { ElementPlusResolver } from 'unplugin-vue-components/resolvers'


// https://vite.dev/config/
export default defineConfig({
  plugins: [
    vue(),
    visualizer({
      open: true, // 构建后自动打开浏览器展示分析报告
      gzipSize: true,
      brotliSize: true,
    }),
    AutoImport({
      resolvers: [ElementPlusResolver()],
    }),
    Components({
      resolvers: [ElementPlusResolver()],
    }),
  ],
  server: {
    host: true, // 允许通过局域网 IP 访问
    port: 5174, // 默认端口，或你自己指定
  },
  build: {
    rollupOptions: {
      output: {
        manualChunks: {
          elementPlus: ['element-plus'],
        },
      },
    },
  },
});
