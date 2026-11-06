<template>
  <div class="login-wrap">
    <div class="login-card">
      <div class="login-brand">
        <div class="login-mark">串</div>
        <div>
          <b>小摊串串经营台账</b>
          <span>蔬菜 · 肉 · 丸子 · 肠 · 底料</span>
        </div>
      </div>

      <form @submit.prevent="submit">
        <label class="login-field">
          <span>手机号</span>
          <input
            v-model="phone"
            type="tel"
            inputmode="numeric"
            autocomplete="username"
            placeholder="请输入手机号"
            maxlength="11"
          />
        </label>

        <label class="login-field">
          <span>密码</span>
          <input
            v-model="password"
            type="password"
            autocomplete="current-password"
            placeholder="请输入密码"
          />
        </label>

        <div v-if="error" class="login-error">{{ error }}</div>

        <button class="login-btn" type="submit" :disabled="busy">
          {{ busy ? '正在登录…' : '登 录' }}
        </button>
      </form>

      <div class="login-foot">仅限登记过的手机号登录</div>
    </div>
  </div>
</template>

<script setup>
import { ref } from 'vue'
import { doLogin } from './store'

const phone = ref('')
const password = ref('')
const error = ref('')
const busy = ref(false)

async function submit() {
  error.value = ''
  if (!/^1\d{10}$/.test(phone.value.trim())) {
    error.value = '手机号格式不对，应该是 11 位数字'
    return
  }
  if (!password.value) {
    error.value = '请填密码'
    return
  }
  busy.value = true
  try {
    await doLogin(phone.value.trim(), password.value)
  } catch (e) {
    error.value = e.message || '登录失败'
  } finally {
    busy.value = false
  }
}
</script>
