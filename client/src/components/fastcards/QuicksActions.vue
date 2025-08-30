<template>
  <div class="quick-actions-view">
    <h2 class="quick-actions-title">Quick Actions</h2>

    <div class="quick-actions-buttons">
      <button class="btn-general" v-for="(action, index) in actions" :key="index" @click="action.handler">
        {{ action.label }}
      </button>
    </div>
  </div>
</template>

<script lang="ts" setup>
import { defineProps, ref } from 'vue';

type Action = {
  label: string;
  handler: () => void;
};

defineProps<{
  actions?: Action[];
}>();

// Ejemplo de acciones por defecto (se puede pasar desde el padre)
const actions = ref([
  { label: 'Acción 1', handler: () => console.log('Acción 1 ejecutada') },
  { label: 'Acción 2', handler: () => console.log('Acción 2 ejecutada') },
  { label: 'Acción 3', handler: () => console.log('Acción 3 ejecutada') },
]);
</script>

<style scoped>
.quick-actions-view {
  background-color: var(--layer-4);
  padding: 20px;
  border-radius: 10px;
  box-shadow: 0 2px 6px rgba(0,0,0,0.1);
  margin-top: 15px;
}

.quick-actions-title {
  font-size: 0.8em;
  font-weight: 600;
  margin-bottom: 15px;
  font-family: 'Courier New', monospace;
  color: var(--title-color);
}

.quick-actions-buttons {
  display: flex;
  gap: 10px;
  flex-wrap: wrap;
}

</style>