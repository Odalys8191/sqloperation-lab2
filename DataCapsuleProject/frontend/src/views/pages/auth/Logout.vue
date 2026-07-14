<script setup>
import { ref, onMounted } from 'vue';
import { useRouter } from 'vue-router'; // 引入 Vue Router
import Modal from '@/components/Modal.vue'; // 引入 Modal 组件

const router = useRouter(); // 获取路由实例
const currentUser = ref(null);

let modalRef = null; // 用于引用 Modal 组件

// 检查本地存储的用户登录信息
const checkCurrentUser = () => {
    const storedUser = window.localStorage.getItem('user_id');
    if (storedUser) {
        currentUser.value = storedUser;
    } else {
        currentUser.value = null;
    }
};

// 退出登录函数
const logout = () => {
    if (!currentUser.value) {
        modalRef.openModal('提示', '当前没有用户登录，请先登录！');
        return;
    }

    window.localStorage.clear();
    modalRef.openModal('提示', '已退出登录');

    // 在弹窗打开后自动跳转到登录页面
    setTimeout(() => {
        router.push('/');
    }, 1500); // 延迟 1.5 秒，给用户一些时间看到弹窗
};

// 页面加载时检查用户登录状态
onMounted(() => {
    checkCurrentUser();
});
</script>

<template>
    <Modal ref="modalRef" />
    <div
        class="bg-surface-50 dark:bg-surface-950 flex items-center justify-center min-h-screen min-w-[100vw] overflow-hidden">
        <div class="flex flex-col items-center justify-center">
            <div
                style="border-radius: 56px; padding: 0.3rem; background: linear-gradient(180deg, var(--primary-color) 10%, rgba(33, 150, 243, 0) 30%)">
                <div class="w-full bg-surface-0 dark:bg-surface-900 py-20 px-8 sm:px-20" style="border-radius: 53px">
                    <div class="text-center mb-8">
                        <h2 class="text-3xl font-bold mb-6">退出登录</h2>

                        <!-- 根据是否有用户显示不同信息 -->
                        <p v-if="currentUser" class="mb-8 text-lg">当前登录用户：{{ currentUser }}</p>
                        <p v-else class="mb-8 text-lg">当前没有用户登录，请先登录。</p>
                    </div>

                    <!-- 仅在有用户登录时显示退出登录按钮 -->
                    <div class="flex justify-center">
                        <button v-if="currentUser" @click="logout"
                            class="py-2 px-6 border border-transparent shadow-sm text-sm font-medium rounded-md text-white bg-red-500 hover:bg-red-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-red-500">
                            退出登录
                        </button>
                        
                        <!-- 在没有用户时显示返回登录按钮 -->
                        <button v-else @click="() => router.push('/')"
                            class="py-2 px-6 border border-transparent shadow-sm text-sm font-medium rounded-md text-white bg-blue-500 hover:bg-blue-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500">
                            返回
                        </button>
                    </div>
                </div>
            </div>
        </div>
    </div>
</template>
