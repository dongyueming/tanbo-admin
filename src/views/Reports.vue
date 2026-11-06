<template>
  <div>
    <div class="page-head">
      <div>
        <h1>利润报表</h1>
        <p>按批次看谁赚钱、谁白干，找出最该改的那一环</p>
      </div>
      <div class="head-actions">
        <span class="tag" :class="sum.profit >= 0 ? 'green' : 'red'">
          总毛利 {{ money(sum.profit) }} 元
        </span>
      </div>
    </div>

    <div class="grid kpi-grid" style="margin-bottom: 14px">
      <div class="kpi"><div class="kpi-label">总收入</div><div class="kpi-value">{{ money(sum.revenue) }}<span class="unit">元</span></div></div>
      <div class="kpi"><div class="kpi-label">原料成本</div><div class="kpi-value">{{ money(sum.cost) }}<span class="unit">元</span></div></div>
      <div class="kpi" :class="sum.profit >= 0 ? 'good' : 'bad'">
        <div class="kpi-label">卖串毛利</div>
        <div class="kpi-value">{{ money(sum.profit) }}<span class="unit">元</span></div>
      </div>
      <div class="kpi"><div class="kpi-label">毛利率</div><div class="kpi-value">{{ money(sum.margin, 1) }}<span class="unit">%</span></div></div>
    </div>

    <div class="card card-pad" style="margin-bottom: 14px; background: #fef6e8; border-color: #f0d9a8">
      <div class="card-title" style="color: #8a6d3b">酱料成本（单独列示）</div>
      <div class="sum-line">
        <div>配汤 <b>{{ money(sauceCost.soup) }}</b> 元</div>
        <div>秘制酱 <b>{{ money(sauceCost.secret) }}</b> 元</div>
        <div>辣酱 <b>{{ money(sauceCost.chili) }}</b> 元</div>
        <div>甜酱 <b>{{ money(sauceCost.sweet) }}</b> 元</div>
        <div style="border-left: 2px solid #dd8f2c; padding-left: 12px">酱料合计 <b>{{ money(sauceCost.total) }}</b> 元</div>
        <div style="border-left: 2px solid #bd3f33; padding-left: 12px">含酱料总成本 <b>{{ money(sum.cost + sauceCost.total) }}</b> 元</div>
      </div>
    </div>

    <div class="card card-pad" style="margin-bottom: 14px">
      <div class="card-title">收入构成 <small>卖串的钱怎么分：成本 vs 赚到的</small></div>
      <Chart v-if="sum.revenue > 0" :option="pieOption" height="280px" />
      <div v-else class="empty">还没有收入</div>
    </div>

    <div class="two-col" style="margin-bottom: 14px">
      <div class="card card-pad">
        <div class="card-title">各批次 收入 / 成本 <small>柱子比出来谁赚得多</small></div>
        <Chart v-if="rows.length" :option="barOption" height="300px" />
        <div v-else class="empty">还没有批次</div>
      </div>
      <div class="card card-pad">
        <div class="card-title">出品率对比 <small>一斤能穿几串</small></div>
        <Chart v-if="rows.length" :option="yieldOption" height="300px" />
        <div v-else class="empty">还没有穿串记录</div>
      </div>
    </div>

    <div class="two-col" style="margin-bottom: 14px">
      <div class="card card-pad">
        <div class="card-title">各品类利润分割 <small>哪个品类最赚钱</small></div>
        <Chart v-if="cats.length" :option="profitPieOption" height="300px" />
        <div v-else class="empty">还没有数据</div>
      </div>
      <div class="card card-pad">
        <div class="card-title">酱料成本占比 <small>配汤/秘制酱/辣酱/甜酱花了多少钱</small></div>
        <Chart v-if="sauceCats.length" :option="saucePieOption" height="300px" />
        <div v-else class="empty">还没有酱料记录</div>
      </div>
    </div>

    <div class="two-col" style="margin-bottom: 14px">
      <div class="card card-pad">
        <div class="card-title">单串成本对比 <small>每个品类一串成本多少钱</small></div>
        <Chart v-if="cats.length" :option="unitCostOption" height="300px" />
        <div v-else class="empty">还没有数据</div>
      </div>
      <div class="card card-pad">
        <div class="card-title">损耗成本对比 <small>哪个品类损耗最亏</small></div>
        <Chart v-if="lossCats.length" :option="lossOption" height="300px" />
        <div v-else class="empty">还没有报损记录</div>
      </div>
    </div>

    <div class="card" style="margin-bottom: 14px">
      <div class="card-title" style="padding: 16px 20px 8px">各品类盈亏 <small>哪个品类赚钱、哪个在赔</small></div>
      <div class="table-wrap">
        <table>
          <thead>
            <tr>
              <th>品类</th>
              <th>批次</th>
              <th>进货成本</th>
              <th>成本占比</th>
              <th>收入</th>
              <th>毛利</th>
              <th>毛利率</th>
              <th>出品</th>
              <th>已售</th>
              <th>剩余</th>
              <th>单串成本</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="g in cats" :key="g.key">
              <td>
                <span class="tag" :style="{ background: g.color + '18', color: g.color }">{{ g.label }}</span>
              </td>
              <td class="num">{{ g.count }}</td>
              <td class="num">{{ money(g.cost) }}</td>
              <td class="num">{{ money(g.costShare, 1) }}%</td>
              <td class="num">{{ money(g.revenue) }}</td>
              <td class="num" :style="{ color: g.profit >= 0 ? 'var(--brand-dark)' : 'var(--danger)', fontWeight: 600 }">{{ money(g.profit) }}</td>
              <td class="num">{{ money(g.margin, 1) }}%</td>
              <td class="num">{{ g.produced }}</td>
              <td class="num">{{ g.sold }}</td>
              <td class="num">{{ g.stock }}</td>
              <td class="num">{{ money(g.unitCost) }}</td>
            </tr>
            <tr v-if="!cats.length">
              <td colspan="11" class="empty">还没有进货记录</td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>

    <div class="card">
      <div class="card-title" style="padding: 16px 20px 8px">批次毛利明细</div>
      <div class="table-wrap">
        <table>
          <thead>
            <tr>
              <th>批次</th>
              <th>品类</th>
              <th>数量</th>
              <th>成本</th>
              <th>出品</th>
              <th>售出</th>
              <th>剩余</th>
              <th>收入</th>
              <th>毛利</th>
              <th>毛利率</th>
              <th>出品率</th>
              <th>单片成本</th>
              <th>每串片数</th>
              <th>单串成本</th>
              <th>单串利润</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="r in rows" :key="r.id">
              <td>第 {{ r.id }} 批</td>
              <td>
                <span class="tag" :style="{ background: catColor(r.category) + '18', color: catColor(r.category) }">{{ r.categoryLabel }}</span>
              </td>
              <td class="num">{{ money(r.weight, 1) }} {{ r.unit }}</td>
              <td class="num">{{ money(r.cost) }}</td>
              <td class="num">{{ r.produced }}</td>
              <td class="num">{{ r.sold }}</td>
              <td class="num">{{ r.stock }}</td>
              <td class="num">{{ money(r.revenue) }}</td>
              <td class="num" :style="{ color: r.profit >= 0 ? 'var(--brand-dark)' : 'var(--danger)', fontWeight: 600 }">
                {{ money(r.profit) }}
              </td>
              <td class="num">{{ money(r.margin, 1) }}%</td>
              <td class="num">{{ money(r.yieldPerJin, 2) }}</td>
              <td class="num">{{ r.leafCost > 0 ? money(r.leafCost, 3) : '-' }}</td>
              <td class="num">{{ r.leavesPerStrand > 0 ? money(r.leavesPerStrand, 2) : '-' }}</td>
              <td class="num">{{ money(r.unitCost) }}</td>
              <td class="num">{{ r.avgPrice > 0 ? money(r.unitProfitDirect) : '-' }}</td>
            </tr>
            <tr v-if="!rows.length">
              <td colspan="15" class="empty">还没有数据</td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>
  </div>
