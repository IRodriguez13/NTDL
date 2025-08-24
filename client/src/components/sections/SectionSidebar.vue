<template>
  <div class="sidebar-section">
    <h3 class="section-title">{{ title }}</h3>
    <div class="section-buttons">
      <button
        v-for="(button, index) in buttons"
        :key="index"
        @click="$emit('button-click', button.action)"
        :class="['toggle-button', 'btn-sidebar', { active: button.action === activeView }]"
      >
        <font-awesome-icon
          v-if="button.icon"
          :icon="button.icon"
          class="button-icon"
        />
        {{ button.label }}
      </button>
    </div>
  </div>
</template>

<script setup lang="ts">
defineProps<{
  title: string;
  buttons: { label: string; action: string; icon?: string }[];
  activeView: string;
}>();
</script>

<style scoped>
.sidebar-section {
  display: flex;
  flex-direction: column;
  justify-content: center;
  margin-bottom: 20px;
  margin: 0  0  0 1rem;
}

.section-title {
  font-size: 1.1rem;
  font-weight: bold;
  text-align: start;
  color: var(--text-blue);
  font-family: 'Courier New', monospace;
}

.section-buttons {
  display: flex;
  flex-direction: column;
  justify-content: center;
}

.button-icon {
  width: 20px;
  height: 20px;
  margin: 0 15px;
}
</style>
