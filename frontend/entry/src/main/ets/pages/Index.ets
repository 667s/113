/**
 * 宿舍安防 — 单文件应用
 */
import { http } from '@kit.NetworkKit'

class RfidRecord {
  id: number = 0; cardUid: string = ''; holderName: string = ''
  isAuthorized: boolean = false; scanTime: string = ''
}
class SensorData {
  id: number = 0; temperature: number = 0; humidity: number = 0; recordedAt: string = ''
}
class AlarmLog {
  id: number = 0; alarmType: string = ''; message: string = ''; triggeredAt: string = ''
}
class AuthorizedCard {
  id: number = 0; cardUid: string = ''; holderName: string = ''
  studentId: string = ''; isActive: boolean = true
}
interface AddCardBody { cardUid: string; holderName: string; studentId: string }
interface StatusBody { isActive: boolean }

// 模拟器网络：试 172.18.0.153(以太网) 或 10.0.2.2(Android约定) 或 10.160.153.147(WiFi)
const BASE: string = 'http://172.18.0.153:8080'
const TAB_LABELS: string[] = ['🏠首页', '💳刷卡', '🌡️温湿度', '🚨告警', '🔑卡片']
const ALARM_FILTERS: string[] = ['全部', 'UNAUTHORIZED']

@Entry
@Component
struct Index {
  @State tab: number = 0
  // 仪表盘
  @State temp: string = '--'; @State humi: string = '--'
  @State pirDetected: boolean = false; @State pirTime: string = ''
  @State cardCount: number = 0
  @State dashAlarms: AlarmLog[] = []; @State dashRecords: RfidRecord[] = []
  // 刷卡
  @State rfidList: RfidRecord[] = []; @State rfidPage: number = 0
  @State rfidLoading: boolean = false; @State rfidEnd: boolean = false; @State rfidErr: string = ''
  // 温湿度
  @State sensorList: SensorData[] = []; @State sensorLatest: SensorData = new SensorData()
  @State sensorPage: number = 0; @State sensorLoading: boolean = false
  @State sensorEnd: boolean = false; @State sensorErr: string = ''
  // 告警
  @State alarmList: AlarmLog[] = []; @State alarmPage: number = 0
  @State alarmLoading: boolean = false; @State alarmEnd: boolean = false
  @State alarmFilter: string = ''; @State alarmErr: string = ''
  // 卡片
  @State cards: AuthorizedCard[] = []; @State showAdd: boolean = false
  @State newUid: string = ''; @State newName: string = ''; @State newSid: string = ''

  private timer: number = -1

  aboutToAppear(): void {
    this.loadDashboard()
    this.loadRfid(true)
    this.loadSensor(true)
    this.loadAlarms(true)
    this.loadCards()
    // 每3秒自动刷新仪表盘, 实时显示温湿度/PIR/告警
    this.timer = setInterval(() => { this.loadDashboard() }, 3000)
  }

  aboutToDisappear(): void {
    if (this.timer >= 0) { clearInterval(this.timer); this.timer = -1 }
  }

  // ==================== 网络 ====================
  async httpGet(path: string): Promise<string> {
    const r: http.HttpRequest = http.createHttp()
    const res: http.HttpResponse = await r.request(BASE + path, {
      method: http.RequestMethod.GET,
      connectTimeout: 8000,
      readTimeout: 8000
    })
    const body: string = res.result as string
    r.destroy()
    if (res.responseCode !== 200) {
      throw new Error('HTTP ' + res.responseCode + ': ' + body)
    }
    return body
  }
  async httpPost(path: string, body: Object): Promise<string> {
    const r: http.HttpRequest = http.createHttp()
    const h: Record<string, string> = {}; h['Content-Type'] = 'application/json'
    const res: http.HttpResponse = await r.request(BASE + path, {
      method: http.RequestMethod.POST, header: h, extraData: JSON.stringify(body),
      connectTimeout: 5000, readTimeout: 5000
    })
    r.destroy()
    return res.result as string
  }
  async httpPut(path: string, body: Object): Promise<string> {
    const r: http.HttpRequest = http.createHttp()
    const h: Record<string, string> = {}; h['Content-Type'] = 'application/json'
    const res: http.HttpResponse = await r.request(BASE + path, {
      method: http.RequestMethod.PUT, header: h, extraData: JSON.stringify(body),
      connectTimeout: 5000, readTimeout: 5000
    })
    r.destroy()
    return res.result as string
  }
  async httpDel(path: string): Promise<string> {
    const r: http.HttpRequest = http.createHttp()
    const res: http.HttpResponse = await r.request(BASE + path, {
      method: http.RequestMethod.DELETE, connectTimeout: 5000, readTimeout: 5000
    })
    r.destroy()
    return res.result as string
  }

