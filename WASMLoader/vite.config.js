import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'

// https://vite.dev/config/
export default defineConfig({
  plugins: [vue()],
  server: {
    host: true, // 允许通过局域网 IP 访问
    port: 5174, // 默认端口，或你自己指定
  },
})
