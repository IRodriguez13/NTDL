<template>
  <div class="status-view">
    <div class="status-left">
      <span class="status-dot"></span>
      <h2 class="status-text">System status:</h2><p class="status-text-paragraf">Operational</p>
    </div>
    <div class="status-right" v-if="uptime">
      <span class="uptime">Uptime: {{ uptime }}</span>
    </div>
  </div>
</template>

<script lang="ts" setup>
defineProps<{
  uptime?: string;
}>();
</script>

<style scoped>
.status-view {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 15px 30px;
  background-color: var(--layer-4);
  border-radius: 10px;
  box-shadow: 0 2px 6px rgba(0,0,0,0.1);
  margin-bottom: 15px;
}

.status-left {
  display: flex;
  align-items: center;
  gap: 10px;
}

.status-dot {
  width: 20px;
  height: 20px;
  border-radius: 50%;
  background-color: #28a745; /* verde */
  box-shadow: 0 0 8px #28a745, 0 0 14px #28a745;
}

.status-text {
  font-size: 0.6em;
  font-weight: 600;
  margin: 0;
  font-family: 'Courier New', monospace;
  color: var(--title-color);
}

.status-text-paragraf {
  font-size: 0.6em;
  margin: 0;
  font-family: 'Courier New', monospace;
  color: var(--paragraf-color);
}

.status-right{
  font-size: 0.5em;
  color: var(--paragraf-color);
}
</style>