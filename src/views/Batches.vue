<template>
  <div>
    <div class="page-head">
      <div>
        <h1>进货台账</h1>
        <p>蔬菜、肉、丸子、肠、底料，进一次货记一笔；成本、保质期、剩余数量都从这儿出</p>
      </div>
      <div class="head-actions">
        <button class="btn primary" @click="openForm()">+ 记一批进货</button>
      </div>
    </div>

    <div class="card card-pad" style="margin-bottom: 14px">
      <div class="cat-filter">
        <button class="cat-chip" :class="{ active: filter === 'all' }" @click="filter = 'all'">
          全部 <b>{{ rows.length }}</b>
        </button>
        <button
          v-for="c in supplierCats"
          :key="c.key"
          class="cat-chip"
          :class="{ active: filter === c.key }"
          @click="filter = c.key"
        >
          <i :style="{ background: c.color }"></i>{{ c.label }} <b>{{ countOf(c.key) }}</b>
        </button>
      </div>
    </div>

    <div class="card">
      <div class="table-wrap">
        <table>
          <thead>
            <tr>
              <th>批次</th>
              <th>品类</th>
              <th>品名</th>
              <th>数量</th>
              <th>每单位进价</th>
              <th>原料成本</th>
              <th>进货日</th>
              <th>保鲜到期</th>
              <th>出品 / 已售 / 剩余</th>
              <th>保鲜状态</th>
              <th style="text-align: right">操作</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="r in shown" :key="r.id">
              <td>第 {{ r.id }} 批</td>
              <td><span class="tag" :style="{ background: colorOf(r.category) + '18', color: colorOf(r.category) }">{{ r.categoryLabel }}</span></td>
              <td>{{ r.name }}</td>
              <td class="num">{{ money(r.weight, 1) }} {{ r.unit }}</td>
              <td class="num">{{ money(r.pricePerJin) }}</td>
              <td class="num">{{ money(r.cost) }} 元</td>
              <td class="num">{{ r.buyDate }}</td>
              <td class="num">{{ r.expireDate }}</td>
              <td class="num">{{ r.produced }} / {{ r.sold }} / {{ r.stock }}</td>
              <td><span class="tag" :class="r.shelfTone">{{ r.shelfText }}</span></td>
              <td style="text-align: right">
                <button class="btn ghost" @click="openForm(r)">编辑</button>
                <button class="btn ghost" style="color: var(--danger)" @click="remove(r)">删除</button>
              </td>
            </tr>
            <tr v-if="!shown.length">
              <td colspan="11" class="empty">还没有进货记录，点右上角记第一批</td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>

    <div v-if="rows.length" class="card card-pad" style="margin-top: 14px">
      <div class="card-title">合计 <small>不同单位不能相加，这里只汇总钱和串数</small></div>
      <div class="sum-line">
        <div>进货总成本 <b>{{ money(sum.cost) }}</b> 元</div>
        <div>出品总数 <b>{{ sum.produced }}</b> 串/份</div>
        <div>已售 <b>{{ sum.sold }}</b></div>
        <div>在手库存 <b>{{ sum.stock }}</b></div>
        <div>平均单串成本 <b>{{ money(sum.unitCost) }}</b> 元</div>
      </div>
      <div class="cat-sum">
        <div v-for="g in cats" :key="g.key" class="cat-sum-item">
          <div class="cat-sum-name"><i :style="{ background: g.color }"></i>{{ g.label }}</div>
          <div class="cat-sum-num">{{ money(g.cost) }} 元成本 · 出 {{ g.produced }} 串</div>
          <div class="cat-sum-num">占成本 {{ money(g.costShare, 1) }}%</div>
        </div>
      </div>
    </div>

    <div v-if="form.open" class="modal-mask" @click.self="closeForm">
      <div class="modal">
        <div class="modal-head">
          <span>{{ form.editId ? '编辑批次' : '记一批进货' }}</span>
          <button class="btn ghost" @click="closeForm">✕</button>
        </div>
        <div class="modal-body">
          <div class="field">
            <label>品类<b>*</b></label>
            <div class="cat-pick">
              <button
                v-for="c in supplierCats"
                :key="c.key"
                type="button"
                class="cat-opt"
                :class="{ active: form.category === c.key }"
                @click="pickCategory(c)"
              >
                <i :style="{ background: c.color }"></i>{{ c.label }}
              </button>
            </div>
          </div>
          <div class="field">
            <label>品名<b>*</b></label>
            <input v-model="form.name" :placeholder="namePlaceholder" />
            <span class="hint">写清楚是什么，比如「包菜」「牛肉」「鱼丸」「火腿肠」「牛油底料」</span>
          </div>
          <div class="field-row">
            <div class="field">
              <label>数量（{{ form.unit }}）<b>*</b></label>
              <input v-model="form.weight" type="number" step="0.1" min="0" :placeholder="qtyPlaceholder" />
            </div>
            <div class="field">
              <label>计价单位</label>
              <select v-model="form.unit">
                <option v-for="u in unitOptions" :key="u" :value="u">{{ u }}</option>
              </select>
            </div>
          </div>
          <div class="field-row">
            <div class="field">
              <label>每{{ form.unit }}进价（元）<b>*</b></label>
              <input v-model="form.pricePerJin" type="number" step="0.1" min="0" :placeholder="pricePlaceholder" />
            </div>
            <div class="field">
              <label>保鲜天数<b>*</b></label>
              <input v-model="form.shelfDays" type="number" step="1" min="1" />
            </div>
          </div>
          <div class="field-row">
            <div class="field">
              <label>进货日期<b>*</b></label>
              <input v-model="form.buyDate" type="date" />
            </div>
            <div class="field"></div>
          </div>
          <div class="field">
            <label>备注</label>
            <textarea v-model="form.note" rows="2" placeholder="比如：菜市场东头老李那儿拿的"></textarea>
          </div>
          <div class="formula" v-if="preview.cost > 0">
            这一批原料成本 = {{ money(form.weight, 1) }} {{ form.unit }} × {{ money(form.pricePerJin) }} 元 =
            <b>{{ money(preview.cost) }}</b> 元
            <br />
            预计到期日：<b>{{ preview.expire }}</b>
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
import { CATEGORIES, batchRows, totals, categoryRows, money, num, todayStr, addDays } from '../stats'

