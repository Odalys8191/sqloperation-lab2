<script setup>
import { DataCapsulesInfoService } from '@/service/DataCapsulesInfoService';
import { FilterMatchMode, FilterOperator } from '@primevue/core/api';
import { onBeforeMount, ref } from 'vue';

const loading1 = ref(null);

const data_capsules = ref(null);
const filters_data_capsules = ref(null);

// 加载数据
onBeforeMount(() => {
    loading1.value = true;
    // Data Capsules
    DataCapsulesInfoService.getData().then((data) => (data_capsules.value = data));
    loading1.value = false;

    initFiltersDataCapsules();
});

function initFiltersDataCapsules() {
    filters_data_capsules.value = {
        global: { value: null, matchMode: FilterMatchMode.CONTAINS },
        DCID: { operator: FilterOperator.AND, constraints: [{ value: null, matchMode: FilterMatchMode.STARTS_WITH }] },
        PTEEID: { operator: FilterOperator.AND, constraints: [{ value: null, matchMode: FilterMatchMode.STARTS_WITH }] },
        Policy: { operator: FilterOperator.AND, constraints: [{ value: null, matchMode: FilterMatchMode.STARTS_WITH }] },
        'DataIntro.Columns': { operator: FilterOperator.AND, constraints: [{ value: null, matchMode: FilterMatchMode.STARTS_WITH }] },
        'DataIntro.DataDescription': { operator: FilterOperator.AND, constraints: [{ value: null, matchMode: FilterMatchMode.STARTS_WITH }] },
        'DataIntro.DataLen': { operator: FilterOperator.AND, constraints: [{ value: null, matchMode: FilterMatchMode.STARTS_WITH }] },
    };
}

</script>

<!-- ---------------------------end of scripts--------------------------- -->

<template>

    <div class="card">
        <div class="font-semibold text-3xl mb-4">现存数据胶囊</div>

        <!--
         paginator分页功能  
         rowHover鼠标悬停高亮 
         globalFilterFields全局搜索的字段 
         showGridlines表格的样式选项，表示表格的行列会显示网格线
         -->
        <DataTable :value="data_capsules" :paginator="true" :rows="10" dataKey="DCID" :rowHover="true"
            v-model:filters="filters_data_capsules" filterDisplay="menu" :loading="loading1"
            :filters="filters_data_capsules"
            :globalFilterFields="['DCID', 'PTEEID', 'DataIntro.Columns', 'DataIntro.DataDescription', 'DataIntro.DataLen', 'Policy']"
            showGridlines>
            <!-- Keyword Search 搜索框 -->
            <template #header>
                <div class="flex justify-between">
                    <Button type="button" icon="pi pi-filter-slash" label="Clear" outlined @click="clearFilter()" />
                    <IconField>
                        <InputIcon>
                            <i class="pi pi-search" />
                        </InputIcon>
                        <InputText v-model="filters_data_capsules['global'].value" placeholder="Keyword Search" />
                    </IconField>
                </div>
            </template>
            <template #empty> No data capsules found. </template>
            <template #loading> Loading data capsules data. Please wait. </template>

            <!-- 
            field: 数据的字段，对应data中的字段，用于显示数据
            header: 表头
            -->
            <!-- Column DCID -->
            <Column field="DCID" header="DCID" style="min-width: 12rem">
                <!-- 数据 -->
                <template #body="{ data }">
                    {{ data.DCID }}
                </template>
                <!-- 过滤器 -->
                <template #filter="{ filterModel }">
                    <InputText v-model="filterModel.value" type="text" placeholder="Search by DCID" />
                </template>
            </Column>

            <!-- Column PTEEID -->
            <Column field="PTEEID" header="PTEEID" style="min-width: 10rem">
                <!-- 数据 -->
                <template #body="{ data }">
                    {{ data.PTEEID }}
                </template>
                <!-- 过滤器 -->
                <template #filter="{ filterModel }">
                    <InputText v-model="filterModel.value" type="text" placeholder="Search by PTEEID" />
                </template>
            </Column>

            <!-- Column DataIntro.Columns -->
            <Column field="DataIntro.Columns" header="Columns" style="min-width: 10rem">
                <!-- 数据 -->
                <template #body="{ data }">
                    {{ data.DataIntro.Columns }}
                </template>
                <!-- 过滤器 -->
                <template #filter="{ filterModel }">
                    <InputText v-model="filterModel.value" type="text" placeholder="Search by Columns" />
                </template>
            </Column>

            <!-- Column DataIntro.DataDescription -->
            <Column field="DataIntro.DataDescription" header="DataDescription" style="min-width: 12rem">
                <!-- 数据 -->
                <template #body="{ data }">
                    {{ data.DataIntro.DataDescription }}
                </template>
                <!-- 过滤器 -->
                <template #filter="{ filterModel }">
                    <InputText v-model="filterModel.value" type="text" placeholder="Search by DataDescription" />
                </template>
            </Column>

            <!-- Column DataIntro.DataLen -->
            <Column field="DataIntro.DataLen" header="DataLen" style="min-width: 12rem">
                <!-- 数据 -->
                <template #body="{ data }">
                    {{ data.DataIntro.DataLen }}
                </template>
                <!-- 过滤器 -->
                <template #filter="{ filterModel }">
                    <InputText v-model="filterModel.value" type="text" placeholder="Search by DataLen" />
                </template>
            </Column>

            <!-- Column Policy -->
            <Column field="Policy" header="Policy" style="min-width: 30rem">
                <!-- 数据 -->
                <template #body="{ data }">
                    {{ data.Policy }}
                </template>
                <!-- 过滤器 -->
                <template #filter="{ filterModel }">
                    <InputText v-model="filterModel.value" type="text" placeholder="Search by Policy" />
                </template>
            </Column>

        </DataTable>
    </div>

</template>

<style scoped lang="scss">
:deep(.p-datatable-frozen-tbody) {
    font-weight: bold;
}

:deep(.p-datatable-scrollable .p-frozen-column) {
    font-weight: bold;
}
</style>
