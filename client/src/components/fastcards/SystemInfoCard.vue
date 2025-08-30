<template>
  <div class="system-card">
    <div class="system-left">
      <img v-if="icon" :src="icon" alt="System Icon" class="system-icon" />
      <i v-else class="fas fa-desktop system-icon"></i>
    </div>
    <div class="system-right">
      <div v-for="(value, key) in info" :key="key" class="info-row">
        <span class="info-key">{{ formatKey(key) }}:</span>
        <span class="info-value">{{ value }}</span>
      </div>
    </div>
  </div>
</template>

<script lang="ts" setup>
defineProps<{
  icon?: string;               
  info: Record<string, string>;
}>();

function formatKey(key: string) {
  return key.replace(/_/g, ' ').replace(/\b\w/g, c => c.toUpperCase());
}
</script>

<style scoped>
.system-card {
  display: flex;
  background-color: var(--layer-4);
  justify-items: center;
  align-items: center;
  border-radius: 10px;
  gap: 15px;
  min-height: 420px;
}

.system-right {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 4px;
  margin: 20px 0;
}

/* Imagen o icono */
.system-left {
  width: 20%;
  height: 420px;
  min-height: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  overflow: hidden;
  border-radius: 10px;
}

.system-icon {
  width: auto;
  height: 100%;
  object-fit: cover;
}

/* ////////////////////////////////////////// */
.info-row {
  display: flex;
  gap: 10px;
  font-family: 'Courier New', monospace;
  font-size: 0.95rem;
}

.info-key {
  font-weight: 600;
  color: var(--text-blue);
  min-width: 120px;
}

.info-value {
  color: var(--paragraf-color);
  flex: 1;
}
</style>