<script setup>
import { CountryService } from '@/service/CountryService';
import { NodeService } from '@/service/NodeService';
import { authHeaders, requireValidSessionBeforeAction } from '@/utils/auth';
import { onMounted, ref, watch, reactive } from 'vue';

const autoValue = ref(null);
const treeSelectNodes = ref(null);
onMounted(() => {
    CountryService.getCountries().then((data) => (autoValue.value = data));
    NodeService.getTreeNodes().then((data) => (treeSelectNodes.value = data));
});

const owner_id = window.localStorage.getItem('user_id'); // 拥有者ID
const data_description = ref(''); // 存储胶囊数据描述
const column_information = ref(''); // 数据列信息
const user_id = ref(''); // 使用者ID
const abe_policy = ref(''); // ABE策略
const usable_times = ref(0); // 可用次数
const selected_categories = ref([]); // 存储选择的使用方式大类
const selected_data_statistics = ref([]); // 存储选择的Data Statistics子类
const selected_machine_learning = ref([]); // 存储选择的Machine Learning子类
const selected_model_finetune = ref([]); // 存储选择的Model Finetune子类
const data_statistics_count_limit_columns = ref(''); // Count允许的列
const data_statistics_min_limit_columns = ref(''); // Min允许的列
const data_statistics_max_limit_columns = ref(''); // Max允许的列
const data_statistics_avg_limit_columns = ref(''); // Avg允许的列
const data_statistics_sum_limit_columns = ref(''); // Sum允许的列
const sql_operation_limit = ref(''); // Select禁止的列
const office_operation_user_count = ref(0); // Office Operation 使用者数量
const office_operation_user_policies = ref([]); // Office Operation 使用者策略
const selected_office_operation_type = ref(null); // Office Operation 文件类型
const response_message = ref(''); // 存储返回的消息
const dc_id = ref(''); // 存储返回的 dc_id
const usageCategories = ref([
    { label: 'Data Statistics', value: 'DATA_STATISTICS' },
    { label: 'Database Operation', value: 'DATABASE_OPERATION' },
    { label: 'Machine Learning', value: 'MACHINE_LEARNING' },
    { label: 'Model Finetune', value: 'MODEL_FINETUNE' },
    { label: 'Office Operation', value: 'OFFICE_OPERATION' },
    { label: 'SQL Operation', value: 'SQL_OPERATION' },
    // { label: 'Plain Text', value: 'PLAIN_TEXT' },
]);
const dataStatisticsSubcategories = ref([
    { label: 'COUNT', value: 'COUNT' },
    { label: 'MIN', value: 'MIN' },
    { label: 'MAX', value: 'MAX' },
    { label: 'AVG', value: 'AVG' },
    { label: 'SUM', value: 'SUM' },
]);
const machineLearningSubcategories = ref([
    { label: 'Classification', value: 'CLASSIFICATION' },
    { label: 'Regression', value: 'REGRESSION' },
    { label: 'Clustering', value: 'CLUSTERING' },
    { label: 'Dimensionality Reduction', value: 'DIMENSIONALITY_REDUCTION' },
]);
const modelFinetuneSubcategories = ref([
    { label: 'deepseek-ai/DeepSeek-R1-Distill-Qwen-1.5B', value: 'DeepSeek_R1_Distill_Qwen_1_5B' },
    { label: 'deepseek-ai/DeepSeek-R1-Distill-Qwen-7B', value: 'DeepSeek_R1_Distill_Qwen_7B' },
    { label: 'deepseek-ai/DeepSeek-R1-Distill-Qwen-14B', value: 'DeepSeek_R1_Distill_Qwen_14B' },
    { label: 'deepseek-ai/DeepSeek-R1-Distill-Qwen-32B', value: 'DeepSeek_R1_Distill_Qwen_32B' },
    { label: 'deepseek-ai/DeepSeek-R1', value: 'DeepSeek_R1' },
]);
const officeOperationTypes = ref([
    { label: 'DOCX', value: 'DOCX' },
    { label: 'XLSX', value: 'XLSX' },
    { label: 'PPTX', value: 'PPTX' },
    { label: 'DOC', value: 'DOC' },
    { label: 'XLS', value: 'XLS' },
    { label: 'PPT', value: 'PPT' },
]);
const SQLOperation = reactive({
    table: '',
    default_access: 'read',
    row_rules: [{ condition: '', access: 'deny' }],
    column_rules: {},
    op_rules: { join: 'deny', nested_query: 'allow', window_functions: 'deny' }
});
const newColumnName = ref('');
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
const officeOperationTypeMap = {
    DOCX: 0,
    XLSX: 1,
    PPTX: 2,
    DOC: 3,
    XLS: 4,
    PPT: 5
};
function addRowRule() {
    SQLOperation.row_rules.push({ condition: '', access: 'deny' });
}
function removeRowRule(index) {
    SQLOperation.row_rules.splice(index, 1);
}
function addColumnRule() {
    const columnName = newColumnName.value.trim();
    if (!columnName) return;
    if (!SQLOperation.column_rules[columnName]) {
        SQLOperation.column_rules[columnName] = {
            access: 'read',
            aggs: [],
            distinct: 'allow',
            order_by: 'allow',
            limit_with_order: 'allow'
        };
        newColumnName.value = '';
    }
}
function removeColumnRule(columnName) {
    delete SQLOperation.column_rules[columnName];
}
function generatePolicyJson() {
    return {
        ComputeType: [1],
        SQLOperation: {
            table: SQLOperation.table,
            default_access: SQLOperation.default_access,
            rules: {
                row_rules: SQLOperation.row_rules,
                column_rules: SQLOperation.column_rules,
                op_rules: SQLOperation.op_rules
            }
        }
    };
}
function mapToEnum(array, enumMap) {
    return array
        .map(item => enumMap[item.toUpperCase()])
        .filter(item => item !== undefined);
}
function splitToArray(value) {
    return value ? value.split(',').map(item => item.trim()).filter(item => item !== '') : [];
}
watch(office_operation_user_count, (newCount) => {
    const currentPolicies = office_operation_user_policies.value;
    const newPolicies = Array(newCount).fill('');
    for (let i = 0; i < Math.min(currentPolicies.length, newCount); i++) {
        newPolicies[i] = currentPolicies[i];
    }
    office_operation_user_policies.value = newPolicies;
});
const uploaded_file = ref(null);
const uploaded_file_content = ref('');
const showSQLPolicy = ref(false);
const handleFileUpload = (event) => {
    const file = event.target.files[0];
    if (file) {
        uploaded_file.value = file;
        const reader = new FileReader();
        reader.onload = (e) => {
            uploaded_file_content.value = e.target.result;
        };
        reader.onerror = (e) => {
            console.error('Error reading file:', e);
            alert('文件读取失败');
        };
        reader.readAsDataURL(file);
    }
    else {
        uploaded_file.value = null;
        uploaded_file_content.value = '';
    }
};
async function uploadDataCapsule() {
    if (!(await requireValidSessionBeforeAction('进行胶囊打包'))) {
        response_message.value = '请先登录';
        dc_id.value = '';
        return;
    }

    const payload = {
        owner_id: owner_id,
        // user_id: user_id.value,
        abe_policy: abe_policy.value,
        usable_times: usable_times.value,
        data_description: data_description.value,
        column_information: column_information.value,
        selected_categories: mapToEnum(selected_categories.value.map(item => item.value), computeTypeMap),
        selected_data_statistics: mapToEnum(selected_data_statistics.value.map(item => item.value), dataStatisticsTypeMap),
        data_statistics_count_limit_columns: splitToArray(data_statistics_count_limit_columns.value),
        data_statistics_min_limit_columns: splitToArray(data_statistics_min_limit_columns.value),
        data_statistics_max_limit_columns: splitToArray(data_statistics_max_limit_columns.value),
        data_statistics_avg_limit_columns: splitToArray(data_statistics_avg_limit_columns.value),
        data_statistics_sum_limit_columns: splitToArray(data_statistics_sum_limit_columns.value),
        sql_operation_limit: sql_operation_limit.value,
        selected_machine_learning: selected_machine_learning.value.map(item => item.value),
        office_operation_user_policies: office_operation_user_policies.value,
        office_users_number: office_operation_user_count.value,
        selected_office_operation_type: selected_office_operation_type.value ? officeOperationTypeMap[selected_office_operation_type.value.value] : null,
        // selected_model_finetune: selected_model_finetune.value.map(item => item.value),
        file: uploaded_file_content.value ? uploaded_file_content.value : "",
    };
    if (selected_categories.value.some(c => c.value === 'SQL_OPERATION')) {
        payload.sql_operation_policy = generatePolicyJson().SQLOperation;
    }
    fetch('/api/producer/generateDataCapsule', {
        method: 'POST',
        headers: authHeaders(),
        body: JSON.stringify(payload),
    })
        .then(async (response) => {
            const data = await response.json().catch(() => ({}));
            if (!response.ok) {
                throw new Error(data.error || data.message || 'Upload failed');
            }
            return data;
        })
        .then((data) => {
            if (!data.dc_id) {
                throw new Error('打包失败：未返回胶囊ID');
            }
            response_message.value = data.message || 'Data capsule generated successfully';
            dc_id.value = data.dc_id;
            console.log('dc_id:', dc_id.value);
        })
        .catch((error) => {
            console.error('Error uploading data capsule:', error);
            response_message.value = error.message ? `上传失败：${error.message}` : '上传失败，请重试';
            dc_id.value = '';
        });
}
</script>

