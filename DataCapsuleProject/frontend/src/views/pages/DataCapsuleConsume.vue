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

<script>
import { authHeaders, requireValidSessionBeforeAction } from '@/utils/auth';

// 使用者ID，应该从登录状态中获取
const user_id = window.localStorage.getItem('user_id'); // 使用者ID
const password = window.localStorage.getItem('password'); // 使用者密码

// 胶囊ID
const dc_id = ref(''); // 胶囊ID

// 使用方式大类
const selected_categories = ref([]); // 存储选择的使用方式大类

// Data Statistics
const selected_data_statistics = ref([]); // 选择的Data Statistics子类
const data_statistics_count_target_columns = ref(''); // Count执行的列
const data_statistics_min_target_columns = ref(''); // Min执行的列
const data_statistics_max_target_columns = ref(''); // Max执行的列
const data_statistics_avg_target_columns = ref(''); // Avg执行的列
const data_statistics_sum_target_columns = ref(''); // Sum执行的列

// Database Operation
const database_operation = ref(''); // Database Operation SQL语句
// SQL Operation
const sql_operation = ref(''); // SQL Operation SQL语句

// Machine Learning
const selected_machine_learning = ref([]); // 存储选择的Machine Learning子类

// Model Finetune
const selected_model_finetune = ref(''); // 存储选择的Model Finetune子类

// 返回消息
const response_message = ref(''); // 存储返回的消息
const compute_result = ref(''); // 存储返回的 compute_result

// 使用方式大类的选项
const usageCategories = ref([
    { label: 'Data Statistics', value: 'DATA_STATISTICS' },
    { label: 'SQL Operation', value: 'SQL_OPERATION' }, // 新增 SQL_OPERATION
    { label: 'Database Operation', value: 'DATABASE_OPERATION' },
    { label: 'Machine Learning', value: 'MACHINE_LEARNING' },
    { label: 'Model Finetune', value: 'MODEL_FINETUNE' },
    { label: 'Office Operation', value: 'OFFICE_OPERATION' }, // 新增 Office Operation
]);

// 各个大类的子类选项
// Data Statistics 子类
const dataStatisticsSubcategories = ref([
    { label: 'COUNT', value: 'COUNT' },
    { label: 'MIN', value: 'MIN' },
    { label: 'MAX', value: 'MAX' },
    { label: 'AVG', value: 'AVG' },
    { label: 'SUM', value: 'SUM' },
]);
// Machine Learning 子类
const machineLearningSubcategories = ref([
    { label: 'Classification', value: 'CLASSIFICATION' },
    { label: 'Regression', value: 'REGRESSION' },
    { label: 'Clustering', value: 'CLUSTERING' },
    { label: 'Dimensionality Reduction', value: 'DIMENSIONALITY_REDUCTION' },
]);
// Model Finetune 子类
const modelFinetuneSubcategories = ref([
    { label: 'deepseek-ai/DeepSeek-R1-Distill-Qwen-1.5B', value: 'DeepSeek_R1_Distill_Qwen_1_5B' },
    { label: 'deepseek-ai/DeepSeek-R1-Distill-Qwen-7B', value: 'DeepSeek_R1_Distill_Qwen_7B' },
    { label: 'deepseek-ai/DeepSeek-R1-Distill-Qwen-14B', value: 'DeepSeek_R1_Distill_Qwen_14B' },
    { label: 'deepseek-ai/DeepSeek-R1-Distill-Qwen-32B', value: 'DeepSeek_R1_Distill_Qwen_32B' },
    { label: 'deepseek-ai/DeepSeek-R1', value: 'DeepSeek_R1' },
]);


// 枚举映射
// Compute Type: edit this when add new compute type
const computeTypeMap = {
    DATA_STATISTICS: 0,
    SQL_OPERATION: 1,
    DATABASE_OPERATION: 2,
    MACHINE_LEARNING: 3,
    MODEL_FINETUNE: 4,
    PLAIN_TEXT: 5,
    OFFICE_OPERATION: 6
};

const dataStatisticsTypeMap = {
    COUNT: 0,
    MIN: 1,
    MAX: 2,
    AVG: 3,
    SUM: 4
};

// 辅助函数：将字符串值映射为对应的枚举编号
function mapToEnum(array, enumMap) {
    return array
        .map(item => enumMap[item.toUpperCase()]) // 转为大写以匹配枚举键
        .filter(item => item !== undefined); // 过滤无效值
}

// 辅助函数：将逗号分隔的字符串转换为数组
function splitToArray(value) {
    return value ? value.split(',').map(item => item.trim()).filter(item => item !== '') : [];
}

// 消费数据胶囊请求

