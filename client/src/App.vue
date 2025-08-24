<template>
  <div class="app">
    <Sidebar @change-view="currentView = $event" />
    <div class="main-content">
          <component :is="currentViewComponent" class="main-view" />
    </div>
  </div>
</template>

<script lang="ts" setup>

import { ref, computed } from 'vue';
import Sidebar from './components/Sidebar.vue';
import CpuView from './views/CpuView.vue';
import RamView from './views/RamView.vue';
import DiskView from './views/DiskView.vue';

const currentView = ref('cpu');

const currentViewComponent = computed(()=>{
  switch (currentView.value) {
    case 'cpu':
      return CpuView;
    case 'ram':
      return RamView;
    case 'disk':
      return DiskView;
    default: return CpuView;
  }
})
</script>

<style scoped>
.app {
  display: flex;
  height: 100vh;
  max-width: 100vw;
  background-color: var(--layer-2);

}

.main-content {
  flex: 1;
  color: white;
  overflow: auto;
  padding: 2vw;
}
</style>