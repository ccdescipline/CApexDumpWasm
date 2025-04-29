import { createApp } from 'vue'
import './style.css'
import App from './App.vue'

// 引入 Element Plus 和样式
import ElementPlus from 'element-plus'
import 'element-plus/dist/index.css'

createApp(App)
.use(ElementPlus)
.mount('#app')
