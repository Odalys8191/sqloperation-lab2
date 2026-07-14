<script setup>
import { CountryService } from '@/service/CountryService';
import { NodeService } from '@/service/NodeService';
import { onMounted, ref } from 'vue';

const autoValue = ref(null);
const treeSelectNodes = ref(null);

onMounted(() => {
    CountryService.getCountries().then((data) => (autoValue.value = data));
    NodeService.getTreeNodes().then((data) => (treeSelectNodes.value = data));
});
</script>

<template>
    <div class="card flex flex-col gap-4 w-full">
        <div class="font-semibold text-3xl">模型下载</div>

        <div class="font-semibold text-xl">模型ID</div>
        <InputText v-model="folderName" placeholder="请输入模型ID" />

        <!-- 下载按钮 -->
        <Button label="下载模型" @click="downloadFolder" />
    </div>
</template>


<script>
import { requireValidSessionBeforeAction } from '@/utils/auth';

export default {
    data() {
        return {
            folderName: "",
        };
    },
    methods: {
        async downloadFolder() {
            if (!(await requireValidSessionBeforeAction('下载模型'))) {
                return;
            }
            if (!this.folderName) {
                alert("请输入模型ID！");
                return;
            }

            const link = document.createElement("a");
            link.href = `http://localhost:1234/download?folder=${encodeURIComponent(this.folderName)}`;
            link.download = `${this.folderName}.zip`;
            link.click();
        }
    }
};
</script>

<!-- <script>
// 使用者ID，应该从登录状态中获取
const user_id = window.localStorage.getItem('user_id'); // 使用者ID
const password = window.localStorage.getItem('password'); // 使用者密码

// 模型ID
const Model = ref('');

// 返回消息
const response_message = ref(''); // 存储返回的消息
const compute_result = ref(''); // 存储返回的 compute_result

// 消费数据胶囊请求
function consumeDataCapsule() {
    // 构造 JSON 数据对象
    const payload = {
        Model: Model.value, // 模型ID
    };

    // 使用 fetch 发送 JSON 数据
    fetch('/api/consumer/consumeDataCapsule', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json', // 设置请求类型为 JSON
        },
        body: JSON.stringify(payload), // 转换 JSON 数据为字符串
    })
        .then((response) => {
            if (response.ok) {
                // 解析 JSON 数据
                return response.json();
            } else {
                throw new Error('Download failed');
            }
        })
        .then((data) => {
            response_message.value = data.message;

            alert(`下载成功!`); // 提示下载成功
        })
        .catch((error) => {
            alert('An error occurred while downloading the model: ' + error.message); // 提示下载失败
        });
}
</script> -->
