/**
 * AuthUtil - JWT Token 管理工具
 */
import { preferences } from '@kit.ArkData'
import fs from '@ohos.file.fs'

const STORE_NAME: string = 'dorm_security_auth'

// CA 证书路径缓存（写入一次，全局复用）
let cachedCaPath: string = ''

// 内存缓存 Token（避免频繁 I/O）
let cachedToken: string = ''
let cachedUsername: string = ''
let cachedDisplayName: string = ''
let cachedRole: string = ''

/** 保存登录信息 */
export async function saveAuth(token: string, username: string,
                                displayName: string, role: string): Promise<void> {
  cachedToken = token
  cachedUsername = username
  cachedDisplayName = displayName
  cachedRole = role

  // 持久化到本地
  let prefs = await preferences.getPreferences(getContext(), STORE_NAME)
  await prefs.put('token', token)
  await prefs.put('username', username)
  await prefs.put('displayName', displayName)
  await prefs.put('role', role)
  await prefs.flush()
}

/** 获取 Token（优先内存缓存） */
export async function getToken(): Promise<string> {
  if (cachedToken.length > 0) {
    return cachedToken
  }
  try {
    let prefs = await preferences.getPreferences(getContext(), STORE_NAME)
    cachedToken = await prefs.get('token', '') as string
    return cachedToken
  } catch (e) {
    return ''
  }
}

/** 获取用户名 */
export async function getUsername(): Promise<string> {
  if (cachedUsername.length > 0) return cachedUsername
  try {
    let prefs = await preferences.getPreferences(getContext(), STORE_NAME)
    cachedUsername = await prefs.get('username', '') as string
    return cachedUsername
  } catch (e) { return '' }
}

/** 获取显示名 */
export async function getDisplayName(): Promise<string> {
  if (cachedDisplayName.length > 0) return cachedDisplayName
  try {
    let prefs = await preferences.getPreferences(getContext(), STORE_NAME)
    cachedDisplayName = await prefs.get('displayName', '') as string
    return cachedDisplayName
  } catch (e) { return '' }
}

/** 是否已登录 */
export async function isLoggedIn(): Promise<boolean> {
  let token = await getToken()
  return token.length > 0
}

/** 退出登录 — 清除所有 Token */
export async function clearAuth(): Promise<void> {
  cachedToken = ''
  cachedUsername = ''
  cachedDisplayName = ''
  cachedRole = ''
  try {
    let prefs = await preferences.getPreferences(getContext(), STORE_NAME)
    await prefs.delete('token')
    await prefs.delete('username')
    await prefs.delete('displayName')
    await prefs.delete('role')
    await prefs.flush()
  } catch (e) { }
}

/**
 * 获取 CA 证书文件路径（HTTPS 自签证书验证用）
 * 首次调用时从 rawfile 读取 ca.crt 写入缓存目录
 */
export async function getCaPath(): Promise<string> {
  if (cachedCaPath.length > 0) {
    return cachedCaPath
  }
  try {
    let caPath = getContext().filesDir + '/ca.pem'
    let rawData = await getContext().resourceManager.getRawFileContent('ca.crt')
    let file = fs.openSync(caPath, fs.OpenMode.CREATE | fs.OpenMode.TRUNC | fs.OpenMode.WRITE_ONLY)
    fs.writeSync(file.fd, rawData.buffer)
    fs.closeSync(file)
    cachedCaPath = caPath
    return caPath
  } catch (e) {
    return ''
  }
}