  // ==================== 仪表盘 ====================
  async loadDashboard(): Promise<void> {
    try {
      const s: string = await this.httpGet('/api/sensor/dht/latest')
      const d: SensorData = JSON.parse(s) as SensorData
      this.temp = d.temperature ? d.temperature.toFixed(1) : '--'
      this.humi = d.humidity ? d.humidity.toFixed(1) : '--'
    } catch (e) {
      this.temp = '--'; this.humi = '--'
    }
    try { const a: string = await this.httpGet('/api/alarm/logs/latest'); this.dashAlarms = JSON.parse(a) as AlarmLog[] } catch (e) {}
    try { const r: string = await this.httpGet('/api/rfid/records/latest'); this.dashRecords = JSON.parse(r) as RfidRecord[] } catch (e) {}
    try {
      const c: string = await this.httpGet('/api/cards')
      this.cardCount = (JSON.parse(c) as AuthorizedCard[]).length
    } catch (e) {}
    try {
      const pir: string = await this.httpGet('/api/sensor/pir/status')
      const ps: Record<string, Object> = JSON.parse(pir) as Record<string, Object>
      this.pirDetected = ps['detected'] as boolean
      this.pirTime = ps['lastTime'] as string
    } catch (e) {}
  }

  // ==================== 刷卡记录 ====================
  async loadRfid(refresh: boolean): Promise<void> {
    if (this.rfidLoading) return
    this.rfidLoading = true; this.rfidErr = ''
    const page: number = refresh ? 0 : this.rfidPage
    try {
      const res: string = await this.httpGet('/api/rfid/records?page=' + String(page) + '&size=20')
      const data: Record<string, Object> = JSON.parse(res) as Record<string, Object>
      const c1: RfidRecord[] = data['content'] as RfidRecord[]
      const tp: number = data['totalPages'] as number
      this.rfidList = refresh ? c1 : this.rfidList.concat(c1)
      this.rfidPage = page + 1
      this.rfidEnd = this.rfidPage >= tp
    } catch (e) {
      const msg: string = String(e)
      console.error('loadRfid failed: ' + msg)
      this.rfidErr = '请求失败: ' + msg
      this.rfidEnd = true
    }
    this.rfidLoading = false
  }

  // ==================== 温湿度 ====================
  async loadSensor(refresh: boolean): Promise<void> {
    if (this.sensorLoading) return
    this.sensorLoading = true; this.sensorErr = ''
    const page: number = refresh ? 0 : this.sensorPage
    try {
      const ls: string = await this.httpGet('/api/sensor/dht/latest')
      this.sensorLatest = JSON.parse(ls) as SensorData
      const hs: string = await this.httpGet('/api/sensor/dht?page=' + String(page) + '&size=20')
      const data: Record<string, Object> = JSON.parse(hs) as Record<string, Object>
      const c1: SensorData[] = data['content'] as SensorData[]
      const tp: number = data['totalPages'] as number
      this.sensorList = refresh ? c1 : this.sensorList.concat(c1)
      this.sensorPage = page + 1
      this.sensorEnd = this.sensorPage >= tp
    } catch (e) {
      const msg: string = String(e)
      console.error('loadSensor failed: ' + msg)
      this.sensorErr = '请求失败: ' + msg
      this.sensorEnd = true
    }
    this.sensorLoading = false
  }

