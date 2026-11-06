const JSON_HEADERS = { 'Content-Type': 'application/json' }

async function request(method, url, body) {
  const res = await fetch(url, {
    method,
    credentials: 'same-origin',
    headers: body ? JSON_HEADERS : undefined,
    body: body ? JSON.stringify(body) : undefined
  })
  if (res.status === 401) {
    const err = new Error('登录状态已失效，请重新登录')
    err.unauthorized = true
    throw err
  }
  if (res.status === 204) return null
  let data = null
  try {
    data = await res.json()
  } catch (e) {
    data = null
  }
  if (!res.ok) {
    throw new Error((data && data.error) || '请求失败（' + res.status + '）')
  }
  return data
}

export const api = {
  me: () => request('GET', '/api/me'),
  login: async (phone, password) => {
    const res = await fetch('/api/login', {
      method: 'POST',
      credentials: 'same-origin',
      headers: JSON_HEADERS,
      body: JSON.stringify({ phone, password })
    })
    let data = null
    try {
      data = await res.json()
    } catch (e) {
      data = null
    }
    if (!res.ok) throw new Error((data && data.error) || '登录失败')
    return data
  },
  logout: () => request('POST', '/api/logout'),
  state: () => request('GET', '/api/state'),
  create: (type, payload) => request('POST', '/api/items/' + type, payload),
  update: (type, id, payload) => request('PUT', '/api/items/' + type + '/' + id, payload),
  remove: (type, id) => request('DELETE', '/api/items/' + type + '/' + id)
}
