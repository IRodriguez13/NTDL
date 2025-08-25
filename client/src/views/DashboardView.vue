<template>
  <div class="view">
    <div class="container-general">
      <InfoView 
      title="CPU Monitor" 
      subtitle="Procesador funcionando correctamente" 
      />
      <StatusView 
        uptime="72 hours"
      />
    </div>
    <div class="container-info">
      <SystemInfoCard
        icon="/linux.jpeg"
        :info="{
          os: 'Debian GNU/Linux 13 (trixie) x86_64',
          host: '81WA (IdeaPad 3 14IML05)',
          kernel: 'Linux 6.12.41+deb13-amd64',
          uptime: '2 hours, 51 mins',
          packages: '1742 (dpkg)',
          display: '1920x1080 @ 60 Hz',
          cpu: 'Intel(R) Core(TM) i5-10210U @ 4.20 GHz',
          gpu: 'Intel UHD Graphics',
          memory: '5.12 GiB / 7.51 GiB (68%)',
          swap: '560.54 MiB / 3.03 GiB (18%)',
          disk: '13.29 GiB / 53.45 GiB (25%) - ext4',
          local_ip: '192.168.0.6/24',
          battery: '86% [Charging]',
          locale: 'es_AR.UTF-8'
        }"
      />
      <div class="container-cards-sys">
          <InfoResumCard 
          title="CPU Usage" 
          value="45%" 
          :usage="45" 
          status="healthy" 
          faIcon="fa-microchip"
        />

        <InfoResumCard 
          title="RAM Usage" 
          value="70%" 
          :usage="70" 
          status="warning" 
          faIcon="fa-memory"
          subtitle="High usage"
        />

        <InfoResumCard 
          title="Disk Space" 
          value="90%" 
          :usage="90" 
          status="critical" 
        />
      </div>
    </div>
  </div>
</template>

<script lang="ts" setup>
import InfoView from '../components/headers/InfoViews.vue';
import StatusView from '../components/headers/StatusView.vue';
import SystemInfoCard from '../components/fastcards/SystemInfoCard.vue';
import InfoResumCard from '@/components/fastcards/InfoResumCard.vue';
</script>

<style scoped>
.container-general, .container-info {
  display: flex;
  gap: 10px;
}

.container-general > *, .container-info > * {
  flex: 1; 
}

.container-cards-sys {
  display: flex;
  flex-direction: column;
  gap: 10px;
  flex: 1;
}
</style>