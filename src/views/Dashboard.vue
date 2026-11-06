<template>
  <div>
    <div class="page-head">
      <div>
        <h1>经营仪表盘</h1>
        <p>今天 {{ todayLabel }} · 一屏看完今天赚了多少、还剩多少串</p>
      </div>
      <div class="head-actions">
        <span class="tag gray">{{ store.batches.length }} 个批次</span>
      </div>
    </div>

    <div v-if="risk.length" class="notice">
      <b>临期提醒：</b>
      <span v-for="(r, i) in risk" :key="r.id">
        {{ i ? '、' : '' }}第 {{ r.id }} 批（{{ r.name }}）还剩 {{ r.stock }} 串，{{ r.shelfText }}
      </span>
      —— 建议今天先降价清掉，别烂在手里。
    </div>

    <div class="grid kpi-grid" style="margin-bottom: 14px">
      <div class="kpi good">
        <div class="kpi-label">今日收入</div>
        <div class="kpi-value">{{ money(today.revenue) }}<span class="unit">元</span></div>
        <div class="kpi-note">卖出 {{ today.sold }} 串</div>
      </div>
      <div class="kpi" :class="today.profit >= 0 ? 'good' : 'bad'">
        <div class="kpi-label">今日毛利</div>
        <div class="kpi-value">{{ money(today.profit) }}<span class="unit">元</span></div>
        <div class="kpi-note">今日进货成本 {{ money(today.cost) }} 元</div>
      </div>
      <div class="kpi">
        <div class="kpi-label">在手库存</div>
        <div class="kpi-value">{{ today.stock }}<span class="unit">串</span></div>
        <div class="kpi-note">未售出且未报损</div>
      </div>
      <div class="kpi" :class="sum.profit >= 0 ? 'good' : 'bad'">
        <div class="kpi-label">累计毛利</div>
        <div class="kpi-value">{{ money(sum.profit) }}<span class="unit">元</span></div>
        <div class="kpi-note">毛利率 {{ money(sum.margin, 1) }}%</div>
      </div>
      <div class="kpi">
        <div class="kpi-label">平均单串成本</div>
        <div class="kpi-value">{{ money(sum.unitCost) }}<span class="unit">元</span></div>
        <div class="kpi-note">总成本 {{ money(sum.cost) }} 元</div>
      </div>
    </div>

    <div v-if="!store.batches.length" class="card card-pad" style="margin-bottom: 14px">
      <div class="card-title">还没开始记账</div>
      <p style="color: #5f7169; font-size: 13px; margin: 6px 0 0; line-height: 1.8">
        先到「进货台账」记一笔进货（什么品类、多少斤/包、多少钱、保质几天），再去「剥叶穿串」记这批货出了几串，
        卖完到「销售记账」记收入，系统就自动把利润算出来了。底料不穿串的，按份记也行。
      </p>
    </div>

    <div class="card card-pad" style="margin-bottom: 14px">
      <div class="card-title">今日汇总 <small>一屏看完今天干了什么</small></div>
      <div class="sum-line">
        <div>今日进货 <b>{{ money(todayCost) }}</b> 元 · {{ todayBatches }} 批</div>
        <div>今日穿串 <b>{{ todayStrands }}</b> 串</div>
        <div>今日销售 <b>{{ money(today.revenue) }}</b> 元 · {{ today.sold }} 串</div>
        <div>今日支出 <b>{{ money(todayExpense) }}</b> 元</div>
        <div style="border-left: 2px solid #dd8f2c; padding-left: 12px">今日净赚 <b>{{ money(todayProfit) }}</b> 元</div>
      </div>
    </div>

    <div class="two-col" style="margin-bottom: 14px">
      <div class="card card-pad">
        <div class="card-title">近 7 天收支走势 <small>收入 / 进货成本 / 毛利</small></div>
        <Chart :option="trendOption" height="300px" />
      </div>
      <div class="card card-pad">
        <div class="card-title">各批次毛利对比 <small>卖串的钱减去进货的钱</small></div>
        <Chart v-if="rows.length" :option="batchOption" height="300px" />
        <div v-else class="empty">记满一个批次后这里出对比图</div>
      </div>
    </div>

    <div class="card card-pad" style="margin-bottom: 14px">
      <div class="card-title">各品类成本占比 <small>钱主要花在哪个品类上</small></div>
      <Chart v-if="cats.length" :option="catPieOption" height="280px" />
      <div v-else class="empty">还没有进货记录</div>
    </div>

    <div class="card card-pad">
      <div class="card-title">利润是怎么算出来的</div>
      <div class="formula">
        原料成本 = 进货数量 × 单价（斤、棵、包、袋、份都行）<br />
        单片成本 = 原料成本 ÷ 剥下的总片数<br />
        单串成本 = 原料成本 ÷ 出品串数（也就是 单片成本 × 每串用几片）<br />
        单串利润 = 每串售价 − 单串成本<br />
        整批毛利 = 销售收入 − 原料成本<br />
        出品率 = 出品串数 ÷ 进货数量（一棵/一斤能出几串，越高越赚）<br />
        <small style="color: #7b8a81">
          上面每一个数都按你填的数量、单价、片数、串数、售价现场算出来，没有写死；
          一棵剥几片、几片穿一串、一串卖多少钱，都可以每次不一样。
        </small>
      </div>
    </div>
  </div>
