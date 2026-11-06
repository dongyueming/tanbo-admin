<template>
  <div>
    <div class="page-head">
      <div>
        <h1>损耗报损</h1>
        <p>烂掉、卖不掉、送人的串单独记，不然算出来的利润是虚的</p>
      </div>
      <div class="head-actions">
        <button class="btn primary" @click="openForm()">+ 记一次报损</button>
      </div>
    </div>

    <div class="grid kpi-grid" style="margin-bottom: 14px">
      <div class="kpi bad">
        <div class="kpi-label">累计报损</div>
        <div class="kpi-value">{{ sum.lost }}<span class="unit">串</span></div>
        <div class="kpi-note">共 {{ store.losses.length }} 次</div>
      </div>
      <div class="kpi" :class="lossRate > 10 ? 'warn' : 'good'">
        <div class="kpi-label">损耗率</div>
        <div class="kpi-value">{{ money(lossRate, 1) }}<span class="unit">%</span></div>
        <div class="kpi-note">报损串数 ÷ 出品串数</div>
      </div>
      <div class="kpi">
        <div class="kpi-label">折算金额</div>
        <div class="kpi-value">{{ money(lostValue) }}<span class="unit">元</span></div>
        <div class="kpi-note">按各批单串成本折算</div>
      </div>
    </div>

    <div class="card">
      <div class="table-wrap">
        <table>
          <thead>
            <tr>
              <th>日期</th>
              <th>批次</th>
              <th>报损串数</th>
              <th>原因</th>
              <th>折算金额</th>
              <th>备注</th>
              <th style="text-align: right">操作</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="r in list" :key="r.id">
              <td class="num">{{ r.date }}</td>
              <td>{{ r.batchLabel }}</td>
              <td class="num"><b style="color: var(--danger)">{{ r.strands }}</b></td>
              <td><span class="tag red">{{ r.reason || '其他' }}</span></td>
              <td class="num">{{ money(r.value) }} 元</td>
              <td style="color: #7b8a81">{{ r.note || '-' }}</td>
              <td style="text-align: right">
                <button class="btn ghost" @click="openForm(r)">编辑</button>
                <button class="btn ghost" style="color: var(--danger)" @click="remove(r)">删除</button>
              </td>
            </tr>
            <tr v-if="!list.length">
              <td colspan="7" class="empty">没有报损记录，说明这批菜卖得挺干净</td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>

    <div v-if="form.open" class="modal-mask" @click.self="closeForm">
      <div class="modal">
        <div class="modal-head">
          <span>{{ form.editId ? '编辑报损记录' : '记一次报损' }}</span>
          <button class="btn ghost" @click="closeForm">✕</button>
        </div>
        <div class="modal-body">
          <div class="field">
            <label>哪一批<b>*</b></label>
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
              <label>报损串数<b>*</b></label>
              <input v-model="form.strands" type="number" step="1" min="1" placeholder="比如 5" />
            </div>
            <div class="field">
              <label>日期<b>*</b></label>
              <input v-model="form.date" type="date" />
            </div>
          </div>
          <div class="field">
            <label>原因</label>
            <select v-model="form.reason">
              <option value="黄叶">黄叶 / 烂叶</option>
              <option value="临期">临期没卖掉</option>
              <option value="破损">破损 / 压坏</option>
              <option value="自用">自用 / 送人</option>
              <option value="其他">其他</option>
            </select>
          </div>
          <div class="field">
            <label>备注</label>
            <textarea v-model="form.note" rows="2" placeholder="比如：雨天人少，剩了 8 串"></textarea>
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
const lossRate = computed(() => (sum.value.produced > 0 ? sum.value.lost / sum.value.produced * 100 : 0))

const list = computed(() =>
  store.losses
    .slice()
    .sort((a, b) => (b.date || '').localeCompare(a.date || '') || b.id - a.id)
    .map((l) => {
      const batch = rows.value.find((b) => b.id === l.batchId)
      return {
        ...l,
        batch,
        batchLabel: batch ? '第 ' + batch.id + ' 批' : '未关联',
        value: batch ? num(l.strands) * batch.unitCost : 0
      }
    })
)

const lostValue = computed(() => list.value.reduce((a, r) => a + r.value, 0))

function stockOf(id) {
  const r = rows.value.find((b) => b.id === id)
  return r ? r.stock : 0
}

const form = reactive({
  open: false,
  editId: null,
  batchId: null,
  strands: '',
  date: todayStr(),
  reason: '黄叶',
  note: ''
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
    form.date = row.date
    form.reason = row.reason || '其他'
    form.note = row.note || ''
  } else {
    form.editId = null
    form.batchId = store.batches[store.batches.length - 1].id
    form.strands = ''
    form.date = todayStr()
    form.reason = '黄叶'
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
    notify('报损串数要填')
    return
  }
  const payload = {
    batchId: num(form.batchId),
    strands: num(form.strands),
    date: form.date,
    reason: form.reason,
    note: form.note
  }
  try {
    if (form.editId) await updateItem('losses', form.editId, payload)
    else await addItem('losses', payload)
    form.open = false
  } catch (e) {
    notify(e.message)
  }
}

async function remove(row) {
  if (!confirm('删除这条报损记录？')) return
  try {
    await removeItem('losses', row.id)
  } catch (e) {
    notify(e.message)
  }
}
</script>
