<template>
  <div class="sidebar">
    <!-- Seccion de logo -->
    <div class="head-logo">
      <div class="logo-container">
        <div class="logo-bg">
          <!-- <img src="/sysmon.png" alt="SysMon Logo" class="logo" /> -->
           <font-awesome-icon :icon="['fas', 'desktop']" class="logo" />
        </div>
        <div class="logo-text">
          <h2 class="title">Sys_Monitor</h2>
          <p class="paragraf">v1.0</p>
        </div>
      </div>
    </div>

    <div class="separator"></div>

    <!-- Secciones de botonera -->
    <SidebarSection
      title="SYSTEM OVERVIEW"
      :buttons="[
        { label: 'Dashboard', action: 'dashboard', icon: 'tachometer-alt' },
        { label: 'CPU Monitor', action: 'cpu', icon: 'microchip' },
        { label: 'Memory', action: 'ram', icon: 'memory' },
        { label: 'Storage', action: 'disk', icon: 'hdd' }
      ]"
      :active-view="activeView"
      @button-click="$emit('change-view', $event)"
    />
  </div>
</template>

<script setup lang="ts">
import SidebarSection from './sections/SectionSidebar.vue';
defineProps<{ activeView: string }>();
</script>

<style scoped>
.head-logo {
  display: flex;
  justify-content: center;
  align-items: center;
}

.sidebar {
  position: relative; 
  width: 20rem;
  background-color: var(--layer-3);
  display: flex;
  flex-direction: column;
  box-shadow: 2px 0 5px rgba(0, 0, 0, 0.1);
}

.sidebar::before {
  content: '';
  position: absolute;
  top: 0;
  right: 0; 
  width: 2px; 
  height: 100%;
  background-color: var(--layer-6);
  border-radius: 1px;
}


.buttons-container {
  display: flex;
  flex-direction: column;
  justify-content: center;
  flex: 1;
  gap: 10px; 
}

.logo-container {
  display: flex;
  align-items: center;
  justify-content: flex-start;
  gap: 10px; /* espacio entre logo y texto */
  padding: 10px 0 0 0;
  margin-top: 10px;
}

.logo-bg {
  background-color: var(--layer-6);
  border-radius: 12px;
  box-shadow: 0 5px 8px rgba(0, 0, 0, 0.2);
  transition: transform 0.2s ease-in-out;
}

.logo-bg:hover {
  transform: scale(1.1);
  cursor: pointer;
}

.logo {
  width: 30px;
  padding: 0 10px;
  object-fit: contain;
  color: var(--text-blue);
}

.logo-text {
  display: flex;
  flex-direction: column;
  justify-content: center;
}
</style>
