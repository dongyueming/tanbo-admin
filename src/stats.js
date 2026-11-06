export function num(v) {
  const n = typeof v === 'number' ? v : parseFloat(v)
  return Number.isFinite(n) ? n : 0
}

export function money(v, digits = 2) {
  const n = num(v)
  return n.toLocaleString('zh-CN', {
    minimumFractionDigits: digits,
    maximumFractionDigits: digits
  })
}

export function round(v, digits = 2) {
  const p = Math.pow(10, digits)
  return Math.round(num(v) * p) / p
}

export function parseDate(str) {
  if (!str) return null
  const parts = String(str).split('-')
  if (parts.length !== 3) return null
  const d = new Date(Number(parts[0]), Number(parts[1]) - 1, Number(parts[2]))
  return Number.isNaN(d.getTime()) ? null : d
}

export function todayStr() {
  const d = new Date()
  const m = String(d.getMonth() + 1).padStart(2, '0')
  const day = String(d.getDate()).padStart(2, '0')
  return d.getFullYear() + '-' + m + '-' + day
}

export function addDays(str, days) {
  const d = parseDate(str)
  if (!d) return ''
  d.setDate(d.getDate() + num(days))
  const m = String(d.getMonth() + 1).padStart(2, '0')
  const day = String(d.getDate()).padStart(2, '0')
  return d.getFullYear() + '-' + m + '-' + day
}

export function daysBetween(from, to) {
  const a = parseDate(from)
  const b = parseDate(to)
  if (!a || !b) return 0
  const ms = b.getTime() - a.getTime()
  return Math.round(ms / 86400000)
}

/** 品类定义：单位、默认保鲜天数、图表配色 */
export const CATEGORIES = [
  { key: 'vegetable', label: '蔬菜', unit: '斤', shelfDays: 4, color: '#2f7d4f' },
  { key: 'meat', label: '肉类', unit: '斤', shelfDays: 2, color: '#bd3f33' },
  { key: 'ball', label: '丸子', unit: '包', shelfDays: 30, color: '#dd8f2c' },
  { key: 'sausage', label: '肠类', unit: '包', shelfDays: 15, color: '#b96334' },
  { key: 'base', label: '底料', unit: '袋', shelfDays: 60, color: '#55655c' },
  { key: 'soup', label: '配汤', unit: '斤', shelfDays: 3, color: '#8b5a2b' },
  { key: 'secret', label: '秘制酱', unit: '斤', shelfDays: 15, color: '#c0392b' },
  { key: 'chili', label: '辣酱', unit: '斤', shelfDays: 15, color: '#e67e22' },
  { key: 'sweet', label: '甜酱', unit: '斤', shelfDays: 15, color: '#d4ac0d' }
]

export function categoryOf(b) {
  return CATEGORIES.find((c) => c.key === b.category) || CATEGORIES[0]
}

export function unitOf(b) {
  return b.unit || categoryOf(b).unit
}

/** 这一批的货卖出去按什么算：底料按份，其余按串 */
export function outWordOf(b) {
  return categoryOf(b).key === 'base' ? '份' : '串'
}

/** 批次汇总：成本、出品、售出、剩余、收入、毛利 */
export function batchRows(state) {
  return state.batches.map((b) => {
    const cat = categoryOf(b)
    const unit = unitOf(b)
    const outWord = outWordOf(b)
    const strands = state.strands.filter((s) => s.batchId === b.id)
    const sales = state.sales.filter((s) => s.batchId === b.id)
    const losses = state.losses.filter((l) => l.batchId === b.id)

    const produced = strands.reduce((a, s) => a + num(s.strands), 0)
    const leaves = strands.reduce((a, s) => a + num(s.leaves), 0)
    const sold = sales.reduce((a, s) => a + num(s.strands), 0)
    const lost = losses.reduce((a, l) => a + num(l.strands), 0)
    const revenue = sales.reduce((a, s) => a + num(s.strands) * num(s.pricePerStrand), 0)
    const cost = num(b.weight) * num(b.pricePerJin)
    const profit = revenue - cost
    const stock = produced - sold - lost

    const yieldPerJin = num(b.weight) > 0 ? produced / num(b.weight) : 0
    const unitCost = produced > 0 ? cost / produced : 0
    const avgPrice = sold > 0 ? revenue / sold : 0

    /* 单位经济：一片多少钱、一串用几片、一串赚多少 */
    const leafCost = leaves > 0 ? cost / leaves : 0
    const leavesPerStrand = produced > 0 ? leaves / produced : 0
    const unitProfitDirect = avgPrice > 0 ? avgPrice - unitCost : 0
    const margin = revenue > 0 ? profit / revenue * 100 : 0
    const unitProfit = sold > 0 ? profit / sold : 0

    const expireDate = addDays(b.buyDate, b.shelfDays)
    const daysLeft = expireDate ? daysBetween(todayStr(), expireDate) : 0

    let shelfTone = 'gray'
    let shelfText = '已卖完'
    if (stock > 0) {
      if (daysLeft < 0) {
        shelfTone = 'red'
        shelfText = '已过期 ' + Math.abs(daysLeft) + ' 天'
      } else if (daysLeft <= 1) {
        shelfTone = 'amber'
        shelfText = '剩 ' + daysLeft + ' 天，抓紧'
      } else {
        shelfTone = 'green'
        shelfText = '剩 ' + daysLeft + ' 天'
      }
    }

    return {
      ...b,
      category: cat.key,
      categoryLabel: cat.label,
      unit,
      outWord,
      produced,
      leaves,
      sold,
      lost,
      stock,
      revenue,
      cost,
      profit,
      margin,
      yieldPerJin,
      unitCost,
      avgPrice,
      unitProfit,
      leafCost,
      leavesPerStrand,
      unitProfitDirect,
      expireDate,
      daysLeft,
      shelfTone,
      shelfText,
      strandCount: strands.length,
      saleCount: sales.length
    }
  })
}