async function consumeDataCapsule() {
    if (!(await requireValidSessionBeforeAction('进行胶囊消费'))) {
        response_message.value = '请先登录';
        compute_result.value = '';
        return;
    }

    // 构造 JSON 数据对象
    const payload = {
        dc_id: dc_id.value, // 胶囊ID
        user_id: user_id, // 使用者ID
        password: password, // 使用者密码
        selected_categories: mapToEnum(selected_categories.value.map(item => item.value), computeTypeMap), // 使用大类
        selected_data_statistics: mapToEnum(selected_data_statistics.value.map(item => item.value), dataStatisticsTypeMap), // Data Statistics 子类
        data_statistics_count_target_columns: splitToArray(data_statistics_count_target_columns.value), // COUNT 执行的列
        data_statistics_min_target_columns: splitToArray(data_statistics_min_target_columns.value), // MIN 执行的列
        data_statistics_max_target_columns: splitToArray(data_statistics_max_target_columns.value), // MAX 执行的列
        data_statistics_avg_target_columns: splitToArray(data_statistics_avg_target_columns.value), // AVG 执行的列
        data_statistics_sum_target_columns: splitToArray(data_statistics_sum_target_columns.value), // SUM 执行的列
        database_operation: {
            Query: database_operation.value
        }, // Database Operation SQL语句
        selected_machine_learning: selected_machine_learning.value.map(item => item.value), // 机器学习子类
        selected_model_finetune: selected_model_finetune.value.value, // 预训练模型
        user_abe_private_key: uploaded_abe_private_key_file_content.value // 上传的ABE私钥内容
    };
    // 如果选择了SQL_OPERATION，则加入SQLOperation字段
    if (selected_categories.value.some(item => item.value === 'SQL_OPERATION')) {
        payload.SQLOperation = { sql: sql_operation.value };
    }

    // 使用 fetch 发送 JSON 数据
    fetch('/api/consumer/consumeDataCapsule', {
        method: 'POST',
        headers: authHeaders(),
        body: JSON.stringify(payload), // 转换 JSON 数据为字符串
    })
        .then((response) => {
            if (response.ok) {
                // 解析 JSON 数据
                return response.json();
            } else {
                throw new Error('Upload failed');
            }
        })
        .then((data) => {
            // 提取返回的 compute_result 并赋值到变量
            response_message.value = data.message;
            compute_result.value = data.compute_result;
            // console.log('compute_result:', compute_result); // 打印 compute_result 到控制台
            // alert(`胶囊消费请求执行成功!`); // 提示消费成功
        })
        .catch((error) => {
            console.error('Error consuming data capsule:', error);
            // alert('An error occurred while consuming');
            response_message.value = '消费失败，请重试'; // 错误消息
            compute_result.value = ''; // 清空 compute_result
        });
}

// 上传密钥文件
const uploaded_abe_private_key_file = ref(null);
const uploaded_abe_private_key_file_content = ref('');
const handleFileUpload = (event) => {
    const file = event.target.files[0];
    if (file) {
        uploaded_abe_private_key_file.value = file;
        const reader = new FileReader();
        reader.onload = (e) => {
            uploaded_abe_private_key_file_content.value = e.target.result;
        };
        reader.onerror = (e) => {
            console.error('Error reading file:', e);
            alert('文件读取失败');
        };
        reader.readAsDataURL(file);
    }
    else {
        uploaded_abe_private_key_file.value = null;
        uploaded_abe_private_key_file_content.value = '';
    }
};

</script>


