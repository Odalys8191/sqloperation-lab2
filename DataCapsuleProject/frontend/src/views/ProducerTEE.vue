<!-- sakai need to change -->
<template>
  <div class="container">
    <h2>表单</h2>
    <SakaiForm @submit="handleSubmit">
      <!-- 可使用次数 -->
      <SakaiInput
        v-model="usageTimes"
        label="可使用次数"
        type="number"
        required
      />

      <!-- 上传数据文件 -->
      <SakaiFileUpload
        v-model="dataFile"
        label="上传数据文件"
        accept=".csv,.txt,.xls"
        required
      />

      <!-- 使用者ID和限制 -->
      <div v-for="(userLimit, index) in userLimits" :key="index" class="user-limit-group">
        <div class="input-group">
          <!-- 使用者ID -->
          <SakaiInput
            v-model="userLimit.userId"
            label="使用者ID"
            type="text"
            required
          />
          <!-- 使用限制 -->
          <SakaiInput
            v-model="userLimit.limit"
            label="使用限制"
            type="text"
            required
          />
        </div>
        <!-- 删除按钮 -->
        <SakaiButton @click="removeUserLimit(index)" label="删除" variant="danger" />
      </div>

      <!-- 添加使用者ID和限制的按钮 -->
      <SakaiButton @click="addUserLimit" label="添加用户" variant="success" />

      <!-- 提交按钮 -->
      <SakaiButton label="提交" type="submit" />
    </SakaiForm>
  </div>
</template>

<script>
import {
  SakaiButton,
  SakaiForm,
  SakaiInput,
  SakaiFileUpload
} from 'sakai-vue'

export default {
  components: {
    SakaiButton,
    SakaiForm,
    SakaiInput,
    SakaiFileUpload
  },
  data() {
    return {
      usageTimes: '',
      dataFile: null,
      userLimits: [{ userId: '', limit: '' }]
    }
  },
  methods: {
    // 添加新的用户ID和限制组
    addUserLimit() {
      this.userLimits.push({ userId: '', limit: '' })
    },
    // 删除指定的用户ID和限制组
    removeUserLimit(index) {
      this.userLimits.splice(index, 1)
    },
    // 提交表单
    handleSubmit() {
      console.log('可使用次数:', this.usageTimes)
      console.log('上传的文件:', this.dataFile)
      console.log('使用者ID和限制:', this.userLimits)
    }
  }
}
</script>

<style scoped>
.container {
  max-width: 600px;
  margin: 0 auto;
  padding: 20px;
}

h2 {
  text-align: center;
}

.user-limit-group {
  margin-bottom: 10px;
}

.input-group {
  display: flex;
  justify-content: space-between;
}

.sakai-button {
  margin-top: 10px;
}
</style>
