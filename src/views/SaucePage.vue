<template>
  <div>
    <div class="page-head">
      <div>
        <h1>{{ title }}</h1>
        <p>{{ subtitle }}</p>
      </div>
      <div class="head-actions">
        <button class="btn primary" @click="openForm()">+ 记一批</button>
      </div>
    </div>

    <div class="grid kpi-grid" style="margin-bottom: 14px">
      <div class="kpi">
        <div class="kpi-label">累计批次</div>
        <div class="kpi-value">{{ rows.length }}<span class="unit">批</span></div>
      </div>
      <div class="kpi">
        <div class="kpi-label">总成本</div>
        <div class="kpi-value">{{ money(sum.cost) }}<span class="unit">元</span></div>
      </div>
      <div class="kpi">
        <div class="kpi-label">平均单斤成本</div>
        <div class="kpi-value">{{ avgUnitCost }}<span class="unit">元/斤</span></div>
      </div>
    </div>

    <div class="card">
      <div class="table-wrap">
        <table>
          <thead>
            <tr>
              <th>批次</th>
              <th>品名</th>
              <th>产出</th>
              <th>原料数</th>
              <th>总成本</th>
              <th>单斤成本</th>
              <th>日期</th>
              <th style="text-align: right">操作</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="r in rows" :key="r.id">
              <td>第 {{ r.id }} 批</td>
              <td>{{ r.name }}</td>
              <td class="num">{{ money(r.weight, 1) }} {{ r.unit }}</td>
              <td class="num">{{ ingredientCount(r) }} 种</td>
              <td class="num">{{ money(r.cost) }} 元</td>
              <td class="num">{{ r.weight > 0 ? (r.cost / r.weight).toFixed(2) : '-' }}</td>
              <td class="num">{{ r.buyDate }}</td>
              <td style="text-align: right">
                <button class="btn ghost" @click="openForm(r)">编辑</button>
                <button class="btn ghost" style="color: var(--danger)" @click="remove(r)">删除</button>
              </td>
            </tr>
            <tr v-if="!rows.length">
              <td colspan="8" class="empty">还没有记录，点右上角记第一批</td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>

    <div v-if="form.open" class="modal-mask" @click.self="closeForm">
      <div class="modal" style="max-width: 680px">
        <div class="modal-head">
          <span>{{ form.editId ? '编辑' : '记一批' }}</span>
          <button class="btn ghost" @click="closeForm">✕</button>
        </div>
        <div class="modal-body">
          <div class="field-row">
            <div class="field">
              <label>品名<b>*</b></label>
              <input v-model="form.name" :placeholder="namePlaceholder" />
            </div>
            <div class="field">
              <label>日期<b>*</b></label>
              <input v-model="form.buyDate" type="date" />
            </div>
          </div>

          <div class="field">
            <label>原料明细 <small style="color:#7b8a81">（选原料、填用量和单价，自动算成本）</small></label>
            <div class="ing-table">
              <div class="ing-head">
                <span>原料名称</span>
                <span>用量</span>
                <span>单位</span>
                <span>单价（元）</span>
                <span>成本（元）</span>
                <span></span>
              </div>
              <div v-for="(row, i) in form.ingredients" :key="i" class="ing-row">
                <select v-model="row.name" @change="onIngNameChange(row)">
                  <option value="">选原料</option>
                  <option v-for="ing in availableIngredients" :key="ing" :value="ing">{{ ing }}</option>
                </select>
                <input v-model="row.qty" type="number" step="0.1" min="0" placeholder="0" />
                <select v-model="row.unit">
                  <option value="g">g</option>
                  <option value="斤">斤</option>
                  <option value="包">包</option>
                  <option value="袋">袋</option>
                  <option value="根">根</option>
                  <option value="个">个</option>
                  <option value="把">把</option>
                  <option value="桶">桶</option>
                </select>
                <input v-model="row.price" type="number" step="0.01" min="0" placeholder="0.00" />
                <span class="ing-cost">{{ ingCost(row).toFixed(2) }}</span>
                <button class="btn ghost" style="color: var(--danger); padding: 2px 8px" @click="removeIng(i)">✕</button>
              </div>
              <button class="btn ghost" style="margin-top: 8px" @click="addIng">+ 添加一行</button>
            </div>
          </div>

          <div class="field-row">
            <div class="field">
              <label>产出（{{ form.unit }}）<b>*</b></label>
              <input v-model="form.weight" type="number" step="0.1" min="0" placeholder="比如 25" />
            </div>
            <div class="field">
              <label>计价单位</label>
              <select v-model="form.unit">
                <option value="斤">斤</option>
                <option value="份">份</option>
                <option value="锅">锅</option>
              </select>
            </div>
          </div>

          <div class="field">
            <label>备注</label>
            <textarea v-model="form.note" rows="2" placeholder="比如：牛骨黑了，5天换一次"></textarea>
          </div>

          <div class="formula">
            <div>原料总成本 = <b>{{ totalCost.toFixed(2) }}</b> 元</div>
            <div v-if="num(form.weight) > 0">单{{ form.unit }}成本 = <b>{{ (totalCost / num(form.weight)).toFixed(2) }}</b> 元/{{ form.unit }}</div>
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
import { batchRows, totals, money, num, todayStr } from '../stats'