  // ==================== 告警 ====================
  async loadAlarms(refresh: boolean): Promise<void> {
    if (this.alarmLoading) return
    this.alarmLoading = true; this.alarmErr = ''
    const page: number = refresh ? 0 : this.alarmPage
    let path: string = '/api/alarm/logs?page=' + String(page) + '&size=20'
    if (this.alarmFilter) path = path + '&type=' + this.alarmFilter
    try {
      const res: string = await this.httpGet(path)
      const data: Record<string, Object> = JSON.parse(res) as Record<string, Object>
      const c1: AlarmLog[] = data['content'] as AlarmLog[]
      const tp: number = data['totalPages'] as number
      this.alarmList = refresh ? c1 : this.alarmList.concat(c1)
      this.alarmPage = page + 1
      this.alarmEnd = this.alarmPage >= tp
    } catch (e) {
      const msg: string = String(e)
      console.error('loadAlarms failed: ' + msg)
      this.alarmErr = '请求失败: ' + msg
      this.alarmEnd = true
    }
    this.alarmLoading = false
  }
  setAlarmFilter(t: string): void {
    this.alarmFilter = t; this.alarmPage = 0; this.alarmList = []; this.alarmEnd = false
    this.loadAlarms(true)
  }

  // ==================== 卡片 ====================
  async loadCards(): Promise<void> {
    try { const res: string = await this.httpGet('/api/cards'); this.cards = JSON.parse(res) as AuthorizedCard[] } catch (e) {}
  }
  async doAddCard(): Promise<void> {
    if (!this.newUid.trim() || !this.newName.trim()) return
    try {
      const body: AddCardBody = { cardUid: this.newUid.trim(), holderName: this.newName.trim(), studentId: this.newSid.trim() }
      await this.httpPost('/api/cards', body)
      this.showAdd = false; this.newUid = ''; this.newName = ''; this.newSid = ''
      this.loadCards()
    } catch (e) {}
  }
  async doToggleCard(card: AuthorizedCard): Promise<void> {
    try { const body: StatusBody = { isActive: !card.isActive }; await this.httpPut('/api/cards/' + String(card.id) + '/status', body); this.loadCards() } catch (e) {}
  }
  async doDeleteCard(card: AuthorizedCard): Promise<void> {
    try { await this.httpDel('/api/cards/' + String(card.id)); this.loadCards() } catch (e) {}
  }

  // ==================== UI ====================
  build(): void {
    Column() {
      Row() { Text(this.pickTitle()).fontSize(20).fontWeight(FontWeight.Bold).fontColor('#FFF') }
      .width('100%').height(50).backgroundColor('#1565C0').padding(16).justifyContent(FlexAlign.Start)
      Tabs({ index: this.tab }) {
        TabContent() { this.Dashboard() }
        TabContent() { this.RfidTab() }
        TabContent() { this.SensorTab() }
        TabContent() { this.AlarmTab() }
        TabContent() { this.CardTab() }
      }.barHeight(0).layoutWeight(1).onChange((i: number) => { this.tab = i })
      Row() {
        ForEach(TAB_LABELS, (label: string, i: number) => {
          Text(label).fontSize(11).fontColor(this.tab === i ? '#1565C0' : '#999')
            .layoutWeight(1).textAlign(TextAlign.Center).padding(6)
            .onClick(() => { this.tab = i })
        })
      }.width('100%').height(50).backgroundColor('#FFF').shadow({ radius: 4, color: '#0C000000', offsetY: -1 })
    }.width('100%').height('100%').backgroundColor('#F5F5F5')
  }
  pickTitle(): string { if (this.tab === 0) return '宿舍安防'; if (this.tab === 1) return '刷卡记录'; if (this.tab === 2) return '温湿度'; if (this.tab === 3) return '告警日志'; return '卡片管理' }