</template>

<script setup>
import { computed } from 'vue'
import Chart from '../components/Chart.vue'
import { store } from '../store'
import { CATEGORIES, batchRows, totals, categoryRows, money, num } from '../stats'

// 利润报表只展示五个进货品类的利润，不包含酱料成本
const SUPPLIER_KEYS = ['vegetable', 'meat', 'ball', 'sausage', 'base']
const SAUCE_KEYS = ['soup', 'secret', 'chili', 'sweet']

const rows = computed(() => batchRows(store).filter((r) => SUPPLIER_KEYS.includes(r.category)))
const sum = computed(() => totals(rows.value))
const cats = computed(() => categoryRows(rows.value))

// 酱料成本（从所有 batches 里过滤出四个酱料品类）
const sauceCats = computed(() => {
  const allRows = batchRows(store).filter((r) => SAUCE_KEYS.includes(r.category))
  return categoryRows(allRows)
})

// 酱料成本明细
const sauceCost = computed(() => {
  const allRows = batchRows(store).filter((r) => SAUCE_KEYS.includes(r.category))
  const byCat = (key) => allRows.filter((r) => r.category === key).reduce((s, r) => s + r.cost, 0)
  return {
    soup: byCat('soup'),
    secret: byCat('secret'),
    chili: byCat('chili'),
    sweet: byCat('sweet'),
    total: allRows.reduce((s, r) => s + r.cost, 0)
  }
})

