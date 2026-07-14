<template>
  <div v-if="isVisible" class="fixed inset-0 flex items-center justify-center bg-black bg-opacity-50 z-50">
    <div class="bg-gradient-to-r from-blue-500 to-blue-700 p-6 rounded-xl shadow-lg max-w-sm w-full">
      <div class="text-center mb-4">
        <h3 class="text-2xl font-semibold text-white">{{ title }}</h3>
        <p class="text-lg text-white">{{ message }}</p>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref } from 'vue';

const isVisible = ref(false);
const title = ref('');
const message = ref('');

// 打开弹窗
const openModal = (newTitle, newMessage) => {
  title.value = newTitle;
  message.value = newMessage;
  isVisible.value = true;

  // 自动关闭弹窗
  setTimeout(() => {
    closeModal();
  }, 1500); // 1.5秒后关闭
};

// 关闭弹窗
const closeModal = () => {
  isVisible.value = false;
};

// 导出函数供父组件使用
defineExpose({
  openModal,
});
</script>

<style scoped>
/* 修改背景为渐变色 */
.bg-gradient-to-r {
  background: linear-gradient(135deg, #3b82f6, #1e40af);
}

/* 调整字体和弹窗的样式 */
h3 {
  font-size: 1.25rem;
  font-weight: 600;
}

p {
  font-size: 1rem;
}

/* 设计圆角、阴影和过渡效果 */
.bg-gradient-to-r {
  box-shadow: 0 4px 10px rgba(0, 0, 0, 0.2);
  transition: all 0.3s ease-in-out;
}

.bg-gradient-to-r:hover {
  transform: scale(1.05);
}
</style>
