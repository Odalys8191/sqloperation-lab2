<script setup>
import FloatingConfigurator from '@/components/FloatingConfigurator.vue';
import TopbarWidget from '@/components/landing/TopbarWidget.vue';
import { ref } from 'vue';
import { useRouter } from 'vue-router';

const router = useRouter();

// 前端字段
const username = ref('');
const password = ref('');
const confirmPassword = ref('');
const passwordVisible = ref(false);
const confirmPasswordVisible = ref(false);
const user_id = ref(''); // 用于存储注册后返回的 user_id
const loading = ref(false);
const successMessage = ref(''); // 用于显示成功信息
const errorMessage = ref(''); // 用于显示失败信息
const showModal = ref(false); // 控制模态框显示与隐藏
const countdown = ref(5); // 注册成功后倒计时跳转
const keyFileName = ref('');
const keyFileBase64 = ref('');
let timer = null;

const register_send = async () => {
    // 清空消息
    successMessage.value = '';
    errorMessage.value = '';
    keyFileName.value = '';
    keyFileBase64.value = '';


    // 基础校验
    if (!username.value || !password.value || !confirmPassword.value) {
        errorMessage.value = '请填写完整的注册信息';
        return;
    }
    if (username.value.length < 3) {
        errorMessage.value = '用户名长度不能少于3位';
        return;
    }
    if (password.value.length < 6) {
        errorMessage.value = '密码长度不能少于6位';
        return;
    }
    if (!/^(?=.*[A-Za-z])(?=.*\d)[A-Za-z\d!@#$%^&*()_+]{6,}$/.test(password.value)) {
        errorMessage.value = '密码需包含字母和数字，且不少于6位';
        return;
    }
    if (password.value !== confirmPassword.value) {
        errorMessage.value = '两次输入的密码不一致';
        return;
    }

    const formData = {
        username: username.value,
        password: password.value,
    };

    loading.value = true;

    try {
        const response = await fetch('/api/user/register', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(formData),
        });

        if (!response.ok) {
            const error = await response.json();
            throw new Error(error.error || '注册失败');
        }

        const data = await response.json();

        // 提取后端返回的 user_id
        user_id.value = data.user_id;
        keyFileName.value = data.abe_private_key_file_name || '';
        keyFileBase64.value = data.abe_private_key_base64 || '';
        successMessage.value = `注册成功！请妥善保存您的用户 ID：${data.user_id}`;

        // 显示成功消息的模态框
        showModal.value = true;
        countdown.value = 5;
        timer && clearInterval(timer);
        timer = setInterval(() => {
            countdown.value--;
            if (countdown.value <= 0) {
                clearInterval(timer);
                navigateToLogin();
            }
        }, 1000);
    } catch (error) {
        console.error('注册失败:', error);
        errorMessage.value = error?.message?.includes('Failed to fetch') ? '无法连接服务器，请稍后重试' : `注册失败: ${error.message}`;
    } finally {
        loading.value = false;
    }
};

// 跳转到登录页面
const navigateToLogin = () => {
    showModal.value = false;
    timer && clearInterval(timer);
    router.push('/auth/login');
};
// 切换密码可见性
const togglePasswordVisible = () => {
    passwordVisible.value = !passwordVisible.value;
};
const toggleConfirmPasswordVisible = () => {
    confirmPasswordVisible.value = !confirmPasswordVisible.value;
};

const downloadPrivateKey = () => {
    if (!keyFileBase64.value) {
        errorMessage.value = '未获取到私钥文件，请重新注册或联系管理员。';
        return;
    }

    const byteCharacters = atob(keyFileBase64.value);
    const byteArray = new Uint8Array(byteCharacters.length);
    for (let i = 0; i < byteCharacters.length; i++) {
        byteArray[i] = byteCharacters.charCodeAt(i);
    }
    const blob = new Blob([byteArray], { type: 'application/octet-stream' });
    const url = URL.createObjectURL(blob);
    const link = document.createElement('a');
    link.href = url;
    link.download = keyFileName.value || 'abe_secret_key_1.bin';
    link.click();
    URL.revokeObjectURL(url);
};
</script>