</template>

<script setup>
import { computed } from 'vue'
import Chart from '../components/Chart.vue'
import { store } from '../store'
import { batchRows, totals, dailySeries, todayStats, expiringSoon, categoryRows, money, todayStr } from '../stats'

const todayLabel = computed(() => {
  const d = new Date()
  const w = ['日', '一', '二', '三', '四', '五', '六'][d.getDay()]
  return d.getFullYear() + '年' + (d.getMonth() + 1) + '月' + d.getDate() + '日 星期' + w
})

const rows = computed(() => batchRows(store))
const sum = computed(() => totals(rows.value))
const today = computed(() => todayStats(store, rows.value))
const series = computed(() => dailySeries(store, 7))
const risk = computed(() => expiringSoon(rows.value, 1))
const cats = computed(() => categoryRows(rows.value))

// 今日汇总
const todayCost = computed(() => {
  const t = todayStr()
  return store.batches.filter((b) => b.buyDate === t).reduce((s, b) => s + (b.cost || 0), 0)
})
const todayBatches = computed(() => {
  const t = todayStr()
  return store.batches.filter((b) => b.buyDate === t).length
})
const todayStrands = computed(() => store.strands.filter((s) => s.date === todayStr()).reduce((a, s) => a + (s.strands || 0), 0))
const todayExpense = computed(() => (store.expenses || []).filter((e) => e.date === todayStr()).reduce((a, e) => a + (e.cost || 0), 0))
const todayProfit = computed(() => today.value.profit - todayExpense.value)

const catPieOption = computed(() => ({
  tooltip: { trigger: 'item', valueFormatter: (v) => money(v) + ' 元' },
  legend: { bottom: 0, itemWidth: 10, itemHeight: 10, textStyle: { color: '#5f7169', fontSize: 12 } },
  series: [
    {
      type: 'pie',
      radius: ['40%', '66%'],
      center: ['50%', '44%'],
      itemStyle: { borderColor: '#fff', borderWidth: 2 },
      label: { show: false },
      data: cats.value.map((g) => ({
        name: g.label,
        value: Math.round(g.cost * 100) / 100,
        itemStyle: { color: g.color }
      }))
    }
  ]
}))

const trendOption = computed(() => ({
  tooltip: { trigger: 'axis', axisPointer: { type: 'shadow' } },
  legend: { bottom: 0, itemWidth: 10, itemHeight: 10, textStyle: { color: '#5f7169', fontSize: 12 } },
  grid: { left: 4, right: 10, top: 16, bottom: 40, containLabel: true },
  xAxis: {
    type: 'category',
    data: series.value.map((s) => s.label),
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
      name: '收入',
      type: 'bar',
      barWidth: 12,
      data: series.value.map((s) => s.revenue),
      itemStyle: { color: '#2f7d4f', borderRadius: [3, 3, 0, 0] }
    },
    {
      name: '进货成本',
      type: 'bar',
      barWidth: 12,
      data: series.value.map((s) => s.cost),
      itemStyle: { color: '#cddcd3', borderRadius: [3, 3, 0, 0] }
    },
    {
      name: '毛利',
      type: 'line',
      smooth: true,
      symbol: 'circle',
      symbolSize: 6,
      data: series.value.map((s) => s.profit),
      lineStyle: { color: '#dd8f2c', width: 2 },
      itemStyle: { color: '#dd8f2c' }
    }
  ]
}))

const batchOption = computed(() => {
  const labels = rows.value.map((r) => '第' + r.id + '批 ' + (r.buyDate || '').slice(5))
  return {
    tooltip: {
      trigger: 'axis',
      axisPointer: { type: 'shadow' },
      valueFormatter: (v) => money(v) + ' 元'
    },
    grid: { left: 4, right: 16, top: 10, bottom: 6, containLabel: true },
    xAxis: {
      type: 'value',
      splitLine: { lineStyle: { color: '#eef3ef' } },
      axisLine: { show: false },
      axisLabel: { color: '#8b9a90', fontSize: 11 }
    },
    yAxis: {
      type: 'category',
      data: labels,
      inverse: true,
      axisLine: { lineStyle: { color: '#dbe5dd' } },
      axisTick: { show: false },
      axisLabel: { color: '#6b7c71', fontSize: 11 }
    },
    series: [
      {
        type: 'bar',
        barWidth: 14,
        data: rows.value.map((r) => ({
          value: Math.round(r.profit * 100) / 100,
          itemStyle: { color: r.profit >= 0 ? '#2f7d4f' : '#bd3f33', borderRadius: 3 }
        }))
      }
    ]
  }
})
</script>
