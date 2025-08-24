<template>
  <div class="info-view">
    <div class="info-left">
      <h3 class="title-view">{{ title }}</h3>
      <p class="subtitle-view" v-if="subtitle">{{ subtitle }}</p>
    </div>
    <div class="info-right" v-if="uptime">
      <span class="uptime">Uptime: {{ uptime }}</span>
    </div>
  </div>
</template>

<script lang="ts" setup>
defineProps<{
  title: string;
  subtitle?: string;
  uptime?: string; // tiempo prendido opcional
}>();
</script>

<style scoped>
.info-view {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 20px;
  background-color: var(--layer-4);
  border-radius: 10px;
  box-shadow: 0 2px 6px rgba(0,0,0,0.1);
  margin-bottom: 15px;
}

.info-left {
  display: flex;
  flex-direction: column;
}

.title-view {
  font-size: 1.2em;
  font-weight: 600;
  margin: 0;
  font-family: 'Courier New', Consolas, 'Lucida Console', Monaco, monospace;

}

.subtitle-view {
  font-size: 0.5em;
  color: var(--paragraf-color);
  opacity: 0.8;
  font-family: 'Courier New', Consolas, 'Lucida Console', Monaco, monospace;

}

.info-right {
  display: flex;
  align-items: flex-start;
}

.uptime {
  font-size: 0.9em;
  margin:0 20px;
  color: var(--layer-7);
}
</style>
