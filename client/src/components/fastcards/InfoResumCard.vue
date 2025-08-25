<template>
  <div class="info-card">
    <div class="card-header">
      <div class="card-icon">
        <img v-if="icon" :src="icon" alt="icon" />
        <i v-else :class="['fas', faIcon]"></i>
      </div>
      <span class="card-title">{{ title }}</span>
      <span class="card-status" :class="status">{{ statusText }}</span>
    </div>
    <div class="card-value">{{ value }}</div>
    <div class="card-subtitle" v-if="subtitle">{{ subtitle }}</div>
    <div class="usage-bar">
      <div class="usage-fill" :style="{ width: usage + '%', backgroundColor: statusColor }"></div>
    </div>
    <div class="usage-percent">{{ usage }}%</div>
  </div>
</template>

<script lang="ts" setup>
import { computed } from 'vue';

const props = defineProps<{
  title: string;
  value: string;
  subtitle?: string;
  usage: number;
  status?: 'healthy' | 'warning' | 'critical';
  icon?: string;
  faIcon?: string;
}>();

const statusColor = computed(() => {
  switch (props.status) {
    case 'warning': return '#FFC107';
    case 'critical': return '#DC3545';
    default: return '#28A745';
  }
});

const statusText = computed(() => {
  switch (props.status) {
    case 'warning': return 'Warning';
    case 'critical': return 'Critical';
    default: return 'Healthy';
  }
});
</script>

<style scoped>
.info-card {
  background-color: var(--layer-4);
  border-radius: 12px;
  width: 100%;
  padding: 10px;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  box-shadow: 0 4px 12px rgba(0,0,0,0.15);
  gap: 5px;
}

.card-header {
  display: flex;
  align-items: center;
  gap: 8px;
}

.card-icon img,
.card-icon i {
  width: 24px;
  height: 24px;
  color: var(--text-blue);
}

.card-title {
  font-size: 1rem;
  font-weight: 500;
  flex: 1;
}

.card-status {
  font-size: 0.75rem;
  padding: 2px 6px;
  border-radius: 6px;
  color: white;
  text-transform: uppercase;
}

.card-status.healthy { background-color: #28A745; }
.card-status.warning { background-color: #FFC107; }
.card-status.critical { background-color: #DC3545; }

.card-value {
  font-size: 1.5rem;
  font-weight: 700;
  margin-top: 10px;
}

.card-subtitle {
  font-size: 0.75rem;
  color: var(--paragraf-color);
  margin-top: 5px;
}

.usage-bar {
  width: 100%;
  height: 6px;
  background-color: #555;
  border-radius: 3px;
  margin-top: 10px;
  overflow: hidden;
}

.usage-fill {
  height: 100%;
  border-radius: 3px;
}

.usage-percent {
  font-size: 0.7rem;
  color: var(--paragraf-color);
  text-align: right;
  margin-top: 3px;
}
</style>