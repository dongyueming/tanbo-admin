<template>
  <div>
    <div class="page-head">
      <div>
        <h1>剥叶穿串</h1>
        <p>剥了几片叶、穿出几串，记在这儿；肉和丸子直接记串数，底料按份记</p>
      </div>
      <div class="head-actions">
        <button class="btn primary" @click="openForm()">+ 记一次穿串</button>
      </div>
    </div>

    <div class="grid kpi-grid" style="margin-bottom: 14px">
      <div class="kpi">
        <div class="kpi-label">累计出品</div>
        <div class="kpi-value">{{ sum.produced }}<span class="unit">串</span></div>
        <div class="kpi-note">来自 {{ store.strands.length }} 次穿串记录</div>
      </div>
      <div class="kpi">
        <div class="kpi-label">平均出品率</div>
        <div class="kpi-value">{{ money(sum.yieldPerJin, 2) }}<span class="unit">串/单位</span></div>
        <div class="kpi-note">平均一个进货单位出几串</div>
      </div>
      <div class="kpi">
        <div class="kpi-label">平均单串成本</div>
        <div class="kpi-value">{{ money(sum.unitCost) }}<span class="unit">元</span></div>
        <div class="kpi-note">定价至少要高于这个数</div>
      </div>
    </div>

    <div class="card">
      <div class="table-wrap">
        <table>
          <thead>
            <tr>
              <th>日期</th>
              <th>批次</th>
              <th>剥叶片数</th>
              <th>出品串数</th>
              <th>每串用几片</th>
              <th>单片成本</th>
              <th>单串成本</th>
              <th>单串利润</th>
              <th>操作人</th>
              <th>备注</th>
              <th style="text-align: right">操作</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="r in list" :key="r.id">
              <td class="num">{{ r.date }}</td>
              <td>{{ r.batchLabel }}</td>
              <td class="num">{{ r.leaves || '-' }}</td>
              <td class="num"><b>{{ r.strands }}</b></td>
              <td class="num">{{ r.batch ? money(r.batch.leavesPerStrand, 2) : '-' }} 片</td>
              <td class="num">{{ r.batch ? money(r.batch.leafCost) : '-' }} 元</td>
              <td class="num">{{ r.batch ? money(r.batch.unitCost) : '-' }} 元</td>
              <td class="num" :style="{ color: r.batch && r.batch.unitProfitDirect >= 0 ? 'var(--brand-dark)' : 'var(--danger)', fontWeight: 600 }">
                {{ r.batch && r.batch.avgPrice > 0 ? money(r.batch.unitProfitDirect) + ' 元' : '-' }}
              </td>
              <td>{{ r.operator || '-' }}</td>
              <td style="color: #7b8a81">{{ r.note || '-' }}</td>
              <td style="text-align: right">
                <button class="btn ghost" @click="openForm(r)">编辑</button>
                <button class="btn ghost" style="color: var(--danger)" @click="remove(r)">删除</button>
              </td>
            </tr>
            <tr v-if="!list.length">
              <td colspan="10" class="empty">还没有穿串记录</td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>

    <div v-if="form.open" class="modal-mask" @click.self="closeForm">
      <div class="modal">
        <div class="modal-head">
          <span>{{ form.editId ? '编辑穿串记录' : '记一次穿串' }}</span>
          <button class="btn ghost" @click="closeForm">✕</button>
        </div>
        <div class="modal-body">
          <div class="field">
            <label>属于哪一批<b>*</b></label>
            <select v-model="form.batchId">
              <option v-for="b in store.batches" :key="b.id" :value="b.id">
                第 {{ b.id }} 批 · {{ b.name }} · {{ b.buyDate }}（{{ money(b.weight, 1) }} {{ b.unit || '斤' }}）
              </option>
            </select>
            <span class="hint" v-if="!store.batches.length" style="color: var(--danger)">
              还没有批次，先去「进货台账」记进货
            </span>
          </div>
          <div class="field-row">
            <div class="field">
              <label>这批一共用了几片叶</label>
              <input v-model="form.leaves" type="number" step="1" min="0" placeholder="比如 300" />
              <span class="hint">填了就能算出单片成本、每串用几片；不填也不影响单串成本和利润</span>
            </div>
            <div class="field">
              <label>穿出几串<b>*</b></label>
              <input v-model="form.strands" type="number" step="1" min="1" placeholder="比如 30" />
            </div>
          </div>
          <div class="field-row">
            <div class="field">
              <label>日期<b>*</b></label>
              <input v-model="form.date" type="date" />
            </div>
            <div class="field">
              <label>操作人</label>
              <input v-model="form.operator" placeholder="谁穿的" />
            </div>
          </div>
          <div class="field">
            <label>备注</label>
            <textarea v-model="form.note" rows="2" placeholder="比如：这天叶小，出的串偏少"></textarea>
          </div>
          <div class="formula" v-if="preview.cost > 0 && num(form.strands) > 0">
            原料成本 {{ money(preview.cost) }} 元 · 出品 {{ num(form.strands) }} 串<br />
            单串成本 = {{ money(preview.cost) }} ÷ {{ num(form.strands) }} = <b>{{ money(preview.cost / num(form.strands)) }}</b> 元/串
            <template v-if="num(form.leaves) > 0">
              <br />单片成本 = {{ money(preview.cost) }} ÷ {{ num(form.leaves) }} = <b>{{ money(preview.cost / num(form.leaves)) }}</b> 元/片
              <br />每串用 <b>{{ money(num(form.leaves) / num(form.strands), 2) }}</b> 片
            </template>
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

const rows = computed(() => batchRows(store))
const sum = computed(() => totals(rows.value))

const list = computed(() =>
  store.strands
    .slice()
    .sort((a, b) => (b.date || '').localeCompare(a.date || '') || b.id - a.id)
    .map((s) => {
      const batch = rows.value.find((b) => b.id === s.batchId)
      return {
        ...s,
        batch,
        batchLabel: batch ? '第 ' + batch.id + ' 批' : '未关联'
      }
    })
)

const form = reactive({
  open: false,
  editId: null,
  batchId: null,
  leaves: '',
  strands: '',
  date: todayStr(),
  operator: '',
  note: ''
})

const preview = computed(() => {
  const batch = store.batches.find((b) => b.id === form.batchId)
  return { cost: batch ? num(batch.weight) * num(batch.pricePerJin) : 0 }
})

function openForm(row) {
  if (!store.batches.length && !row) {
    notify('请先在「进货台账」里记一批进货')
    return
  }
  if (row) {
    form.editId = row.id
    form.batchId = row.batchId
    form.leaves = row.leaves
    form.strands = row.strands
    form.date = row.date
    form.operator = row.operator || ''
    form.note = row.note || ''
  } else {
    form.editId = null
    form.batchId = store.batches[store.batches.length - 1].id
    form.leaves = ''
    form.strands = ''
    form.date = todayStr()
    form.operator = ''
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
    notify('出品串数要填')
    return
  }
  const payload = {
    batchId: num(form.batchId),
    leaves: num(form.leaves),
    strands: num(form.strands),
    date: form.date,
    operator: form.operator,
    note: form.note
  }
  try {
    if (form.editId) await updateItem('strands', form.editId, payload)
    else await addItem('strands', payload)
    form.open = false
  } catch (e) {
    notify(e.message)
  }
}

async function remove(row) {
  if (!confirm('删除这条穿串记录？')) return
  try {
    await removeItem('strands', row.id)
  } catch (e) {
    notify(e.message)
  }
}
</script>