export function totals(rows) {
  const revenue = rows.reduce((a, r) => a + r.revenue, 0)
  const cost = rows.reduce((a, r) => a + r.cost, 0)
  const profit = revenue - cost
  const produced = rows.reduce((a, r) => a + r.produced, 0)
  const sold = rows.reduce((a, r) => a + r.sold, 0)
  const lost = rows.reduce((a, r) => a + r.lost, 0)
  const stock = rows.reduce((a, r) => a + r.stock, 0)
  const weight = rows.reduce((a, r) => a + num(r.weight), 0)
  return {
    revenue,
    cost,
    profit,
    margin: revenue > 0 ? profit / revenue * 100 : 0,
    produced,
    sold,
    lost,
    stock,
    weight,
    yieldPerJin: weight > 0 ? produced / weight : 0,
    unitCost: produced > 0 ? cost / produced : 0,
    batches: rows.length
  }
}

/** 近 N 天：收入、进货成本、毛利 */
export function dailySeries(state, days = 7) {
  const out = []
  const today = new Date()
  for (let i = days - 1; i >= 0; i--) {
    const d = new Date(today)
    d.setDate(d.getDate() - i)
    const m = String(d.getMonth() + 1).padStart(2, '0')
    const day = String(d.getDate()).padStart(2, '0')
    const key = d.getFullYear() + '-' + m + '-' + day
    const revenue = state.sales
      .filter((s) => s.date === key)
      .reduce((a, s) => a + num(s.strands) * num(s.pricePerStrand), 0)
    const cost = state.batches
      .filter((b) => b.buyDate === key)
      .reduce((a, b) => a + num(b.weight) * num(b.pricePerJin), 0)
    out.push({
      date: key,
      label: m + '/' + day,
      revenue: round(revenue),
      cost: round(cost),
      profit: round(revenue - cost)
    })
  }
  return out
}

/** 今日经营：收入、售出串数、进货成本 */
export function todayStats(state, rows) {
  const today = todayStr()
  const todaySales = state.sales.filter((s) => s.date === today)
  const revenue = todaySales.reduce((a, s) => a + num(s.strands) * num(s.pricePerStrand), 0)
  const sold = todaySales.reduce((a, s) => a + num(s.strands), 0)
  const cost = state.batches
    .filter((b) => b.buyDate === today)
    .reduce((a, b) => a + num(b.weight) * num(b.pricePerJin), 0)
  const lost = state.losses
    .filter((l) => l.date === today)
    .reduce((a, l) => a + num(l.strands), 0)
  const stock = rows.reduce((a, r) => a + r.stock, 0)
  const risk = rows.filter((r) => r.stock > 0 && r.daysLeft <= 1)
  return { revenue, sold, cost, profit: revenue - cost, lost, stock, risk }
}

/** 未来 N 天内到期的批次 */
export function expiringSoon(rows, days = 2) {
  return rows.filter((r) => r.stock > 0 && r.daysLeft <= days)
}

/** 按品类汇总：成本、收入、毛利、占比 */
export function categoryRows(rows) {
  const totalCost = rows.reduce((a, r) => a + r.cost, 0)
  const totalRevenue = rows.reduce((a, r) => a + r.revenue, 0)

  return CATEGORIES.map((c) => {
    const rs = rows.filter((r) => r.category === c.key)
    const cost = rs.reduce((a, r) => a + r.cost, 0)
    const revenue = rs.reduce((a, r) => a + r.revenue, 0)
    const profit = revenue - cost
    const produced = rs.reduce((a, r) => a + r.produced, 0)
    const sold = rs.reduce((a, r) => a + r.sold, 0)
    const stock = rs.reduce((a, r) => a + r.stock, 0)
    const qty = rs.reduce((a, r) => a + num(r.weight), 0)

    return {
      key: c.key,
      label: c.label,
      color: c.color,
      count: rs.length,
      cost,
      revenue,
      profit,
      margin: revenue > 0 ? profit / revenue * 100 : 0,
      produced,
      sold,
      stock,
      qty,
      unitCost: produced > 0 ? cost / produced : 0,
      unitProfit: sold > 0 ? profit / sold : 0,
      costShare: totalCost > 0 ? cost / totalCost * 100 : 0,
      revenueShare: totalRevenue > 0 ? revenue / totalRevenue * 100 : 0
    }
  }).filter((g) => g.count > 0)
}