// 损耗成本（从 losses 里按品类汇总）
const lossCats = computed(() => {
  const map = {}
  store.losses.forEach((l) => {
    const batch = store.batches.find((b) => b.id === l.batchId)
    if (!batch) return
    const cat = batch.category || 'vegetable'
    if (!map[cat]) {
      const c = CATEGORIES.find((x) => x.key === cat)
      map[cat] = { key: cat, label: c ? c.label : cat, color: c ? c.color : '#5f7169', cost: 0 }
    }
    // 损耗金额 = 损耗串数 × 单串成本
    const batchRow = batchRows(store).find((b) => b.id === l.batchId)
    const unitCost = batchRow ? batchRow.unitCost : 0
    map[cat].cost += num(l.strands) * unitCost
  })
  return Object.values(map).filter((g) => g.cost > 0)
})

function catColor(key) {
  const c = CATEGORIES.find((x) => x.key === key)
  return c ? c.color : '#5f7169'
}

const pieOption = computed(() => ({
  tooltip: { trigger: 'item', valueFormatter: (v) => money(v) + ' 元' },
  legend: { bottom: 0, itemWidth: 10, itemHeight: 10, textStyle: { color: '#5f7169', fontSize: 12 } },
  series: [
    {
      type: 'pie',
      radius: ['42%', '68%'],
      center: ['50%', '44%'],
      avoidLabelOverlap: true,
      itemStyle: { borderColor: '#fff', borderWidth: 2 },
      label: { show: false },
      data: [
        { name: '原料成本', value: Math.round(sum.value.cost * 100) / 100, itemStyle: { color: '#cddcd3' } },
        { name: '毛利', value: Math.round(Math.max(sum.value.profit, 0) * 100) / 100, itemStyle: { color: '#2f7d4f' } }
      ]
    }
  ]
}))

const barOption = computed(() => ({
  tooltip: { trigger: 'axis', axisPointer: { type: 'shadow' }, valueFormatter: (v) => money(v) + ' 元' },
  legend: { bottom: 0, itemWidth: 10, itemHeight: 10, textStyle: { color: '#5f7169', fontSize: 12 } },
  grid: { left: 4, right: 12, top: 14, bottom: 38, containLabel: true },
  xAxis: {
    type: 'category',
    data: rows.value.map((r) => '第' + r.id + '批'),
    axisLine: { lineStyle: { color: '#dbe5dd' } },
    axisTick: { show: false },
    axisLabel: { color: '#6b7c71', fontSize: 11 }
  },
  yAxis: {
    type: 'value',
    splitLine: { lineStyle: { color: '#eef3ef' } },
    axisLine: { show: false },
    axisLabel: { color: '#8b9a90', fontSize: 11 }
  },
  series: [
    { name: '收入', type: 'bar', barWidth: 14, data: rows.value.map((r) => Math.round(r.revenue * 100) / 100), itemStyle: { color: '#2f7d4f', borderRadius: [3, 3, 0, 0] } },
    { name: '成本', type: 'bar', barWidth: 14, data: rows.value.map((r) => Math.round(r.cost * 100) / 100), itemStyle: { color: '#cddcd3', borderRadius: [3, 3, 0, 0] } }
  ]
}))

const yieldOption = computed(() => ({
  tooltip: { trigger: 'axis', valueFormatter: (v) => money(v) + ' 串/斤' },
  grid: { left: 4, right: 12, top: 16, bottom: 38, containLabel: true },
  xAxis: {
    type: 'category',
    data: rows.value.map((r) => '第' + r.id + '批'),
    axisLine: { lineStyle: { color: '#dbe5dd' } },
    axisTick: { show: false },
    axisLabel: { color: '#6b7c71', fontSize: 11 }
  },
  yAxis: {
    type: 'value',
    splitLine: { lineStyle: { color: '#eef3ef' } },
    axisLine: { show: false },
    axisLabel: { color: '#8b9a90', fontSize: 11 }
  },
  series: [
    {
      type: 'bar',
      barWidth: 20,
      data: rows.value.map((r) => Math.round(r.yieldPerJin * 100) / 100),
      itemStyle: { color: '#dd8f2c', borderRadius: [4, 4, 0, 0] },
      markLine: {
        symbol: 'none',
        lineStyle: { color: '#bd3f33', type: 'dashed' },
        data: [{ type: 'average', name: '平均' }],
        label: { color: '#bd3f33', fontSize: 11, formatter: '平均 {c}' }
      }
    }
  ]
}))

