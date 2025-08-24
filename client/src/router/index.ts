import { createRouter, createWebHistory } from 'vue-router'
import CpuView from '../views/CpuView.vue'
import RamView from '../views/RamView.vue'
import DiskView from '../views/DiskView.vue'
import DashboardView from '../views/DashboardView.vue'

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/dashboard',
      name: 'dashboard',
      component: DashboardView,
    },
    {
      path: '/cpu',
      name: 'cpu',
      component: CpuView,
    },
    {
      path: '/ram',
      name: 'ram',
      component: RamView,
    },
    {
      path: '/disk',
      name: 'disk',
      component: DiskView,
    },
  ],
})

export default router