<template>
    <div class="bg-surface-0 dark:bg-surface-900">
        <div id="home" class="landing-wrapper overflow-hidden">
            <div class="py-6 px-6 mx-0 md:mx-12 lg:mx-20 lg:px-20 flex items-center justify-between relative lg:static">
                <TopbarWidget />
            </div>
        </div>
    </div>
    <div
        class="bg-surface-50 dark:bg-surface-950 flex items-center justify-center min-h-screen min-w-[100vw] overflow-hidden">
        <div class="flex flex-col items-center justify-center">
            <div
                style="border-radius: 56px; padding: 0.3rem; background: linear-gradient(180deg, var(--primary-color) 10%, rgba(33, 150, 243, 0) 30%)">
                <div class="w-full bg-surface-0 dark:bg-surface-900 py-20 px-8 sm:px-20" style="border-radius: 53px">
                    <div class="text-center mb-8">
                        <svg viewBox="0 0 54 40" fill="none" xmlns="http://www.w3.org/2000/svg"
                            class="mb-8 w-16 shrink-0 mx-auto">
                            <!-- SVG Path 内容略 -->
                        </svg>
                        <h2 class="text-3xl font-bold">注册</h2>
                    </div>

                    <form @submit.prevent="register_send">
                        <div class="mb-6">
                            <label for="username" class="block text-sm font-medium">用户名</label>
                            <input id="username" v-model="username" type="text" placeholder="请输入用户名"
                                class="mt-1 block w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:ring-primary focus:border-primary sm:text-sm">
                        </div>

                        <div class="mb-6 relative">
                            <label for="password" class="block text-sm font-medium">密码</label>
                            <input :type="passwordVisible ? 'text' : 'password'" id="password" v-model="password"
                                placeholder="请输入密码"
                                class="mt-1 block w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:ring-primary focus:border-primary sm:text-sm pr-10">
                            <span @click="togglePasswordVisible"
                                class="absolute right-3 top-9 cursor-pointer select-none text-gray-400">
                                <svg v-if="!passwordVisible" xmlns="http://www.w3.org/2000/svg" class="h-5 w-5"
                                    fill="none" viewBox="0 0 24 24" stroke="currentColor">
                                    <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2"
                                        d="M15 12a3 3 0 11-6 0 3 3 0 016 0z" />
                                    <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2"
                                        d="M2.458 12C3.732 7.943 7.523 5 12 5c4.478 0 8.268 2.943 9.542 7-1.274 4.057-5.064 7-9.542 7-4.477 0-8.268-2.943-9.542-7z" />
                                </svg>
                                <svg v-else xmlns="http://www.w3.org/2000/svg" class="h-5 w-5" fill="none"
                                    viewBox="0 0 24 24" stroke="currentColor">
                                    <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2"
                                        d="M13.875 18.825A10.05 10.05 0 0112 19c-4.478 0-8.268-2.943-9.542-7a9.956 9.956 0 012.293-3.95m3.25-2.6A9.956 9.956 0 0112 5c4.478 0 8.268 2.943 9.542 7a9.965 9.965 0 01-4.293 5.03M15 12a3 3 0 11-6 0 3 3 0 016 0z" />
                                </svg>
                            </span>
                        </div>

                        <div class="mb-6 relative">
                            <label for="confirmPassword" class="block text-sm font-medium">确认密码</label>
                            <input :type="confirmPasswordVisible ? 'text' : 'password'" id="confirmPassword"
                                v-model="confirmPassword" placeholder="请再次输入密码"
                                class="mt-1 block w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:ring-primary focus:border-primary sm:text-sm pr-10">
                            <span @click="toggleConfirmPasswordVisible"
                                class="absolute right-3 top-9 cursor-pointer select-none text-gray-400">
                                <svg v-if="!confirmPasswordVisible" xmlns="http://www.w3.org/2000/svg" class="h-5 w-5"
                                    fill="none" viewBox="0 0 24 24" stroke="currentColor">
                                    <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2"
                                        d="M15 12a3 3 0 11-6 0 3 3 0 016 0z" />
                                    <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2"
                                        d="M2.458 12C3.732 7.943 7.523 5 12 5c4.478 0 8.268 2.943 9.542 7-1.274 4.057-5.064 7-9.542 7-4.477 0-8.268-2.943-9.542-7z" />
                                </svg>
                                <svg v-else xmlns="http://www.w3.org/2000/svg" class="h-5 w-5" fill="none"
                                    viewBox="0 0 24 24" stroke="currentColor">
                                    <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2"
                                        d="M13.875 18.825A10.05 10.05 0 0112 19c-4.478 0-8.268-2.943-9.542-7a9.956 9.956 0 012.293-3.95m3.25-2.6A9.956 9.956 0 0112 5c4.478 0 8.268 2.943 9.542 7a9.965 9.965 0 01-4.293 5.03M15 12a3 3 0 11-6 0 3 3 0 016 0z" />
                                </svg>
                            </span>
                        </div>

                        <!-- 显示错误消息 -->
                        <div v-if="errorMessage" class="mb-6 bg-red-100 text-red-700 px-4 py-2 rounded-md">
                            <p>{{ errorMessage }}</p>
                        </div>

                        <div class="flex items-center justify-between">
                            <button type="submit" :disabled="loading"
                                class="inline-flex justify-center py-2 px-4 border border-transparent shadow-sm text-sm font-medium rounded-md text-white bg-primary hover:bg-primary-dark focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-primary">
                                {{ loading ? '注册中...' : '注册' }}
                            </button>
                        </div>
                    </form>
                </div>
            </div>
        </div>
    </div>

    <!-- 模态框 -->
    <div v-if="showModal" class="fixed inset-0 flex items-center justify-center bg-gray-500 bg-opacity-50 z-50">
        <div class="bg-white p-6 rounded-lg w-80">
            <h3 class="text-lg font-bold text-center mb-4">注册成功</h3>
            <p class="text-center">{{ successMessage }}</p>
            <p class="text-center text-sm text-green-700 mt-2" v-if="keyFileName">请立即下载并保存私钥文件：{{ keyFileName }}</p>
            <p class="text-center text-sm text-gray-500 mt-2">{{ countdown }} 秒后自动跳转到登录页面</p>
            <div class="flex justify-between mt-6">
                <button v-if="keyFileBase64" @click="downloadPrivateKey"
                    class="px-4 py-2 bg-emerald-500 text-white rounded-md hover:bg-emerald-600">
                    下载密钥文件
                </button>
                <button @click="navigateToLogin" class="px-4 py-2 bg-blue-500 text-white rounded-md hover:bg-blue-600">
                    立即登录
                </button>
                <button @click="showModal = false; timer && clearInterval(timer);"
                    class="px-4 py-2 bg-gray-300 text-gray-700 rounded-md hover:bg-gray-400">
                    留在当前页面
                </button>
            </div>
        </div>
    </div>
</template>
