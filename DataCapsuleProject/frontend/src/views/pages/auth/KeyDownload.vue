<script setup>
import TopbarWidget from '@/components/landing/TopbarWidget.vue';
import { authHeaders, clearAuthState } from '@/utils/auth';
import { ref } from 'vue';
import { useRouter } from 'vue-router';

const keyFileName = ref('');
const keyFileBase64 = ref('');
const statusMessage = ref('点击按钮后将从后端获取最新密钥文件。');
const router = useRouter();

const fetchPrivateKey = async () => {
    const response = await fetch('/api/user/downloadPrivateKey', {
        method: 'GET',
        headers: authHeaders()
    });

    const payload = await response.json().catch(() => ({}));

    if (response.status === 401) {
        clearAuthState();
        throw new Error('登录状态已失效，请重新登录。');
    }
    if (!response.ok) {
        throw new Error(payload.error || '获取密钥文件失败');
    }
    if (!payload.abe_private_key_base64) {
        throw new Error('后端未返回密钥文件内容');
    }
    keyFileName.value = payload.abe_private_key_file_name || 'abe_secret_key_1.bin';
    keyFileBase64.value = payload.abe_private_key_base64;
};

const downloadPrivateKey = async () => {
    try {
        await fetchPrivateKey();
    } catch (error) {
        statusMessage.value = error.message || '获取密钥文件失败';
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
    statusMessage.value = '密钥文件已开始下载，正在返回首页...';
    setTimeout(() => {
        router.push('/');
    }, 1000);
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
                <div class="w-full bg-surface-0 dark:bg-surface-900 py-16 px-8 sm:px-20" style="border-radius: 53px">
                    <div class="text-center mb-8">
                        <h2 class="text-3xl font-bold">下载密钥</h2>
                        <p class="mt-4 text-base">{{ statusMessage }}</p>
                    </div>
                    <div class="flex justify-center">
                        <button @click="downloadPrivateKey"
                            class="inline-flex justify-center py-2 px-6 border border-transparent shadow-sm text-sm font-medium rounded-md text-white bg-primary hover:bg-primary-dark focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-primary">
                            下载密钥文件
                        </button>
                    </div>
                </div>
            </div>
        </div>
    </div>
</template>