  // ==================== 仪表盘 Tab ====================
  @Builder Dashboard(): void {
    Scroll() { Column() {
      Row() { this.StatCard('🌡️ 温度', this.temp + '°C', '#FFF3E0'); Blank().width(12); this.StatCard('💧 湿度', this.humi + '%', '#E3F2FD') }.width('100%').padding(12)
      Row() { this.StatCard('🔑 授权卡', String(this.cardCount) + '张', '#E8F5E9'); Blank().width(12); this.StatCard('👤 人体红外', this.pirDetected ? '有人' : '无人', this.pirDetected ? '#FFEBEE' : '#E8F5E9') }.width('100%').padding(12)
      Text('⚠️ 最新告警').fontSize(16).fontWeight(FontWeight.Bold).width('100%').padding(16)
      if (this.dashAlarms.length === 0) { Text('暂无').fontSize(13).fontColor('#999').padding(16) }
      ForEach(this.dashAlarms.slice(0, 5), (a: AlarmLog) => {
        Row() {
          Text(this.emoji(a.alarmType)).fontSize(18)
          Column() { Text(a.message).fontSize(14).maxLines(1).textOverflow({ overflow: TextOverflow.Ellipsis }); Text(a.triggeredAt).fontSize(11).fontColor('#AAA') }
          .alignItems(HorizontalAlign.Start).padding(8).layoutWeight(1)
          Text(a.alarmType).fontSize(10).fontColor('#FFF').backgroundColor(this.acolor(a.alarmType)).borderRadius(4).padding(6)
        }.width('100%').padding(12).backgroundColor('#FFF').borderRadius(8).margin({ top: 4 })
      })
      Text('💳 最新刷卡').fontSize(16).fontWeight(FontWeight.Bold).width('100%').padding(16)
      if (this.dashRecords.length === 0) { Text('暂无').fontSize(13).fontColor('#999').padding(16) }
      ForEach(this.dashRecords.slice(0, 5), (r: RfidRecord) => {
        Row() {
          Column() { Text(r.holderName ? r.holderName : r.cardUid).fontSize(14); Text(r.scanTime).fontSize(11).fontColor('#AAA') }
          .alignItems(HorizontalAlign.Start).layoutWeight(1)
          Text(r.isAuthorized ? '已授权' : '未授权').fontSize(11).fontColor('#FFF').backgroundColor(r.isAuthorized ? '#4CAF50' : '#F44336').borderRadius(4).padding(6)
        }.width('100%').padding(12).backgroundColor('#FFF').borderRadius(8).margin({ top: 4 })
      })
      Blank().height(20)
    }}.scrollBar(BarState.Off).width('100%').height('100%')
  }
  @Builder StatCard(title: string, value: string, bg: ResourceColor): void {
    Column() { Text(value).fontSize(24).fontWeight(FontWeight.Bold); Text(title).fontSize(12).fontColor('#666').margin({ top: 4 }) }
    .width('48%').padding(16).backgroundColor(bg).borderRadius(10)
  }

  // ==================== 刷卡 Tab ====================
  @Builder RfidTab(): void {
    List() {
      if (this.rfidList.length === 0 && !this.rfidLoading && this.rfidErr.length > 0) {
        ListItem() { Column() { Text('⚠️').fontSize(36); Text(this.rfidErr).fontSize(14).fontColor('#F44336').padding(12) }.width('100%').alignItems(HorizontalAlign.Center).padding(40) }
      }
      ForEach(this.rfidList, (r: RfidRecord) => {
        ListItem() {
          Row() {
            Column() { Text(r.holderName ? r.holderName : '未知').fontSize(15).fontWeight(FontWeight.Medium); Text('UID: ' + r.cardUid).fontSize(11).fontColor('#999'); Text(r.scanTime).fontSize(10).fontColor('#AAA') }
            .alignItems(HorizontalAlign.Start).layoutWeight(1)
            Text(r.isAuthorized ? '已授权' : '未授权').fontSize(11).fontColor('#FFF').backgroundColor(r.isAuthorized ? '#4CAF50' : '#F44336').borderRadius(4).padding(8)
          }.width('100%').padding(12).backgroundColor('#FFF').borderRadius(8).margin({ top: 4 })
        }
      })
      if (this.rfidList.length === 0 && this.rfidLoading) {
        ListItem() { Row() { LoadingProgress().width(20).height(20).color('#1565C0') }.width('100%').justifyContent(FlexAlign.Center).padding(40) }
      }
      if (!this.rfidEnd && this.rfidList.length > 0) {
        ListItem() { Row() { LoadingProgress().width(16).height(16).color('#1565C0'); Text('加载中...').fontSize(12).fontColor('#999').padding(8) }.width('100%').justifyContent(FlexAlign.Center).padding(12) }
        .onAppear(() => { this.loadRfid(false) })
      }
    }.scrollBar(BarState.Off).width('100%').height('100%').padding(12)
  }

