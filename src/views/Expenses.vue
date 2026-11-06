<template>
  <div>
    <div class="page-head">
      <div>
        <h1>其他支出</h1>
        <p>人工、煤气、摊位费、包装材料，这些不记，利润就是虚的</p>
      </div>
      <div class="head-actions">
        <button class="btn primary" @click="openForm()">+ 记一笔支出</button>
      </div>
    </div>

    <div class="grid kpi-grid" style="margin-bottom: 14px">
      <div class="kpi">
        <div class="kpi-label">累计支出</div>
        <div class="kpi-value">{{ money(totalCost) }}<span class="unit">元</span></div>
      </div>
      <div class="kpi">
        <div class="kpi-label">本月支出</div>
        <div class="kpi-value">{{ money(monthCost) }}<span class="unit">元</span></div>
      </div>
      <div class="kpi">
        <div class="kpi-label">日均支出</div>
        <div class="kpi-value">{{ money(dailyAvg) }}<span class="unit">元/天</span></div>
      </div>
      <div class="kpi">
        <div class="kpi-label">支出笔数</div>
        <div class="kpi-value">{{ list.length }}<span class="unit">笔</span></div>
      </div>
    </div>

    <div class="card" style="margin-bottom: 14px">
      <div class="card-title" style="padding: 16px 20px 8px">按类别汇总</div>
      <div class="cat-sum">
        <div v-for="c in byCat" :key="c.key" class="cat-sum-item">
          <div class="cat-sum-name"><i :style="{ background: c.color }"></i>{{ c.label }}</div>
          <div class="cat-sum-num">{{ money(c.cost) }} 元 · {{ c.count }} 笔</div>
        </div>
      </div>
    </div>

    <div class="card">
      <div class="table-wrap">
        <table>
          <thead>
            <tr>
              <th>日期</th>
              <th>类别</th>
              <th>项目</th>
              <th>金额</th>
              <th>备注</th>
              <th style="text-align: right">操作</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="r in list" :key="r.id">
              <td class="num">{{ r.date }}</td>
              <td><span class="tag" :style="{ background: colorOf(r.cat) + '18', color: colorOf(r.cat) }">{{ labelOf(r.cat) }}</span></td>
              <td>{{ r.name }}</td>
              <td class="num"><b>{{ money(r.cost) }}</b> 元</td>
              <td style="color: #7b8a81">{{ r.note || '-' }}</td>
              <td style="text-align: right">
                <button class="btn ghost" @click="openForm(r)">编辑</button>
                <button class="btn ghost" style="color: var(--danger)" @click="remove(r)">删除</button>
              </td>
            </tr>
            <tr v-if="!list.length">
              <td colspan="6" class="empty">还没有支出记录，点右上角记第一笔</td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>

    <div v-if="form.open" class="modal-mask" @click.self="closeForm">
      <div class="modal">
        <div class="modal-head">
          <span>{{ form.editId ? '编辑支出' : '记一笔支出' }}</span>
          <button class="btn ghost" @click="closeForm">✕</button>
        </div>
        <div class="modal-body">
          <div class="field">
            <label>类别<b>*</b></label>
            <div class="cat-pick">
              <button
                v-for="c in EXP_CATS"
                :key="c.key"
                type="button"
                class="cat-opt"
                :class="{ active: form.cat === c.key }"
                @click="form.cat = c.key"
              >
                <i :style="{ background: c.color }"></i>{{ c.label }}
              </button>
            </div>
          </div>
          <div class="field-row">
            <div class="field">
              <label>项目<b>*</b></label>
              <input v-model="form.name" placeholder="比如：煤气、摊位费、签子" />
            </div>
            <div class="field">
              <label>金额（元）<b>*</b></label>
              <input v-model="form.cost" type="number" step="0.5" min="0" placeholder="比如 30" />
            </div>
          </div>
          <div class="field">
            <label>日期<b>*</b></label>
            <input v-model="form.date" type="date" />
          </div>
          <div class="field">
            <label>备注</label>
            <textarea v-model="form.note" rows="2" placeholder="比如：请了一个阿姨帮忙穿串"></textarea>
          </div>
        </div>
        <div class="modal-foot">
          <button class="btn" @click="closeForm">取消</button>
          <button class="btn primary" @click="save">保存</button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, computed } from 'vue'
