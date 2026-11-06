<template>
  <div>
    <div class="page-head">
      <div>
        <h1>销售记账</h1>
        <p>卖了多少串、一串多少钱，记一笔收入就到账</p>
      </div>
      <div class="head-actions">
        <button class="btn primary" @click="openForm()">+ 记一笔销售</button>
      </div>
    </div>

    <div class="grid kpi-grid" style="margin-bottom: 14px">
      <div class="kpi good">
        <div class="kpi-label">累计收入</div>
        <div class="kpi-value">{{ money(sum.revenue) }}<span class="unit">元</span></div>
        <div class="kpi-note">已售 {{ sum.sold }} 串</div>
      </div>
      <div class="kpi">
        <div class="kpi-label">平均售价</div>
        <div class="kpi-value">{{ money(avgPrice) }}<span class="unit">元/串</span></div>
        <div class="kpi-note">收入 ÷ 已售串数</div>
      </div>
      <div class="kpi" :class="sum.profit >= 0 ? 'good' : 'bad'">
        <div class="kpi-label">盘面毛利</div>
        <div class="kpi-value">{{ money(sum.profit) }}<span class="unit">元</span></div>
        <div class="kpi-note">收入 − 全部进货成本</div>
      </div>
    </div>

    <div class="card">
      <div class="table-wrap">
        <table>
          <thead>
            <tr>
              <th>日期</th>
              <th>批次</th>
              <th>售出串数</th>
              <th>每串售价</th>
              <th>收入金额</th>
              <th>渠道</th>
              <th>备注</th>
              <th style="text-align: right">操作</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="r in list" :key="r.id">
              <td class="num">{{ r.date }}</td>
              <td>{{ r.batchLabel }}</td>
              <td class="num">{{ r.strands }}</td>
              <td class="num">{{ money(r.pricePerStrand) }} 元</td>
              <td class="num"><b>{{ money(num(r.strands) * num(r.pricePerStrand)) }}</b> 元</td>
              <td><span class="tag" :class="r.channel === '批发' ? 'amber' : 'green'">{{ r.channel || '零售' }}</span></td>
              <td style="color: #7b8a81">{{ r.note || '-' }}</td>
              <td style="text-align: right">
                <button class="btn ghost" @click="openForm(r)">编辑</button>
                <button class="btn ghost" style="color: var(--danger)" @click="remove(r)">删除</button>
              </td>
            </tr>
            <tr v-if="!list.length">
              <td colspan="8" class="empty">还没有销售记录</td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>

    <div v-if="form.open" class="modal-mask" @click.self="closeForm">
      <div class="modal">
        <div class="modal-head">
          <span>{{ form.editId ? '编辑销售记录' : '记一笔销售' }}</span>
          <button class="btn ghost" @click="closeForm">✕</button>
        </div>
        <div class="modal-body">
          <div class="field">
            <label>卖的是哪一批<b>*</b></label>
            <select v-model="form.batchId">
              <option v-for="b in store.batches" :key="b.id" :value="b.id">
                第 {{ b.id }} 批 · {{ b.name }} · 在手 {{ stockOf(b.id) }} 串
              </option>
            </select>
            <span class="hint" v-if="!store.batches.length" style="color: var(--danger)">
              还没有批次，先去「进货台账」记进货
            </span>
          </div>
          <div class="field-row">
            <div class="field">
              <label>售出串数<b>*</b></label>
              <input v-model="form.strands" type="number" step="1" min="1" placeholder="比如 20" />
            </div>
            <div class="field">
              <label>每串售价（元）<b>*</b></label>
              <input v-model="form.pricePerStrand" type="number" step="0.1" min="0" placeholder="比如 2" />
            </div>
          </div>
          <div class="field-row">
            <div class="field">
              <label>日期<b>*</b></label>
              <input v-model="form.date" type="date" />
            </div>
            <div class="field">
              <label>渠道</label>
              <select v-model="form.channel">
                <option value="零售">零售</option>
                <option value="批发">批发</option>
                <option value="团购">团购</option>
                <option value="自用">自用 / 赠送</option>
              </select>
            </div>
          </div>
          <div class="field">
            <label>备注</label>
            <textarea v-model="form.note" rows="2" placeholder="比如：夜市那波客人买走的"></textarea>
          </div>
          <div class="formula" v-if="amount > 0">
            这笔收入 = {{ num(form.strands) }} 串 × {{ money(form.pricePerStrand) }} 元 =
            <b>{{ money(amount) }}</b> 元
            <div v-if="unitCostHint > 0" style="margin-top: 4px">
              这批单串成本 {{ money(unitCostHint) }} 元，这笔估算赚
              <b>{{ money(amount - num(form.strands) * unitCostHint) }}</b> 元
            </div>
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
import { reactive, computed } from 'vue'
import { store, addItem, updateItem, removeItem, notify } from '../store'
import { batchRows, totals, money, num, todayStr } from '../stats'

const rows = computed(() => batchRows(store))
const sum = computed(() => totals(rows.value))
const avgPrice = computed(() => (sum.value.sold > 0 ? sum.value.revenue / sum.value.sold : 0))

const list = computed(() =>
  store.sales
    .slice()
    .sort((a, b) => (b.date || '').localeCompare(a.date || '') || b.id - a.id)
    .map((s) => {
      const batch = rows.value.find((b) => b.id === s.batchId)
      return { ...s, batch, batchLabel: batch ? '第 ' + batch.id + ' 批' : '未关联' }
    })
)

function stockOf(id) {
  const r = rows.value.find((b) => b.id === id)
  return r ? r.stock : 0
}

const form = reactive({
  open: false,
  editId: null,
  batchId: null,
  strands: '',
  pricePerStrand: '',
  date: todayStr(),
  channel: '零售',
  note: ''
})

const amount = computed(() => num(form.strands) * num(form.pricePerStrand))
const unitCostHint = computed(() => {
  const r = rows.value.find((b) => b.id === form.batchId)
  return r ? r.unitCost : 0
})

function openForm(row) {
  if (!store.batches.length && !row) {
    notify('请先在「进货台账」里记一批进货')
    return
  }
  if (row) {
    form.editId = row.id
    form.batchId = row.batchId
    form.strands = row.strands
    form.pricePerStrand = row.pricePerStrand
    form.date = row.date
    form.channel = row.channel || '零售'
    form.note = row.note || ''
  } else {
    form.editId = null
    form.batchId = store.batches[store.batches.length - 1].id
    form.strands = ''
    form.pricePerStrand = ''
    form.date = todayStr()
    form.channel = '零售'
    form.note = ''
  }
  form.open = true
}

function closeForm() {
  form.open = false
}

async function save() {
  if (!form.batchId) {
    notify('要选一个批次')
    return
  }
  if (!num(form.strands)) {
    notify('售出串数要填')
    return
  }
  const payload = {
    batchId: num(form.batchId),
    strands: num(form.strands),
    pricePerStrand: num(form.pricePerStrand),
    date: form.date,
    channel: form.channel,
    note: form.note
  }
  try {
    if (form.editId) await updateItem('sales', form.editId, payload)
    else await addItem('sales', payload)
    form.open = false
  } catch (e) {
    notify(e.message)
  }
}

async function remove(row) {
  if (!confirm('删除这条销售记录？')) return
  try {
    await removeItem('sales', row.id)
  } catch (e) {
    notify(e.message)
  }
}
</script>