  // ==================== 温湿度 Tab ====================
  @Builder SensorTab(): void {
    List() {
      ListItem() {
        Row() {
          Column() { Text('🌡️').fontSize(32); Text(this.sensorLatest.temperature ? this.sensorLatest.temperature.toFixed(1) + '°C' : '-- °C').fontSize(26).fontWeight(FontWeight.Bold).fontColor('#FF6F00'); Text('温度').fontSize(12).fontColor('#999') }
          .width('46%').padding(16).backgroundColor('#FFF8E1').borderRadius(10)
          Blank()
          Column() { Text('💧').fontSize(32); Text(this.sensorLatest.humidity ? this.sensorLatest.humidity.toFixed(1) + '%' : '-- %').fontSize(26).fontWeight(FontWeight.Bold).fontColor('#1565C0'); Text('湿度').fontSize(12).fontColor('#999') }
          .width('46%').padding(16).backgroundColor('#E8EAF6').borderRadius(10)
        }.width('100%').padding(12)
      }
      ListItem() { Text('📊 历史数据').fontSize(15).fontWeight(FontWeight.Bold).width('100%').padding(16) }
      if (this.sensorList.length === 0 && !this.sensorLoading && this.sensorErr.length > 0) {
        ListItem() { Column() { Text('⚠️').fontSize(36); Text(this.sensorErr).fontSize(14).fontColor('#F44336').padding(12) }.width('100%').alignItems(HorizontalAlign.Center).padding(40) }
      }
      ForEach(this.sensorList, (d: SensorData) => {
        ListItem() {
          Row() { Text('🌡️ ' + d.temperature.toFixed(1) + '°C').fontSize(14).fontWeight(FontWeight.Medium).fontColor('#FF6F00'); Text('💧 ' + d.humidity.toFixed(1) + '%').fontSize(14).fontWeight(FontWeight.Medium).fontColor('#1565C0').padding(16); Blank(); Text(d.recordedAt).fontSize(11).fontColor('#AAA') }
          .width('100%').padding(12).backgroundColor('#FFF').borderRadius(8).margin({ top: 2 })
        }
      })
      if (this.sensorList.length === 0 && this.sensorLoading) {
        ListItem() { Row() { LoadingProgress().width(20).height(20) }.width('100%').justifyContent(FlexAlign.Center).padding(40) }
      }
      if (!this.sensorEnd && this.sensorList.length > 0) {
        ListItem() { Row() { LoadingProgress().width(16).height(16); Text('加载中...').fontSize(12).fontColor('#999').padding(8) }.width('100%').justifyContent(FlexAlign.Center).padding(12) }
        .onAppear(() => { this.loadSensor(false) })
      }
    }.scrollBar(BarState.Off).width('100%').height('100%').padding(12)
  }

  // ==================== 告警 Tab ====================
  @Builder AlarmTab(): void {
    Column() {
      Row() {
        ForEach(ALARM_FILTERS, (t: string) => {
          Text(t).fontSize(12).fontColor(this.filterOn(t) ? '#FFF' : '#666').backgroundColor(this.filterOn(t) ? '#1565C0' : '#FFF')
          .borderRadius(14).border({ width: 1, color: '#DDD' }).padding(12).margin(6)
          .onClick(() => { this.setAlarmFilter(t === '全部' ? '' : t) })
        })
      }.width('100%').padding(10).backgroundColor('#FFF')
      List() {
        if (this.alarmList.length === 0 && !this.alarmLoading && this.alarmErr.length > 0) {
          ListItem() { Column() { Text('⚠️').fontSize(36); Text(this.alarmErr).fontSize(14).fontColor('#F44336').padding(12) }.width('100%').alignItems(HorizontalAlign.Center).padding(40) }
        }
        ForEach(this.alarmList, (a: AlarmLog) => {
          ListItem() {
            Row() {
              Text(this.emoji(a.alarmType)).fontSize(24)
              Column() { Text(a.message).fontSize(14).maxLines(2).textOverflow({ overflow: TextOverflow.Ellipsis }); Text(a.triggeredAt).fontSize(11).fontColor('#AAA').margin({ top: 2 }) }
              .alignItems(HorizontalAlign.Start).layoutWeight(1).padding(8)
              Text(a.alarmType).fontSize(10).fontColor('#FFF').backgroundColor(this.acolor(a.alarmType)).borderRadius(4).padding(6)
            }.width('100%').padding(12).backgroundColor('#FFF').borderRadius(8).margin({ top: 4 })
          }
        })
        if (this.alarmList.length === 0 && this.alarmLoading) {
          ListItem() { Row() { LoadingProgress().width(20).height(20) }.width('100%').justifyContent(FlexAlign.Center).padding(40) }
        }
        if (!this.alarmEnd && this.alarmList.length > 0) {
          ListItem() { Row() { LoadingProgress().width(16).height(16); Text('加载中...').fontSize(12).fontColor('#999').padding(8) }.width('100%').justifyContent(FlexAlign.Center).padding(12) }
          .onAppear(() => { this.loadAlarms(false) })
        }
      }.scrollBar(BarState.Off).layoutWeight(1).padding(12)
    }.width('100%').height('100%')
  }
  filterOn(t: string): boolean { if (t === '全部' && this.alarmFilter === '') return true; return t === this.alarmFilter }

