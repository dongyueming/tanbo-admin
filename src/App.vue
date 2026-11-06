<template>
  <div v-if="auth.checking" class="boot">正在连接服务器…</div>

  <Login v-else-if="!auth.user" />

  <div v-else class="shell">
    <aside class="sidebar">
      <div class="brand">
        <div class="brand-mark">串</div>
        <div class="brand-text">
          <b>小摊经营台账</b>
          <span>BAOCAI · TANBO</span>
        </div>
      </div>

      <nav class="nav">
        <button
          v-for="t in tabs"
          :key="t.key"
          class="nav-item"
          :class="{ active: current === t.key }"
          @click="current = t.key"
        >
          <i class="nav-dot"></i>{{ t.label }}
        </button>
      </nav>

      <div class="sidebar-foot">
        <div class="who">{{ auth.user }}</div>
        <button class="logout-btn" @click="handleLogout">退出登录</button>
      </div>
    </aside>

    <main class="main">
      <div v-if="store.error" class="notice">{{ store.error }}</div>
      <component :is="currentView" v-bind="currentProps" />
    </main>

    <div v-if="store.toast" class="toast">{{ store.toast }}</div>
  </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import { store, auth, reload, checkAuth, doLogout } from './store'
import Login from './Login.vue'
import Dashboard from './views/Dashboard.vue'
import Batches from './views/Batches.vue'
import Strands from './views/Strands.vue'
import Sales from './views/Sales.vue'
import Losses from './views/Losses.vue'
import Reports from './views/Reports.vue'
import SaucePage from './views/SaucePage.vue'
import Expenses from './views/Expenses.vue'

const tabs = [
  { key: 'dashboard', label: '经营仪表盘' },
  { key: 'batches', label: '进货台账' },
  { key: 'strands', label: '剥叶穿串' },
  { key: 'sales', label: '销售记账' },
  { key: 'losses', label: '损耗报损' },
  { key: 'expenses', label: '其他支出' },
  { key: 'soup', label: '配汤' },
  { key: 'secret', label: '秘制酱' },
  { key: 'chili', label: '辣酱' },
  { key: 'sweet', label: '甜酱' },
  { key: 'reports', label: '利润报表' }
]

const views = {
  dashboard: Dashboard,
  batches: Batches,
  strands: Strands,
  sales: Sales,
  losses: Losses,
  expenses: Expenses,
  reports: Reports
}

const sauceProps = {
  soup: {
    category: 'soup',
    title: '配汤',
    subtitle: '锅底汤：牛骨 + 大料包 + 汤底调料，5天换一次',
    namePlaceholder: '30斤锅底汤',
    ingredientList: ['牛骨', '甘松', '灵草', '小茴', '桂皮', '香沙', '八角', '香叶', '白芷', '良姜', '草果', '孜然粉', '冰糖', '牛骨高汤', '六月豆瓣酱', '三五火锅底料', '排骨味鲜宝', '味精', '老母鸡粉', '莲花鸡精']
  },
  secret: {
    category: 'secret',
    title: '秘制酱',
    subtitle: '菜籽油 + 芝麻香辣酱 + 调料',
    namePlaceholder: '秘制酱',
    ingredientList: ['菜籽油', '芝麻', '细辣椒粉', '孜然粉', '双汇鸡精', '味精', '徽园芝麻香辣酱']
  },
  chili: {
    category: 'chili',
    title: '辣酱',
    subtitle: '菜籽油 + 花椒辣椒 + 花生芝麻',
    namePlaceholder: '辣酱（辣油）',
    ingredientList: ['菜籽油', '青花椒', '红花椒', '香叶', '桂皮', '八角', '草果', '小茴', '洋葱', '香菜', '香葱', '中粗辣椒粉', '奶油花生', '芝麻', '白酒']
  },
  sweet: {
    category: 'sweet',
    title: '甜酱',
    subtitle: '甜面酱 + 白糖 + 调料',
    namePlaceholder: '甜酱',
    ingredientList: ['徽园甜面酱', '白糖', '双汇鸡精', '味精', '油']
  }
}

const current = ref('dashboard')
const currentView = computed(() => {
  if (sauceProps[current.value]) return SaucePage
  return views[current.value] || Dashboard
})
const currentProps = computed(() => sauceProps[current.value] || {})

async function handleLogout() {
  await doLogout()
  current.value = 'dashboard'
}

onMounted(async () => {
  await checkAuth()
  if (auth.user) await reload()
})
</script>
