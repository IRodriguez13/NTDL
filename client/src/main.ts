import './assets/main.css'
import { library } from '@fortawesome/fontawesome-svg-core'
import { FontAwesomeIcon } from '@fortawesome/vue-fontawesome'
import { faTachometerAlt, faMicrochip, faMemory, faHdd, faDesktop } from '@fortawesome/free-solid-svg-icons';
import { createApp } from 'vue'
import { createPinia } from 'pinia'

import App from './App.vue'
import router from './router'

const app = createApp(App)


// Agregar íconos a la librería
library.add(faTachometerAlt, faMicrochip, faMemory, faHdd, faDesktop);
app.component('font-awesome-icon', FontAwesomeIcon)
app.use(createPinia())
app.use(router)

app.mount('#app')
