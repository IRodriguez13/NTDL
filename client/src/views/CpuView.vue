<!-- Vista de Control del cpu -->
<template>
    <div class="view">
        <div class="container-general">
            <InfoView title="CPU Monitor" subtitle="Procesador funcionando correctamente" />
            <StatusView uptime="72 hours" />
        </div>
    </div>
</template>

<script lang="ts" setup>
import InfoView from '../components/headers/InfoViews.vue';
import StatusView from '../components/headers/StatusView.vue';
</script>

<style scoped>

</style>
