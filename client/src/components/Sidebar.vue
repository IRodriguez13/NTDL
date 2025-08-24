<!-- Sidebar -->
<template>
    <div class="sidebar">
        <!-- Logo -->
        <div class="logo-container">
            <div class="logo-bg">
                <img src="/sysmon.png" alt="SysMon Logo" class="logo">
            </div>
        </div>

        <!-- Botones -->
        <div class="buttons-container">
            <button @click="$emit('change-view', 'cpu')" class="toggle-button btn-general low-shadow hover-smooth">
                CPU
            </button>
            <button @click="$emit('change-view', 'ram')" class="toggle-button btn-general low-shadow hover-smooth">
                RAM
            </button>
            <button @click="$emit('change-view', 'disk')" class="toggle-button btn-general low-shadow hover-smooth">
                Disk
            </button>
        </div>
    </div>
</template>

<script setup lang="ts">
</script>

<style scoped>
.sidebar {
  width: 20%;
  background-color: var(--layer-3);
  display: flex;
  flex-direction: column;
  padding: 10px;
  margin: 2vw 0;
  box-shadow: 2px 0 5px rgba(0, 0, 0, 0.1);
  border-top-right-radius: 20px;
  border-bottom-right-radius: 20px;

}

.logo-container {
  display: flex;
  justify-content: center; 
}

.logo-bg {
  background-color: var(--layer-6);
  padding: 10px; 
  margin: 20% 0 0 0;
  border-radius: 50%; 
  display: flex;
  align-items: center;
  transition: transform 0.2s ease-in-out;
  box-shadow: 0 5px 8px rgba(0, 0, 0, 0.2);
}

.logo-bg:hover {
  transform: scale(1.1);
  cursor: pointer;
}

.logo {
  width: 10vw;  
  height: auto;
  object-fit: contain;
  border-radius: 50%;
}

.buttons-container {
  display: flex;
  flex-direction: column;
  justify-content: center;
  flex: 1;
  gap: 10px; 
}
</style>