import { store, addItem, updateItem, removeItem, notify } from '../store'
import { money, num, todayStr } from '../stats'

const EXP_CATS = [
  { key: 'labor', label: '人工', color: '#8b5a2b' },
  { key: 'gas', label: '煤气/炭火', color: '#bd3f33' },
  { key: 'rent', label: '摊位费', color: '#dd8f2c' },
  { key: 'pack', label: '包装材料', color: '#2f7d4f' },
  { key: 'util', label: '水电杂费', color: '#55655c' },
  { key: 'other', label: '其他', color: '#7b8a81' }
]

const list = computed(() =>
  store.expenses
    .slice()
    .sort((a, b) => (b.date || '').localeCompare(a.date || '') || b.id - a.id)
)

const totalCost = computed(() => list.value.reduce((s, r) => s + num(r.cost), 0))

const monthCost = computed(() => {
  const now = new Date()
  const ym = now.getFullYear() + '-' + String(now.getMonth() + 1).padStart(2, '0')
  return list.value.filter((r) => (r.date || '').startsWith(ym)).reduce((s, r) => s + num(r.cost), 0)
})

const dailyAvg = computed(() => {
  if (!list.value.length) return 0
  const days = Math.max(1, list.value.length)
  return totalCost.value / days
})

const byCat = computed(() => {
  const map = {}
  EXP_CATS.forEach((c) => {
    const rs = list.value.filter((r) => r.cat === c.key)
    map[c.key] = {
      key: c.key,
      label: c.label,
      color: c.color,
      count: rs.length,
      cost: rs.reduce((s, r) => s + num(r.cost), 0)
    }
  })
  return Object.values(map).filter((c) => c.count > 0)
})

function colorOf(key) {
  const c = EXP_CATS.find((x) => x.key === key)
  return c ? c.color : '#7b8a81'
}

function labelOf(key) {
  const c = EXP_CATS.find((x) => x.key === key)
  return c ? c.label : key
}

const form = reactive({
  open: false,
  editId: null,
  cat: 'labor',
  name: '',
  cost: '',
  date: todayStr(),
  note: ''
})

function openForm(row) {
  if (row) {
    form.editId = row.id
    form.cat = row.cat
    form.name = row.name
    form.cost = row.cost
    form.date = row.date
    form.note = row.note || ''
  } else {
    form.editId = null
    form.cat = 'labor'
    form.name = ''
    form.cost = ''
    form.date = todayStr()
    form.note = ''
  }
  form.open = true
}

function closeForm() {
  form.open = false
}

async function save() {
  if (!form.name.trim()) {
    notify('项目要填')
    return
  }
  if (!num(form.cost)) {
    notify('金额要填')
    return
  }
  const payload = {
    cat: form.cat,
    name: form.name,
    cost: num(form.cost),
    date: form.date,
    note: form.note
  }
  try {
    if (form.editId) {
      await updateItem('expenses', form.editId, payload)
    } else {
      await addItem('expenses', payload)
    }
    form.open = false
  } catch (e) {
    notify(e.message)
  }
}

async function remove(row) {
  if (!confirm('删除这笔支出？')) return
  try {
    await removeItem('expenses', row.id)
  } catch (e) {
    notify(e.message)
  }
}
</script>

<style scoped>
.cat-sum {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(160px, 1fr));
  gap: 10px;
  padding: 0 20px 16px;
}

.cat-sum-item {
  border: 1px solid var(--line);
  border-radius: 10px;
  padding: 10px 12px;
  background: #fbfcfb;
}

.cat-sum-name {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 13px;
  font-weight: 600;
  color: var(--ink);
}

.cat-sum-name i {
  width: 8px;
  height: 8px;
  border-radius: 50%;
}

.cat-sum-num {
  font-size: 12px;
  color: var(--sub);
  margin-top: 2px;
}

.cat-pick {
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
}

.cat-opt {
  border: 1px solid var(--line);
  background: #fff;
  border-radius: 8px;
  padding: 7px 14px;
  font-size: 13px;
  color: var(--sub);
  display: inline-flex;
  align-items: center;
  gap: 6px;
}

.cat-opt.active {
  border-color: var(--brand);
  background: var(--brand-soft);
  color: var(--brand-dark);
  font-weight: 600;
}

.cat-opt i {
  width: 7px;
  height: 7px;
  border-radius: 50%;
}
</style>
