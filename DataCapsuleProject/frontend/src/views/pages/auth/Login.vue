<script setup>
import FloatingConfigurator from '@/components/FloatingConfigurator.vue';
import TopbarWidget from '@/components/landing/TopbarWidget.vue';
import Modal from '@/components/Modal.vue';
import { ref, onMounted } from 'vue';

const user_id = ref('');
const password = ref('');
const isLoggedIn = ref(false);
const currentUser = ref(null);

let modalRef = null;

const checkLoginStatus = () => {
    const storedUser = window.localStorage.getItem('user_id');
    const storedToken = window.localStorage.getItem('token');
    const loginTime = window.localStorage.getItem('login_time');
    const sessionDuration = 24 * 60 * 60 * 1000;

    if (storedUser && storedToken && loginTime) {
        const now = Date.now();
        if (now - parseInt(loginTime, 10) < sessionDuration) {
            isLoggedIn.value = true;
            currentUser.value = storedUser;
        } else {
            window.localStorage.clear();
        }
    } else {
        window.localStorage.clear();
    }
};

const login_send = () => {
    if (isLoggedIn.value) {
        modalRef.openModal('提示', '当前设备已有用户登录，请先退出登录再登录。');
        return;
    }

    const formData = {
        user_id: user_id.value,
        password: password.value,
    };

    fetch('/api/user/login', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify(formData),
    })
        .then((response) => {
            if (response.ok) {
                return response.json();
            } else {
                throw new Error('登录失败');
            }
        })
        .then((data) => {
            const loginTime = Date.now();
            window.localStorage.setItem('token', data.access);
            window.localStorage.setItem('refresh', data.refresh);
            window.localStorage.setItem('user_id', user_id.value);
            window.localStorage.setItem('password', password.value);
            window.localStorage.setItem('login_time', loginTime.toString());

            console.log('登录成功:', data);
            console.log('token:', data.access);
            console.log('refresh:', data.refresh);
            console.log('user_id:', user_id.value);

            modalRef.openModal('成功', '登录成功！');
            setTimeout(() => {
                window.location.href = '/';
            }, 1000);
        })
        .catch((error) => {
            console.error('登录失败:', error);
            modalRef.openModal('错误', '登录失败，请检查用户名和密码是否正确。');
        });
};

const logout = () => {
    window.localStorage.clear();
    isLoggedIn.value = false;
    currentUser.value = null;
    modalRef.openModal('提示', '已退出登录');
};

onMounted(() => {
    checkLoginStatus();
});
</script>

<template>
    <div class="bg-surface-0 dark:bg-surface-900">
        <div id="home" class="landing-wrapper overflow-hidden">
            <div class="py-6 px-6 mx-0 md:mx-12 lg:mx-20 lg:px-20 flex items-center justify-between relative lg:static">
                <TopbarWidget />
            </div>
        </div>
    </div>
    <Modal ref="modalRef" />
    <div
        class="bg-surface-50 dark:bg-surface-950 flex items-center justify-center min-h-screen min-w-[100vw] overflow-hidden">
        <div class="flex flex-col items-center justify-center">
            <div
                style="border-radius: 56px; padding: 0.3rem; background: linear-gradient(180deg, var(--primary-color) 10%, rgba(33, 150, 243, 0) 30%)">
                <div class="w-full bg-surface-0 dark:bg-surface-900 py-20 px-8 sm:px-20" style="border-radius: 53px">
                    <div class="text-center mb-8">
                        <h2 class="text-3xl font-bold">登录</h2>
                    </div>

                    <!-- 显示登录状态 -->
                    <div v-if="isLoggedIn" class="text-center mb-8">
                        <p>当前登录用户：{{ currentUser }}</p>
                        <button @click="logout"
                            class="inline-flex justify-center py-2 px-4 border border-transparent shadow-sm text-sm font-medium rounded-md text-white bg-red-500 hover:bg-red-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-red-500">
                            退出登录
                        </button>
                    </div>

                    <!-- 登录表单 -->
                    <form v-else @submit.prevent="login_send">
                        <div class="mb-6">
                            <label for="user_id" class="block text-sm font-medium">用户 ID</label>
                            <input id="user_id" v-model="user_id" type="text" placeholder="请输入用户 ID"
                                class="mt-1 block w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:ring-primary focus:border-primary sm:text-sm">
                        </div>

                        <div class="mb-6">
                            <label for="password" class="block text-sm font-medium">密码</label>
                            <input id="password" v-model="password" type="password" placeholder="请输入密码"
                                class="mt-1 block w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:ring-primary focus:border-primary sm:text-sm">
                        </div>

                        <div class="flex items-center justify-between">
                            <button type="submit"
                                class="inline-flex justify-center py-2 px-4 border border-transparent shadow-sm text-sm font-medium rounded-md text-white bg-primary hover:bg-primary-dark focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-primary">
                                登录
                            </button>
                        </div>
                    </form>
                </div>
            </div>
        </div>
    </div>
</template>