<template>

    <div class="card flex flex-col gap-4 w-full">
        <div class="font-semibold text-3xl">数据胶囊打包</div>

        <!-- 胶囊数据描述输入框 -->
        <div class="font-semibold text-xl">数据内容描述</div>
        <Textarea v-model="data_description" placeholder="请输入您提交的数据的内容描述" :autoResize="true" rows="3" cols="30" />

        <!-- 数据列信息输入框 -->
        <!-- <div class="font-semibold text-xl">数据列信息</div>
        <Textarea v-model="column_information" placeholder="请输入您提交的数据的列信息" :autoResize="true" rows="2" cols="30" /> -->

        <!-- 使用者ID输入框 -->
        <!-- <div class="font-semibold text-xl">使用者ID</div> -->
        <!-- <InputText v-model="user_id" placeholder="请输入使用者ID" /> -->

        <!-- ABE策略输入框 -->
        <div class="font-semibold text-xl">ABE策略</div>
        <InputText v-model="abe_policy" placeholder="请输入ABE策略" />

        <!-- 可用次数输入框 -->
        <div class="font-semibold text-xl">可用次数</div>
        <InputNumber v-model="usable_times" placeholder="请输入可用次数" />

        <!-- 胶囊使用方式大类选择框（多选） -->
        <div class="font-semibold text-xl">胶囊允许的使用方式</div>
        <MultiSelect v-model="selected_categories" :options="usageCategories" optionLabel="label"
            placeholder="请选择该胶囊允许的使用方式" display="chip">
        </MultiSelect>

        <!-- 根据选择的大类动态显示对应的子类选择框 -->
        <!-- DATA_STATISTICS -->
        <!-- 数据列信息输入框 -->
        <div v-if="selected_categories.some(category => category.value === 'DATA_STATISTICS' || category.value === 'DATABASE_OPERATION')"
            class="font-semibold text-xl">数据列信息</div>
        <Textarea
            v-if="selected_categories.some(category => category.value === 'DATA_STATISTICS' || category.value === 'DATABASE_OPERATION')"
            v-model="column_information" placeholder="请输入您提交的数据的列信息" :autoResize="true" rows="2" cols="30" />

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
            class="font-semibold text-base">COUNT操作允许列</div>
        <Textarea v-if="selected_data_statistics.some(subcategory => subcategory.value === 'COUNT')"
            v-model="data_statistics_count_limit_columns" placeholder="COUNT操作允许的列, 列名之间以','分隔" :autoResize="true"
            rows="1" cols="30" />

        <div v-if="selected_data_statistics.some(subcategory => subcategory.value === 'MIN')"
            class="font-semibold text-base">
            MIN操作允许列
        </div>
        <Textarea v-if="selected_data_statistics.some(subcategory => subcategory.value === 'MIN')"
            v-model="data_statistics_min_limit_columns" placeholder="MIN操作允许的列, 列名之间以','分隔" :autoResize="true" rows="1"
            cols="30" />

        <div v-if="selected_data_statistics.some(subcategory => subcategory.value === 'MAX')"
            class="font-semibold text-base">
            MAX操作允许列
        </div>
        <Textarea v-if="selected_data_statistics.some(subcategory => subcategory.value === 'MAX')"
            v-model="data_statistics_max_limit_columns" placeholder="MAX操作允许的列, 列名之间以','分隔" :autoResize="true" rows="1"
            cols="30" />

        <div v-if="selected_data_statistics.some(subcategory => subcategory.value === 'AVG')"
            class="font-semibold text-base">
            AVG操作允许列
        </div>
        <Textarea v-if="selected_data_statistics.some(subcategory => subcategory.value === 'AVG')"
            v-model="data_statistics_avg_limit_columns" placeholder="AVG操作允许的列, 列名之间以','分隔" :autoResize="true" rows="1"
            cols="30" />

        <div v-if="selected_data_statistics.some(subcategory => subcategory.value === 'SUM')"
            class="font-semibold text-base">
            SUM操作允许列
        </div>
        <Textarea v-if="selected_data_statistics.some(subcategory => subcategory.value === 'SUM')"
            v-model="data_statistics_sum_limit_columns" placeholder="SUM操作允许的列, 列名之间以','分隔" :autoResize="true" rows="1"
            cols="30" />


        <!-- SQL_OPERATION -->
        <div v-if="selected_categories.some(c => c.value === 'SQL_OPERATION')" class="card p-4 my-4 border rounded">
            <h3>SQL Operation 策略配置</h3>

            <label>表名:</label>
            <input v-model="SQLOperation.table" placeholder="输入表名" class="border p-1 mb-2 w-full" />

            <label>默认访问权限:</label>
            <select v-model="SQLOperation.default_access" class="border p-1 mb-2 w-full">
                <option value="read">read</option>
                <option value="deny">deny</option>
            </select>

            <h4>行级规则</h4>
            <div v-for="(rule, index) in SQLOperation.row_rules" :key="index" class="mb-2 flex gap-2 items-center">
                <input v-model="rule.condition" placeholder="条件 " class="border p-1 flex-grow" />
                <select v-model="rule.access" class="border p-1 w-24">
                    <option value="read">read</option>
                    <option value="deny">deny</option>
                </select>
                <button @click="removeRowRule(index)" class="text-red-600">删除</button>
            </div>
            <button @click="addRowRule" class="my-2 px-3 py-1 bg-blue-500 text-white rounded">添加行规则</button>

            <h4>列级规则</h4>
            <div class="mb-2">
                <input v-model="newColumnName" placeholder="输入列名添加规则" class="border p-1 w-48" />
                <button @click="() => { addColumnRule(newColumnName); newColumnName = ''; }"
                    class="ml-2 px-3 py-1 bg-green-500 text-white rounded">添加列规则</button>
            </div>

            <div v-for="(rule, colName) in SQLOperation.column_rules" :key="colName" class="mb-4 border p-3 rounded">
                <div class="flex justify-between items-center mb-1">
                    <strong>{{ colName }}</strong>
                    <button @click="() => removeColumnRule(colName)" class="text-red-600">删除列规则</button>
                </div>

                <label>访问权限:</label>
                <select v-model="rule.access" class="border p-1 mb-2 w-full">
                    <option value="read">read</option>
                    <option value="deny">deny</option>
                    <option value="agg">agg</option>
                </select>

                <div v-if="rule.access === 'agg'">
                    <label>允许的聚合函数 (勾选):</label>
                    <div class="flex gap-2 mb-2">
                        <label><input type="checkbox" value="sum" v-model="rule.aggs" /> sum</label>
                        <label><input type="checkbox" value="avg" v-model="rule.aggs" /> avg</label>
                        <label><input type="checkbox" value="count" v-model="rule.aggs" /> count</label>
                        <label><input type="checkbox" value="max" v-model="rule.aggs" /> max</label>
                        <label><input type="checkbox" value="min" v-model="rule.aggs" /> min</label>
                    </div>
                </div>

                <label>distinct 权限:</label>
                <select v-model="rule.distinct" class="border p-1 mb-2 w-full">
                    <option value="allow">allow</option>
                    <option value="deny">deny</option>
                </select>

                <label>order_by 权限:</label>
                <select v-model="rule.order_by" class="border p-1 mb-2 w-full">
                    <option value="allow">allow</option>
                    <option value="deny">deny</option>
                </select>

                <label>limit_with_order 权限:</label>
                <select v-model="rule.limit_with_order" class="border p-1 mb-2 w-full">
                    <option value="allow">allow</option>
                    <option value="deny">deny</option>
                </select>
            </div>

            <h4>操作级规则</h4>
            <div class="mb-2">
                <label>join:</label>
                <select v-model="SQLOperation.op_rules.join" class="border p-1 mb-2 w-full">
                    <option value="allow">allow</option>
                    <option value="deny">deny</option>
                </select>
            </div>
            <div class="mb-2">
                <label>nested_query:</label>
                <select v-model="SQLOperation.op_rules.nested_query" class="border p-1 mb-2 w-full">
                    <option value="allow">allow</option>
                    <option value="deny">deny</option>
                </select>
            </div>
            <div class="mb-2">
                <label>window_functions:</label>
                <select v-model="SQLOperation.op_rules.window_functions" class="border p-1 mb-2 w-full">
                    <option value="allow">allow</option>
                    <option value="deny">deny</option>
                </select>
            </div>

            <button @click="showSQLPolicy = !showSQLPolicy" class="mt-4 px-4 py-2 bg-indigo-600 text-white rounded">
                {{ showSQLPolicy ? '隐藏' : '显示' }}策略 JSON
            </button>
            <div v-if="showSQLPolicy" class="mt-2 p-2 bg-gray-100 rounded text-xs">
                <pre>{{ JSON.stringify(generatePolicyJson(), null, 2) }}</pre>
            </div>
        </div>
        <!-- DATABASE_OPERATION -->
        <div v-if="selected_categories.some(category => category.value === 'DATABASE_OPERATION')"
            class="font-semibold text-xl">
            SQL操作限制
        </div>
        <div v-if="selected_categories.some(category => category.value === 'DATABASE_OPERATION')"
            class="flex flex-col md:flex-row gap-4">
            <Textarea v-model="sql_operation_limit" placeholder="SQL操作的限制" :autoResize="true" rows="1" cols="30" />
        </div>


        <!-- MACHINE_LEARNING -->
        <div v-if="selected_categories.some(category => category.value === 'MACHINE_LEARNING')"
            class="font-semibold text-xl">
            机器学习任务子类型
        </div>
        <div v-if="selected_categories.some(category => category.value === 'MACHINE_LEARNING')"
            class="flex flex-col md:flex-row gap-4">
            <MultiSelect v-model="selected_machine_learning" :options="machineLearningSubcategories" optionLabel="label"
                placeholder="选择机器学习任务允许的子类型" display="chip" />
        </div>

        <!-- MODEL_FINETUNE -->
        <!--选择预训练模型-->
        <div v-if="selected_categories.some(category => category.value === 'MODEL_FINETUNE')"
            class="font-semibold text-xl">
            预训练模型
        </div>
        <div v-if="selected_categories.some(category => category.value === 'MODEL_FINETUNE')"
            class="flex flex-col md:flex-row gap-4">
            <MultiSelect v-model="selected_model_finetune" :options="modelFinetuneSubcategories" optionLabel="label"
                placeholder="选择预训练模型" display="chip" />
        </div>

        <!-- OFFICE_OPERATION -->
        <div v-if="selected_categories.some(category => category.value === 'OFFICE_OPERATION')"
            class="font-semibold text-xl">
            Office 文件类型
        </div>
        <div v-if="selected_categories.some(category => category.value === 'OFFICE_OPERATION')"
            class="flex flex-col md:flex-row gap-4">
            <Dropdown v-model="selected_office_operation_type" :options="officeOperationTypes" optionLabel="label"
                placeholder="选择Office文件类型" />
        </div>

        <div v-if="selected_categories.some(category => category.value === 'OFFICE_OPERATION')"
            class="font-semibold text-xl">
            Office Operation 使用者数量
        </div>
        <div v-if="selected_categories.some(category => category.value === 'OFFICE_OPERATION')"
            class="flex flex-col md:flex-row gap-4">
            <InputNumber v-model="office_operation_user_count" placeholder="请输入使用者数量" />
        </div>

        <div v-if="selected_categories.some(category => category.value === 'OFFICE_OPERATION') && office_operation_user_count > 0"
            class="font-semibold text-xl">
            使用者ABE策略
        </div>
        <div v-for="(policy, index) in office_operation_user_policies" :key="index"
            v-if="selected_categories.some(category => category.value === 'OFFICE_OPERATION') && office_operation_user_count > 0"
            class="flex flex-col md:flex-row gap-4">
            <InputText v-model="office_operation_user_policies[index]"
                :placeholder="'请输入使用者 ' + (index + 1) + ' 的ABE策略'" />
        </div>

        <!-- 上传胶囊文件 -->
        <!-- <div class="font-semibold text-xl">Upload Data Capsule File</div>
        <div class="flex flex-col md:flex-row gap-4">
            <FileUpload name="file" v-model="uploaded_file" :multiple="false" accept=".json,.csv,.txt,.sql"
                :maxFileSize="1000000000" customUpload @upload="uploadFile" />
        </div> -->

        <!-- 文件上传输入框 -->
        <div class="font-semibold text-xl">上传胶囊数据文件</div>
        <!-- <input type="file" @change="handleFileUpload" accept=".txt,.csv,.json,.sql" /> -->
        <input type="file" @change="handleFileUpload" />
        <!-- <p v-if="validationErrors.uploaded_file" class="text-red-500">请上传文件</p> -->
        <!-- <div v-if="uploaded_file_content" class="mt-4 p-2 border border-gray-300 rounded">
            <p class="font-semibold">文件内容:</p>
            <pre class="whitespace-pre-wrap">{{ uploaded_file_content }}</pre>
        </div> -->


        <!-- 上传按钮 -->
        <Button label="Upload" icon="pi pi-check" @click="uploadDataCapsule" />

        <!-- 显示返回消息 -->
        <div v-if="response_message" class="mt-4 p-4 border rounded-lg bg-green-100">
            <div><strong>消息:</strong> {{ response_message }}</div>
            <div><strong>胶囊ID:</strong> {{ dc_id }}</div>
        </div>

        <div v-if="uploaded_file_content" class="mt-4 p-2 border border-gray-300 rounded">
            <p class="font-semibold">文件内容:</p>
            <pre class="whitespace-pre-wrap">{{ uploaded_file_content }}</pre>
        </div>
    </div>

</template>