// 各品类利润分割（环形图）
const profitPieOption = computed(() => ({
  tooltip: { trigger: 'item', valueFormatter: (v) => money(v) + ' 元' },
  legend: { bottom: 0, itemWidth: 10, itemHeight: 10, textStyle: { color: '#5f7169', fontSize: 12 } },
  series: [
    {
      type: 'pie',
      radius: ['40%', '66%'],
      center: ['50%', '44%'],
      itemStyle: { borderColor: '#fff', borderWidth: 2 },
      label: { show: true, formatter: '{b}\n{d}%', fontSize: 12, color: '#3c4c43' },
      data: cats.value.filter((g) => g.profit > 0).map((g) => ({
        name: g.label,
        value: Math.round(g.profit * 100) / 100,
        itemStyle: { color: g.color }
      }))
    }
  ]
}))

// 酱料成本占比（环形图）
const saucePieOption = computed(() => ({
  tooltip: { trigger: 'item', valueFormatter: (v) => money(v) + ' 元' },
  legend: { bottom: 0, itemWidth: 10, itemHeight: 10, textStyle: { color: '#5f7169', fontSize: 12 } },
  series: [
    {
      type: 'pie',
      radius: ['40%', '66%'],
      center: ['50%', '44%'],
      itemStyle: { borderColor: '#fff', borderWidth: 2 },
      label: { show: true, formatter: '{b}\n{d}%', fontSize: 12, color: '#3c4c43' },
      data: sauceCats.value.map((g) => ({
        name: g.label,
        value: Math.round(g.cost * 100) / 100,
        itemStyle: { color: g.color }
      }))
    }
  ]
}))

// 单串成本对比（横向柱状图）
const unitCostOption = computed(() => ({
  tooltip: { trigger: 'axis', axisPointer: { type: 'shadow' }, valueFormatter: (v) => money(v) + ' 元/串' },
  grid: { left: 4, right: 16, top: 10, bottom: 6, containLabel: true },
  xAxis: {
    type: 'value',
    splitLine: { lineStyle: { color: '#eef3ef' } },
    axisLine: { show: false },
    axisLabel: { color: '#8b9a90', fontSize: 11 }
  },
  yAxis: {
    type: 'category',
    data: cats.value.map((g) => g.label),
    inverse: true,
    axisLine: { lineStyle: { color: '#dbe5dd' } },
    axisTick: { show: false },
    axisLabel: { color: '#6b7c71', fontSize: 12 }
  },
  series: [
    {
      type: 'bar',
      barWidth: 16,
      data: cats.value.map((g) => ({
        value: Math.round(g.unitCost * 100) / 100,
        itemStyle: { color: g.color, borderRadius: 3 }
      }))
    }
  ]
}))

// 损耗成本对比（柱状图）
const lossOption = computed(() => ({
  tooltip: { trigger: 'axis', axisPointer: { type: 'shadow' }, valueFormatter: (v) => money(v) + ' 元' },
  grid: { left: 4, right: 12, top: 16, bottom: 30, containLabel: true },
  xAxis: {
    type: 'category',
    data: lossCats.value.map((g) => g.label),
    axisLine: { lineStyle: { color: '#dbe5dd' } },
    axisTick: { show: false },
    axisLabel: { color: '#6b7c71', fontSize: 12 }
  },
  yAxis: {
    type: 'value',
    splitLine: { lineStyle: { color: '#eef3ef' } },
    axisLine: { show: false },
    axisLabel: { color: '#8b9a90', fontSize: 11 }
  },
  series: [
    {
      type: 'bar',
      barWidth: 24,
      data: lossCats.value.map((g) => ({
        value: Math.round(g.cost * 100) / 100,
        itemStyle: { color: '#bd3f33', borderRadius: [4, 4, 0, 0] }
      }))
    }
  ]
}))
</script>
