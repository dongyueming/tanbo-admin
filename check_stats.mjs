import { readFileSync } from 'node:fs'
import { batchRows, categoryRows, totals } from './src/stats.js'

const read = (p) => JSON.parse(readFileSync(p, 'utf8'))
const state = {
  batches: read('data/batches.json'),
  strands: read('data/strands.json'),
  sales: read('data/sales.json'),
  losses: read('data/losses.json')
}

const rows = batchRows(state)
const cats = categoryRows(rows)
const sum = totals(rows)

console.log('批次行数: ' + rows.length)
console.log('')
console.log('批次明细 (id | 品类 | 品名 | 数量 | 成本 | 片数 | 出品 | 每串片数 | 单片成本 | 单串成本 | 均价 | 单串利润)')
for (const r of rows) {
  console.log(
    [
      r.id,
      r.categoryLabel,
      r.name,
      r.weight + r.unit,
      r.cost.toFixed(2),
      r.leaves || '-',
      r.produced,
      r.leavesPerStrand ? r.leavesPerStrand.toFixed(2) : '-',
      r.leafCost ? r.leafCost.toFixed(3) : '-',
      r.unitCost.toFixed(2),
      r.avgPrice ? r.avgPrice.toFixed(2) : '-',
      r.avgPrice ? r.unitProfitDirect.toFixed(2) : '-'
    ].join(' | ')
  )
}

console.log('')
console.log('品类汇总:')
for (const g of cats) {
  console.log(
    [
      g.label,
      '批次' + g.count,
      '成本' + g.cost.toFixed(2),
      '占比' + g.costShare.toFixed(1) + '%',
      '收入' + g.revenue.toFixed(2),
      '毛利' + g.profit.toFixed(2),
      '毛利率' + g.margin.toFixed(1) + '%'
    ].join(' | ')
  )
}

console.log('')
console.log('总计: 成本 ' + sum.cost.toFixed(2) + ' / 收入 ' + sum.revenue.toFixed(2) + ' / 毛利 ' + sum.profit.toFixed(2) + ' / 毛利率 ' + sum.margin.toFixed(1) + '%')

// 一致性校验
const catCost = cats.reduce((a, g) => a + g.cost, 0)
const catRev = cats.reduce((a, g) => a + g.revenue, 0)
const catProfit = cats.reduce((a, g) => a + g.profit, 0)
const diffs = [
  Math.abs(catCost - sum.cost),
  Math.abs(catRev - sum.revenue),
  Math.abs(catProfit - sum.profit)
]
const maxDiff = Math.max(...diffs)
console.log('')
console.log('品类合计 vs 总计 最大偏差: ' + maxDiff.toFixed(6))
console.log(maxDiff < 0.01 ? '校验通过' : '校验失败：品类汇总对不上总计')