const UNIT_OPTIONS = ['斤', '棵', '包', '袋', '瓶', '份', '箱', '个', '根', '只']

// 进货台账只显示这五个品类，酱料类在单独页面里
const SUPPLIER_KEYS = ['vegetable', 'meat', 'ball', 'sausage', 'base']
const supplierCats = CATEGORIES.filter((c) => SUPPLIER_KEYS.includes(c.key))

const allRows = computed(() => batchRows(store))
const rows = computed(() => allRows.value.filter((r) => SUPPLIER_KEYS.includes(r.category)))
const sum = computed(() => totals(rows.value))
const cats = computed(() => categoryRows(allRows.value))

const filter = ref('all')
const shown = computed(() => (filter.value === 'all' ? rows.value : rows.value.filter((r) => r.category === filter.value)))

function countOf(key) {
  return rows.value.filter((r) => r.category === key).length
}

function colorOf(key) {
  const c = CATEGORIES.find((x) => x.key === key)
  return c ? c.color : '#5f7169'
}

const unitOptions = computed(() => {
  const u = form.unit
  return UNIT_OPTIONS.includes(u) ? UNIT_OPTIONS : [u, ...UNIT_OPTIONS]
})

const namePlaceholder = computed(() => {
  const map = { vegetable: '包菜', meat: '牛肉', ball: '鱼丸', sausage: '火腿肠', base: '牛油底料' }
  return map[form.category] || '品名'
})