const props = defineProps({
  category: { type: String, required: true },
  title: { type: String, required: true },
  subtitle: { type: String, default: '' },
  namePlaceholder: { type: String, default: '品名' },
  ingredientList: { type: Array, default: () => [] }
})

const rows = computed(() => batchRows(store).filter((r) => r.category === props.category))
const sum = computed(() => totals(rows.value))

const avgUnitCost = computed(() => {
  const totalWeight = rows.value.reduce((s, r) => s + r.weight, 0)
  return totalWeight > 0 ? (sum.value.cost / totalWeight).toFixed(2) : '0.00'
})

function ingredientCount(r) {
  if (r.ingredients && Array.isArray(r.ingredients)) return r.ingredients.length
  return 0
}

const form = reactive({
  open: false,
  editId: null,
  name: '',
  weight: '',
  unit: '斤',
  buyDate: todayStr(),
  note: '',
  ingredients: []
})

const totalCost = computed(() => {
  return form.ingredients.reduce((s, r) => s + ingCost(r), 0)
})

function ingCost(row) {
  return num(row.qty) * num(row.price)
}

const availableIngredients = computed(() => {
  const used = form.ingredients.map((r) => r.name)
  return props.ingredientList.filter((n) => !used.includes(n))
})

function onIngNameChange(row) {
  // 选了原料后，可以自动带出默认单位（可选）
}

function addIng() {
  form.ingredients.push({ name: '', qty: '', unit: 'g', price: '' })
}

function removeIng(i) {
  form.ingredients.splice(i, 1)
}

function openForm(row) {
  if (row) {
    form.editId = row.id
    form.name = row.name
    form.weight = row.weight
    form.unit = row.unit
    form.buyDate = row.buyDate
    form.note = row.note || ''
    form.ingredients = row.ingredients && Array.isArray(row.ingredients) ? row.ingredients.map((r) => ({ ...r })) : []
  } else {
    form.editId = null
    form.name = ''
    form.weight = ''
    form.unit = '斤'
    form.buyDate = todayStr()
    form.note = ''
    form.ingredients = []
  }
  form.open = true
}

function closeForm() {
  form.open = false
}

async function save() {
  if (!form.name.trim()) {
    notify('品名要填')
    return
  }
  if (!num(form.weight)) {
    notify('产出数量要填')
    return
  }
  const validIngs = form.ingredients.filter((r) => r.name && num(r.qty) > 0 && num(r.price) > 0)
  if (!validIngs.length) {
    notify('至少填一行原料明细（名称+用量+单价）')
    return
  }
  const cost = totalCost.value
  const pricePerJin = cost / num(form.weight)
  const payload = {
    category: props.category,
    name: form.name,
    weight: num(form.weight),
    unit: form.unit,
    pricePerJin: Number(pricePerJin.toFixed(4)),
    cost: Number(cost.toFixed(2)),
    buyDate: form.buyDate,
    shelfDays: 30,
    note: form.note,
    ingredients: validIngs.map((r) => ({
      name: r.name,
      qty: num(r.qty),
      unit: r.unit,
      price: num(r.price),
      cost: Number(ingCost(r).toFixed(2))
    }))
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
  if (!confirm('删除这条记录？')) return
  try {
    await removeItem('batches', row.id)
  } catch (e) {
    notify(e.message)
  }
}
</script>

<style scoped>
.ing-table {
  border: 1px solid var(--line);
  border-radius: 8px;
  overflow: hidden;
}

.ing-head, .ing-row {
  display: grid;
  grid-template-columns: 1.2fr 0.8fr 0.6fr 0.8fr 0.8fr 32px;
  gap: 6px;
  padding: 6px 8px;
  align-items: center;
}

.ing-head {
  background: #f5f7f6;
  font-size: 12px;
  color: var(--sub);
  font-weight: 600;
}

.ing-row {
  border-top: 1px solid var(--line);
  font-size: 13px;
}

.ing-row input, .ing-row select {
  width: 100%;
  padding: 5px 6px;
  border: 1px solid var(--line);
  border-radius: 6px;
  font-size: 13px;
}

.ing-cost {
  text-align: right;
  font-weight: 600;
  color: var(--ink);
}

.formula {
  margin-top: 12px;
  padding: 10px 12px;
  background: var(--brand-soft);
  border-radius: 8px;
  font-size: 13px;
  color: var(--brand-dark);
  line-height: 1.8;
}

.formula b {
  font-size: 15px;
}
</style>
