import { reactive } from 'vue'
import { api } from './api'

export const store = reactive({
  batches: [],
  strands: [],
  sales: [],
  losses: [],
  expenses: [],
  ready: false,
  error: '',
  toast: ''
})

export const auth = reactive({
  user: null,
  checking: true
})

let toastTimer = null

export function notify(msg) {
  store.toast = msg
  if (toastTimer) clearTimeout(toastTimer)
  toastTimer = setTimeout(() => {
    store.toast = ''
  }, 3000)
}

export async function reload() {
  try {
    const data = await api.state()
    store.batches = data.batches || []
    store.strands = data.strands || []
    store.sales = data.sales || []
    store.losses = data.losses || []
    store.expenses = data.expenses || []
    store.ready = true
    store.error = ''
  } catch (e) {
    if (e.unauthorized) {
      auth.user = null
    }
    store.error = e.message || '数据加载失败'
    store.ready = true
  }
}

export async function checkAuth() {
  try {
    const me = await api.me()
    auth.user = me.phone
  } catch (e) {
    auth.user = null
  }
  auth.checking = false
}

export async function doLogin(phone, password) {
  const r = await api.login(phone, password)
  auth.user = r.phone
  await reload()
}

export async function doLogout() {
  try {
    await api.logout()
  } catch (e) {
    /* 忽略：本地状态照样清掉 */
  }
  auth.user = null
  store.batches = []
  store.strands = []
  store.sales = []
  store.losses = []
  store.expenses = []
  store.ready = false
}

export async function addItem(type, payload) {
  await api.create(type, payload)
  await reload()
  notify('已保存')
}

export async function updateItem(type, id, payload) {
  await api.update(type, id, payload)
  await reload()
  notify('已更新')
}

export async function removeItem(type, id) {
  await api.remove(type, id)
  await reload()
  notify('已删除')
}