const qtyPlaceholder = computed(() => {
  const map = { 斤: '比如 20.5', 棵: '比如 30', 包: '比如 5', 袋: '比如 3', 瓶: '比如 2', 份: '比如 40', 箱: '比如 2', 个: '比如 50', 根: '比如 60', 只: '比如 20' }
  return map[form.unit] || '数量'
})

const pricePlaceholder = computed(() => {
  const map = { 斤: '比如 1.8', 棵: '比如 1', 包: '比如 12', 袋: '比如 18', 瓶: '比如 9', 份: '比如 2.5', 箱: '比如 60', 个: '比如 1', 根: '比如 0.5', 只: '比如 3' }
  return map[form.unit] || '单价'
})

const form = reactive({
  open: false,
  editId: null,
  category: 'vegetable',
  name: '包菜',
  weight: '',
  unit: '斤',
  pricePerJin: '',
  buyDate: todayStr(),
  shelfDays: 4,
  note: ''
})

const preview = computed(() => ({
  cost: num(form.weight) * num(form.pricePerJin),
  expire: addDays(form.buyDate, form.shelfDays)
}))

function pickCategory(c) {
  form.category = c.key
  form.unit = c.unit
  form.shelfDays = c.shelfDays
  if (!form.editId) {
    const map = { vegetable: '包菜', meat: '牛肉', ball: '鱼丸', sausage: '火腿肠', base: '牛油底料' }
    form.name = map[c.key] || ''
  }
}

function openForm(row) {
  if (row) {
    form.editId = row.id
    form.category = row.category
    form.name = row.name
    form.weight = row.weight
    form.unit = row.unit
    form.pricePerJin = row.pricePerJin
    form.buyDate = row.buyDate
    form.shelfDays = row.shelfDays
    form.note = row.note || ''
  } else {
    form.editId = null
    form.category = 'vegetable'
    form.name = '包菜'
    form.weight = ''
    form.unit = '斤'
    form.pricePerJin = ''
    form.buyDate = todayStr()
    form.shelfDays = 4
    form.note = ''
  }
  form.open = true
}

function closeForm() {
  form.open = false
}

async function save() {
  if (!num(form.weight) || !num(form.pricePerJin)) {
    notify('数量（' + form.unit + '）和每' + form.unit + '进价都要填')
    return
  }
  const payload = {
    category: form.category,
    name: form.name || '未命名',
    weight: num(form.weight),
    unit: form.unit,
    pricePerJin: num(form.pricePerJin),
    buyDate: form.buyDate,
    shelfDays: num(form.shelfDays),
    note: form.note
  }
  try {
    if (form.editId) {
      await updateItem('batches', form.editId, payload)
    } else {
      await addItem('batches', payload)
    }
    form.open = false
  } catch (e) {
    notify(e.message)
  }
}

async function remove(row) {
  if (!confirm('删除第 ' + row.id + ' 批「' + row.name + '」？相关的穿串、销售记录不会被删除，但会失去关联。')) return
  try {
    await removeItem('batches', row.id)
  } catch (e) {
    notify(e.message)
  }
}
</script>

<style scoped>
.cat-filter {
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
}

.cat-chip {
  border: 1px solid var(--line);
  background: #fff;
  border-radius: 20px;
  padding: 6px 14px;
  font-size: 13px;
  color: var(--sub);
  display: inline-flex;
  align-items: center;
  gap: 6px;
}

.cat-chip.active {
  border-color: var(--brand);
  background: var(--brand-soft);
  color: var(--brand-dark);
  font-weight: 600;
}

.cat-chip b {
  font-weight: 600;
  color: var(--ink);
}

.cat-chip i {
  width: 7px;
  height: 7px;
  border-radius: 50%;
  display: inline-block;
}

.sum-line {
  display: flex;
  gap: 26px;
  flex-wrap: wrap;
  font-size: 13px;
  color: #3c4c43;
}

.cat-sum {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
  gap: 10px;
  margin-top: 14px;
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