<template>
    <div class="card flex flex-col gap-4 w-full">
        <div class="font-semibold text-3xl">数据胶囊消费</div>

        <div class="font-semibold text-xl">胶囊ID</div>
        <InputText v-model="dc_id" placeholder="请输入胶囊ID" />

        <!-- 胶囊使用方式大类选择框（多选） -->
        <div class="font-semibold text-xl">使用方式</div>
        <MultiSelect v-model="selected_categories" :options="usageCategories" optionLabel="label"
            placeholder="请选择如何使用该胶囊" display="chip">
        </MultiSelect>

        <!-- 根据选择的大类动态显示对应的子类选择框 -->
        <!-- DATA_STATISTICS -->
        <div v-if="selected_categories.some(category => category.value === 'DATA_STATISTICS')"
            class="font-semibold text-xl">
            数据分析任务子类型
        </div>
        <div v-if="selected_categories.some(category => category.value === 'DATA_STATISTICS')"
            class="flex flex-col md:flex-row gap-4">
            <MultiSelect v-model="selected_data_statistics" :options="dataStatisticsSubcategories" optionLabel="label"
                placeholder="选择数据分析任务允许的子类型" display="chip" />
        </div>

        <!-- 根据选择的子类动态显示对应的子类限制输入栏 -->
        <div v-if="selected_data_statistics.some(subcategory => subcategory.value === 'COUNT')"
            class="font-semibold text-base">
            COUNT执行列
        </div>
        <Textarea v-if="selected_data_statistics.some(subcategory => subcategory.value === 'COUNT')"
            v-model="data_statistics_count_target_columns" placeholder="执行COUNT操作的列, 列名之间以','分隔" :autoResize="true"
            rows="1" cols="30" />

        <div v-if="selected_data_statistics.some(subcategory => subcategory.value === 'MIN')"
            class="font-semibold text-base">
            MIN执行列
        </div>
        <Textarea v-if="selected_data_statistics.some(subcategory => subcategory.value === 'MIN')"
            v-model="data_statistics_min_target_columns" placeholder="执行MIN操作的列, 列名之间以','分隔" :autoResize="true" rows="1"
            cols="30" />

        <div v-if="selected_data_statistics.some(subcategory => subcategory.value === 'MAX')"
            class="font-semibold text-base">
            MAX执行列
        </div>
        <Textarea v-if="selected_data_statistics.some(subcategory => subcategory.value === 'MAX')"
            v-model="data_statistics_max_target_columns" placeholder="执行MAX操作的列, 列名之间以','分隔" :autoResize="true" rows="1"
            cols="30" />

        <div v-if="selected_data_statistics.some(subcategory => subcategory.value === 'AVG')"
            class="font-semibold text-base">
            AVG执行列
        </div>
        <Textarea v-if="selected_data_statistics.some(subcategory => subcategory.value === 'AVG')"
            v-model="data_statistics_avg_target_columns" placeholder="执行AVG操作的列, 列名之间以','分隔" :autoResize="true" rows="1"
            cols="30" />

        <div v-if="selected_data_statistics.some(subcategory => subcategory.value === 'SUM')"
            class="font-semibold text-base">
            SUM执行列
        </div>
        <Textarea v-if="selected_data_statistics.some(subcategory => subcategory.value === 'SUM')"
            v-model="data_statistics_sum_target_columns" placeholder="执行SUM操作的列, 列名之间以','分隔" :autoResize="true" rows="1"
            cols="30" />


        <!-- SQL_OPERATION -->
        <div v-if="selected_categories.some(category => category.value === 'SQL_OPERATION')"
            class="font-semibold text-xl">
            SQL Operation 命令
        </div>
        <div v-if="selected_categories.some(category => category.value === 'SQL_OPERATION')"
            class="flex flex-col md:flex-row gap-4">
            <Textarea v-model="sql_operation" placeholder="请输入要执行的SQL语句" :autoResize="true" rows="1" cols="30" />
        </div>

        <!-- DATABASE_OPERATION -->
        <div v-if="selected_categories.some(category => category.value === 'DATABASE_OPERATION')"
            class="font-semibold text-xl">
            SQL命令
        </div>
        <div v-if="selected_categories.some(category => category.value === 'DATABASE_OPERATION')"
            class="flex flex-col md:flex-row gap-4">
            <Textarea v-model="database_operation" placeholder="要执行的SQL语句" :autoResize="true" rows="1" cols="30" />
        </div>


        <!-- MACHINE_LEARNING -->
        <div v-if="selected_categories.some(category => category.value === 'MACHINE_LEARNING')"
            class="font-semibold text-xl">
            机器学习任务子类型
        </div>
        <div v-if="selected_categories.some(category => category.value === 'MACHINE_LEARNING')"
            class="flex flex-col md:flex-row gap-4">
            <MultiSelect v-model="selected_machine_learning" :options="machineLearningSubcategories" optionLabel="label"
                placeholder="选择要执行的机器学习任务的子类型" display="chip" />
        </div>

        <!-- MODEL_FINETUNE -->
        <div v-if="selected_categories.some(category => category.value === 'MODEL_FINETUNE')"
            class="font-semibold text-xl">
            预训练模型
        </div>
        <div v-if="selected_categories.some(category => category.value === 'MODEL_FINETUNE')"
            class="flex flex-col md:flex-row gap-4">
            <Select v-model="selected_model_finetune" :options="modelFinetuneSubcategories" optionLabel="label"
                placeholder="选择要执行的预训练模型" display="chip" />
        </div>

        <!-- 文件上传输入框 -->
        <div class="font-semibold text-xl">上传ABE私钥</div>
        <input type="file" @change="handleFileUpload" />

        <!-- 上传按钮 -->
        <Button label="提交请求" icon="pi pi-check" @click="consumeDataCapsule" />

        <!-- 显示返回消息 -->
        <div v-if="response_message" class="mt-4 p-4 border rounded-lg bg-green-100">
            <div><strong>消息:</strong> {{ response_message }}</div>
            <div><strong>消费结果:</strong> {{ compute_result }}</div>
        </div>

        <div v-if="uploaded_abe_private_key_file_content" class="mt-4 p-2 border border-gray-300 rounded">
            <p class="font-semibold">文件内容:</p>
            <pre class="whitespace-pre-wrap">{{ uploaded_abe_private_key_file_content }}</pre>
        </div>
    </div>
</template>