  // ==================== 卡片 Tab ====================
  @Builder CardTab(): void {
    Stack() {
      List() {
        if (this.cards.length === 0) {
          ListItem() { Column() { Text('🔑').fontSize(48).opacity(0.3); Text('暂无授权卡').fontSize(14).fontColor('#999').margin({ top: 8 }); Text('点击右下角 + 添加').fontSize(11).fontColor('#CCC') }.width('100%').padding(60).alignItems(HorizontalAlign.Center) }
        }
        ForEach(this.cards, (c: AuthorizedCard) => {
          ListItem() {
            Row() {
              Column() { Text(c.holderName).fontSize(15).fontWeight(FontWeight.Medium); Text('UID: ' + c.cardUid).fontSize(11).fontColor('#999'); if (c.studentId) { Text('学号: ' + c.studentId).fontSize(11).fontColor('#999') } }
              .alignItems(HorizontalAlign.Start).layoutWeight(1)
              Column() {
                Text(c.isActive ? '已启用' : '已停用').fontSize(11).fontColor('#FFF').backgroundColor(c.isActive ? '#4CAF50' : '#9E9E9E').borderRadius(4).padding(8)
                Row() { Text('切换').fontSize(10).fontColor('#1565C0').padding(4).onClick(() => { this.doToggleCard(c) }); Text('删除').fontSize(10).fontColor('#F44336').padding(4).margin(8).onClick(() => { this.doDeleteCard(c) }) }.margin({ top: 4 })
              }.alignItems(HorizontalAlign.End)
            }.width('100%').padding(12).backgroundColor('#FFF').borderRadius(8).margin({ top: 4 })
          }
        })
      }.scrollBar(BarState.Off).width('100%').height('100%').padding({ bottom: 80 })
      Button('＋').fontSize(26).width(52).height(52).borderRadius(26).backgroundColor('#1565C0').fontColor('#FFF')
      .shadow({ radius: 6, color: '#3D000000', offsetY: 3 }).position({ x: '82%', y: '90%' }).onClick(() => { this.showAdd = true })
    }.width('100%').height('100%')
    if (this.showAdd) {
      Column() {
        Column() {
          Text('添加授权卡').fontSize(17).fontWeight(FontWeight.Bold).width('100%').padding({ bottom: 14 })
          TextInput({ placeholder: '卡号 UID（必填）' }).onChange((v: string) => { this.newUid = v }).margin({ bottom: 8 })
          TextInput({ placeholder: '姓名（必填）' }).onChange((v: string) => { this.newName = v }).margin({ bottom: 8 })
          TextInput({ placeholder: '学号（选填）' }).onChange((v: string) => { this.newSid = v }).margin({ bottom: 14 })
          Row() { Button('取消').backgroundColor('#EEE').fontColor('#666').layoutWeight(1).onClick(() => { this.showAdd = false }); Blank().width(10); Button('确认').backgroundColor('#1565C0').fontColor('#FFF').layoutWeight(1).onClick(() => { this.doAddCard() }) }.width('100%')
        }.padding(20).backgroundColor('#FFF').borderRadius(14).width('82%').shadow({ radius: 12, color: '#A0000000', offsetY: 6 })
      }.width('100%').height('100%').backgroundColor('#80000000').justifyContent(FlexAlign.Center).alignItems(HorizontalAlign.Center).onClick(() => { this.showAdd = false })
    }
  }

  // ==================== 工具 ====================
  acolor(t: string): ResourceColor { if (t === 'PIR') return '#FF9800'; if (t === 'TEMP') return '#F44336'; if (t === 'HUMIDITY') return '#2196F3'; if (t === 'UNAUTHORIZED') return '#9C27B0'; return '#757575' }
  emoji(t: string): string { if (t === 'PIR') return '🚶'; if (t === 'TEMP') return '🔥'; if (t === 'HUMIDITY') return '💧'; if (t === 'UNAUTHORIZED') return '🚫'; return '⚠️' }
}
