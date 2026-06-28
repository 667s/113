/**
 * 宿舍安防 — 4 Tab 应用
 * 1.传感器  2.刷卡记录  3.告警日志  4.授权卡管理
 */
import { http } from '@kit.NetworkKit'

// ==================== 数据模型 ====================
class RfidRecord {
  id: number = 0
  cardUid: string = ''
  holderName: string = ''
  isAuthorized: boolean = false
  scanTime: string = ''
}

class AlarmLog {
  id: number = 0
  alarmType: string = ''
  message: string = ''
  triggeredAt: string = ''
}

class AuthorizedCard {
  id: number = 0
  cardUid: string = ''
  holderName: string = ''
  studentId: string = ''
  isActive: boolean = true
}

interface AddCardBody {
  cardUid: string
  holderName: string
  studentId: string
}

interface StatusBody {
  isActive: boolean
}

// ==================== 常量 ====================
const BASE: string = 'http://172.18.0.153:8080'
const TAB_NAMES: string[] = ['传感器', '刷卡记录', '告警日志', '卡片管理']

// ==================== 主页面 ====================
@Entry
@Component
struct Index {
  @State tab: number = 0

  // ---- 传感器 ----
  @State temperature: number = 0
  @State humidity: number = 0
  @State dhtTime: string = ''
  @State pirDetected: boolean = false
  @State pirTime: string = ''

  // ---- 刷卡记录 ----
  @State rfidList: RfidRecord[] = []
  @State rfidPage: number = 0
  @State rfidEnd: boolean = false
  @State rfidLoading: boolean = false
  @State rfidErr: string = ''

  // ---- 告警日志 ----
  @State alarmList: AlarmLog[] = []
  @State alarmPage: number = 0
  @State alarmEnd: boolean = false
  @State alarmLoading: boolean = false
  @State alarmFilter: string = ''
  @State alarmErr: string = ''

  // ---- 卡片管理 ----
  @State cards: AuthorizedCard[] = []
  @State showAdd: boolean = false
  @State newUid: string = ''
  @State newName: string = ''
  @State newSid: string = ''

  aboutToAppear(): void {
    this.loadSensor()
    this.loadRfid(true)
    this.loadAlarms(true)
    this.loadCards()
    // 每2秒轮询传感器 + 刷卡最新 + 告警最新
    this.startPolling()
  }

  // 定时器句柄
  private pollTimer: number = -1

  startPolling(): void {
    this.pollTimer = setInterval(() => {
      this.loadSensor()
      this.loadLatestRfid()
      this.loadLatestAlarms()
    }, 2000)
  }

  // 静默拉取最新刷卡 (不翻页，只插入新数据到列表头部)
  async loadLatestRfid(): Promise<void> {
    try {
      const res: string = await this.httpGet('/api/rfid/records/latest')
      const latest: RfidRecord[] = JSON.parse(res) as RfidRecord[]
      if (latest.length === 0 || this.rfidList.length === 0) {
        return
      }
      const lastId: number = this.rfidList[0].id
      const newItems: RfidRecord[] = []
      for (let i = 0; i < latest.length; i++) {
        if (latest[i].id > lastId) {
          newItems.push(latest[i])
        }
      }
      if (newItems.length > 0) {
        this.rfidList = newItems.concat(this.rfidList)
      }
    } catch (e) {
    }
  }

  // 静默拉取最新告警 (不翻页，只插入新数据到列表头部)
  async loadLatestAlarms(): Promise<void> {
    try {
      const res: string = await this.httpGet('/api/alarm/logs/latest')
      const latest: AlarmLog[] = JSON.parse(res) as AlarmLog[]
      if (latest.length === 0 || this.alarmList.length === 0) {
        return
      }
      const lastId: number = this.alarmList[0].id
      const newItems: AlarmLog[] = []
      for (let i = 0; i < latest.length; i++) {
        if (latest[i].id > lastId) {
          newItems.push(latest[i])
        }
      }
      if (newItems.length > 0) {
        this.alarmList = newItems.concat(this.alarmList)
      }
    } catch (e) {
    }
  }

  // ==================== 网络 ====================
  async httpGet(path: string): Promise<string> {
    const r: http.HttpRequest = http.createHttp()
    const res: http.HttpResponse = await r.request(BASE + path, {
      method: http.RequestMethod.GET,
      connectTimeout: 5000,
      readTimeout: 5000
    })
    r.destroy()
    if (res.responseCode !== 200) {
      throw new Error('HTTP ' + res.responseCode)
    }
    return res.result as string
  }

  async httpPost(path: string, body: Object): Promise<string> {
    const r: http.HttpRequest = http.createHttp()
    const h: Record<string, string> = {}
    h['Content-Type'] = 'application/json'
    const res: http.HttpResponse = await r.request(BASE + path, {
      method: http.RequestMethod.POST,
      header: h,
      extraData: JSON.stringify(body),
      connectTimeout: 5000,
      readTimeout: 5000
    })
    r.destroy()
    return res.result as string
  }

  async httpPut(path: string, body: Object): Promise<string> {
    const r: http.HttpRequest = http.createHttp()
    const h: Record<string, string> = {}
    h['Content-Type'] = 'application/json'
    const res: http.HttpResponse = await r.request(BASE + path, {
      method: http.RequestMethod.PUT,
      header: h,
      extraData: JSON.stringify(body),
      connectTimeout: 5000,
      readTimeout: 5000
    })
    r.destroy()
    return res.result as string
  }

  async httpDel(path: string): Promise<string> {
    const r: http.HttpRequest = http.createHttp()
    const res: http.HttpResponse = await r.request(BASE + path, {
      method: http.RequestMethod.DELETE,
      connectTimeout: 5000,
      readTimeout: 5000
    })
    r.destroy()
    return res.result as string
  }

  // ==================== 传感器 ====================
  async loadSensor(): Promise<void> {
    try {
      const dht: string = await this.httpGet('/api/sensor/dht/latest')
      const d: Record<string, Object> = JSON.parse(dht) as Record<string, Object>
      this.temperature = d['temperature'] as number
      this.humidity = d['humidity'] as number
      this.dhtTime = (d['recordedAt'] as string) || ''
    } catch (e) {
      this.temperature = 0
      this.humidity = 0
    }
    try {
      const pir: string = await this.httpGet('/api/sensor/pir/status')
      const p: Record<string, Object> = JSON.parse(pir) as Record<string, Object>
      this.pirDetected = (p['detected'] as boolean) || false
      this.pirTime = (p['lastTime'] as string) || ''
    } catch (e) {
      this.pirDetected = false
    }
  }

  // ==================== 刷卡记录 ====================
  async loadRfid(refresh: boolean): Promise<void> {
    if (this.rfidLoading) {
      return
    }
    this.rfidLoading = true
    this.rfidErr = ''
    const page: number = refresh ? 0 : this.rfidPage
    try {
      const res: string = await this.httpGet('/api/rfid/records?page=' + String(page) + '&size=20')
      const data: Record<string, Object> = JSON.parse(res) as Record<string, Object>
      const list: RfidRecord[] = data['content'] as RfidRecord[]
      const tp: number = data['totalPages'] as number
      if (refresh) {
        this.rfidList = list
      } else {
        this.rfidList = this.rfidList.concat(list)
      }
      this.rfidPage = page + 1
      this.rfidEnd = this.rfidPage >= tp
    } catch (e) {
      this.rfidErr = '加载失败'
      this.rfidEnd = true
    }
    this.rfidLoading = false
  }

  // ==================== 告警日志 ====================
  async loadAlarms(refresh: boolean): Promise<void> {
    if (this.alarmLoading) {
      return
    }
    this.alarmLoading = true
    this.alarmErr = ''
    const page: number = refresh ? 0 : this.alarmPage
    let path: string = '/api/alarm/logs?page=' + String(page) + '&size=20'
    if (this.alarmFilter.length > 0) {
      path = path + '&type=' + this.alarmFilter
    }
    try {
      const res: string = await this.httpGet(path)
      const data: Record<string, Object> = JSON.parse(res) as Record<string, Object>
      const list: AlarmLog[] = data['content'] as AlarmLog[]
      const tp: number = data['totalPages'] as number
      if (refresh) {
        this.alarmList = list
      } else {
        this.alarmList = this.alarmList.concat(list)
      }
      this.alarmPage = page + 1
      this.alarmEnd = this.alarmPage >= tp
    } catch (e) {
      this.alarmErr = '加载失败'
      this.alarmEnd = true
    }
    this.alarmLoading = false
  }

  setAlarmFilter(t: string): void {
    this.alarmFilter = t
    this.alarmPage = 0
    this.alarmList = []
    this.alarmEnd = false
    this.loadAlarms(true)
  }

  // ==================== 卡片管理 ====================
  async loadCards(): Promise<void> {
    try {
      const res: string = await this.httpGet('/api/cards')
      this.cards = JSON.parse(res) as AuthorizedCard[]
    } catch (e) {
    }
  }

  async doAddCard(): Promise<void> {
    if (!this.newUid.trim() || !this.newName.trim()) {
      return
    }
    try {
      const body: AddCardBody = {
        cardUid: this.newUid.trim(),
        holderName: this.newName.trim(),
        studentId: this.newSid.trim()
      }
      await this.httpPost('/api/cards', body)
      this.showAdd = false
      this.newUid = ''
      this.newName = ''
      this.newSid = ''
      this.loadCards()
    } catch (e) {
    }
  }

  async doToggleCard(card: AuthorizedCard): Promise<void> {
    try {
      const body: StatusBody = { isActive: !card.isActive }
      await this.httpPut('/api/cards/' + String(card.id) + '/status', body)
      this.loadCards()
    } catch (e) {
    }
  }

  async doDeleteCard(card: AuthorizedCard): Promise<void> {
    try {
      await this.httpDel('/api/cards/' + String(card.id))
      this.loadCards()
    } catch (e) {
    }
  }

  // ==================== UI 总框架 ====================
  build(): void {
    Column() {
      // 顶栏标题
      Row() {
        Text('宿舍安防')
          .fontSize(20)
          .fontWeight(FontWeight.Bold)
          .fontColor('#FFFFFF')
      }
      .width('100%')
      .height(50)
      .backgroundColor('#1565C0')
      .justifyContent(FlexAlign.Center)

      // 内容区
      Tabs({ index: this.tab }) {
        TabContent() { this.SensorTab() }
        TabContent() { this.RfidTab() }
        TabContent() { this.AlarmTab() }
        TabContent() { this.CardTab() }
      }
      .barHeight(0)
      .layoutWeight(1)
      .onChange((i: number) => { this.tab = i })

      // 底部导航
      Row() {
        ForEach(TAB_NAMES, (label: string, i: number) => {
          Column() {
            Text(this.tabIcon(i))
              .fontSize(20)
            Text(label)
              .fontSize(11)
              .fontColor(this.tab === i ? '#1565C0' : '#999999')
              .margin({ top: 3 } as Margin)
          }
          .width('25%')
          .padding(6)
          .onClick(() => { this.tab = i })
        })
      }
      .width('100%')
      .height(52)
      .backgroundColor('#FFFFFF')
    }
    .width('100%')
    .height('100%')
    .backgroundColor('#F5F5F5')
  }

  tabIcon(i: number): string {
    if (i === 0) return '🌡️'
    if (i === 1) return '💳'
    if (i === 2) return '🚨'
    return '🔑'
  }

  // ==================== 1. 传感器 Tab ====================
  @Builder SensorTab(): void {
    Scroll() {
      Column() {
        // 温湿度双卡片
        Row() {
          Column() {
            Text('🌡️').fontSize(36)
            Text(this.temperature ? this.temperature.toFixed(1) + '°C' : '-- °C')
              .fontSize(28)
              .fontWeight(FontWeight.Bold)
              .fontColor('#E65100')
              .margin({ top: 6 } as Margin)
            Text('温度')
              .fontSize(13)
              .fontColor('#999999')
              .margin({ top: 4 } as Margin)
          }
          .width('47%')
          .height(140)
          .justifyContent(FlexAlign.Center)
          .backgroundColor('#FFF3E0')
          .borderRadius(12)

          Blank().width('6%')

          Column() {
            Text('💧').fontSize(36)
            Text(this.humidity ? this.humidity.toFixed(1) + '%' : '-- %')
              .fontSize(28)
              .fontWeight(FontWeight.Bold)
              .fontColor('#0D47A1')
              .margin({ top: 6 } as Margin)
            Text('湿度')
              .fontSize(13)
              .fontColor('#999999')
              .margin({ top: 4 } as Margin)
          }
          .width('47%')
          .height(140)
          .justifyContent(FlexAlign.Center)
          .backgroundColor('#E3F2FD')
          .borderRadius(12)
        }
        .width('100%')
        .padding(12)
        .justifyContent(FlexAlign.Center)

        // PIR 人体检测卡片
        Row() {
          Column() {
            Text('🚶').fontSize(36)
            Text(this.pirDetected ? '有人' : '无人')
              .fontSize(22)
              .fontWeight(FontWeight.Bold)
              .fontColor(this.pirDetected ? '#E65100' : '#388E3C')
              .margin({ top: 6 } as Margin)
            Text('人体检测')
              .fontSize(13)
              .fontColor('#999999')
              .margin({ top: 4 } as Margin)
          }
          .width('100%')
          .height(120)
          .justifyContent(FlexAlign.Center)
          .backgroundColor(this.pirDetected ? '#FFF3E0' : '#E8F5E9')
          .borderRadius(12)
        }
        .width('100%')
        .padding(12)

        // 提示
        Text('下拉刷新获取最新数据')
          .fontSize(12)
          .fontColor('#CCCCCC')
          .width('100%')
          .textAlign(TextAlign.Center)
          .padding(16)

        Blank().height(40)
      }
    }
    .scrollBar(BarState.Off)
    .width('100%')
    .height('100%')
    .onAppear(() => { this.loadSensor() })
  }

  // ==================== 2. 刷卡记录 Tab ====================
  @Builder RfidTab(): void {
    Column() {
      // 标题栏
      Row() {
        Text('刷卡记录')
          .fontSize(17)
          .fontWeight(FontWeight.Bold)
        Blank()
        Text('共 ' + String(this.rfidList.length) + ' 条')
          .fontSize(12)
          .fontColor('#999999')
      }
      .width('100%')
      .padding(12)
      .backgroundColor('#FFFFFF')

      // 列表
      if (this.rfidList.length === 0 && this.rfidLoading) {
        Column() {
          LoadingProgress().width(24).height(24).color('#1565C0')
          Text('加载中...').fontSize(13).fontColor('#999999').margin({ top: 8 } as Margin)
        }
        .width('100%')
        .layoutWeight(1)
        .justifyContent(FlexAlign.Center)
      } else if (this.rfidList.length === 0 && this.rfidErr.length > 0) {
        Column() {
          Text('⚠️').fontSize(40)
          Text(this.rfidErr).fontSize(14).fontColor('#F44336').margin({ top: 8 } as Margin)
        }
        .width('100%')
        .layoutWeight(1)
        .justifyContent(FlexAlign.Center)
        .onClick(() => { this.loadRfid(true) })
      } else {
        List() {
          ForEach(this.rfidList, (r: RfidRecord) => {
            ListItem() {
              Row() {
                Column() {
                  Text(r.holderName ? r.holderName : '未知用户')
                    .fontSize(15)
                    .fontWeight(FontWeight.Medium)
                    .fontColor('#333333')
                  Text('UID: ' + r.cardUid)
                    .fontSize(11)
                    .fontColor('#999999')
                    .margin({ top: 2 } as Margin)
                  Text(r.scanTime)
                    .fontSize(10)
                    .fontColor('#AAAAAA')
                    .margin({ top: 2 } as Margin)
                }
                .alignItems(HorizontalAlign.Start)
                .layoutWeight(1)

                Text(r.isAuthorized ? '✓ 已授权' : '✗ 未授权')
                  .fontSize(12)
                  .fontColor('#FFFFFF')
                  .backgroundColor(r.isAuthorized ? '#4CAF50' : '#F44336')
                  .borderRadius(4)
                  .padding({ left: 8, right: 8, top: 4, bottom: 4 } as Padding)
              }
              .width('100%')
              .padding(12)
              .backgroundColor('#FFFFFF')
              .borderRadius(8)
              .margin({ left: 12, right: 12, top: 4 } as Margin)
            }
          })

          // 触底加载
          if (!this.rfidEnd) {
            ListItem() {
              Row() {
                LoadingProgress().width(16).height(16).color('#1565C0')
                Text('加载更多...')
                  .fontSize(12)
                  .fontColor('#999999')
                  .margin({ left: 6 } as Margin)
              }
              .width('100%')
              .justifyContent(FlexAlign.Center)
              .padding(14)
            }
            .onAppear(() => { this.loadRfid(false) })
          }

          ListItem() {
            Text('已加载全部')
              .fontSize(11)
              .fontColor('#CCCCCC')
              .width('100%')
              .textAlign(TextAlign.Center)
              .padding(14)
          }
        }
        .scrollBar(BarState.Off)
        .layoutWeight(1)
      }
    }
    .width('100%')
    .height('100%')
  }

  // ==================== 3. 告警日志 Tab ====================
  @Builder AlarmTab(): void {
    Column() {
      // 类型筛选栏
      Scroll() {
        Row() {
          ForEach(['全部', 'PIR', 'TEMP', 'HUMIDITY', 'UNAUTHORIZED'], (t: string) => {
            Text(t)
              .fontSize(12)
              .fontColor(this.isFilterActive(t) ? '#FFFFFF' : '#666666')
              .backgroundColor(this.isFilterActive(t) ? '#1565C0' : '#FFFFFF')
              .borderRadius(16)
              .border({ width: 1, color: '#E0E0E0' } as BorderOptions)
              .padding({ left: 14, right: 14, top: 6, bottom: 6 } as Padding)
              .margin({ right: 8 } as Margin)
              .onClick(() => {
                this.setAlarmFilter(t === '全部' ? '' : t)
              })
          })
        }
        .padding({ left: 12, right: 12, top: 8, bottom: 8 } as Padding)
      }
      .scrollBar(BarState.Off)
      .backgroundColor('#FFFFFF')

      // 列表
      if (this.alarmList.length === 0 && this.alarmLoading) {
        Column() {
          LoadingProgress().width(24).height(24).color('#1565C0')
          Text('加载中...').fontSize(13).fontColor('#999999').margin({ top: 8 } as Margin)
        }
        .width('100%')
        .layoutWeight(1)
        .justifyContent(FlexAlign.Center)
      } else if (this.alarmList.length === 0 && this.alarmErr.length > 0) {
        Column() {
          Text('⚠️').fontSize(40)
          Text(this.alarmErr).fontSize(14).fontColor('#F44336').margin({ top: 8 } as Margin)
        }
        .width('100%')
        .layoutWeight(1)
        .justifyContent(FlexAlign.Center)
        .onClick(() => { this.loadAlarms(true) })
      } else if (this.alarmList.length === 0) {
        Column() {
          Text('✅').fontSize(40)
          Text('暂无告警').fontSize(14).fontColor('#999999').margin({ top: 8 } as Margin)
        }
        .width('100%')
        .layoutWeight(1)
        .justifyContent(FlexAlign.Center)
      } else {
        List() {
          ForEach(this.alarmList, (a: AlarmLog) => {
            ListItem() {
              Row() {
                // 类型图标
                Column() {
                  Text(this.alarmEmoji(a.alarmType)).fontSize(26)
                }
                .width(42)
                .height(42)
                .borderRadius(21)
                .backgroundColor(this.alarmBg(a.alarmType))
                .justifyContent(FlexAlign.Center)

                // 内容
                Column() {
                  Text(a.message)
                    .fontSize(14)
                    .fontColor('#333333')
                    .maxLines(2)
                    .textOverflow({ overflow: TextOverflow.Ellipsis } as TextOverflowOptions)
                  Text(a.triggeredAt)
                    .fontSize(11)
                    .fontColor('#AAAAAA')
                    .margin({ top: 4 } as Margin)
                }
                .alignItems(HorizontalAlign.Start)
                .layoutWeight(1)
                .margin({ left: 10 } as Margin)

                // 类型标签
                Text(a.alarmType)
                  .fontSize(10)
                  .fontColor('#FFFFFF')
                  .backgroundColor(this.alarmTag(a.alarmType))
                  .borderRadius(4)
                  .padding({ left: 6, right: 6, top: 2, bottom: 2 } as Padding)
              }
              .width('100%')
              .padding(12)
              .backgroundColor('#FFFFFF')
              .borderRadius(8)
              .margin({ left: 12, right: 12, top: 4 } as Margin)
            }
          })

          if (!this.alarmEnd) {
            ListItem() {
              Row() {
                LoadingProgress().width(16).height(16).color('#F44336')
                Text('加载更多...')
                  .fontSize(12)
                  .fontColor('#999999')
                  .margin({ left: 6 } as Margin)
              }
              .width('100%')
              .justifyContent(FlexAlign.Center)
              .padding(14)
            }
            .onAppear(() => { this.loadAlarms(false) })
          }

          ListItem() {
            Text('已加载全部')
              .fontSize(11)
              .fontColor('#CCCCCC')
              .width('100%')
              .textAlign(TextAlign.Center)
              .padding(14)
          }
        }
        .scrollBar(BarState.Off)
        .layoutWeight(1)
      }
    }
    .width('100%')
    .height('100%')
  }

  isFilterActive(t: string): boolean {
    if (t === '全部' && this.alarmFilter === '') {
      return true
    }
    return t === this.alarmFilter
  }

  // ==================== 4. 卡片管理 Tab ====================
  @Builder CardTab(): void {
    Stack() {
      List() {
        // 标题
        ListItem() {
          Row() {
            Text('授权卡管理')
              .fontSize(17)
              .fontWeight(FontWeight.Bold)
            Blank()
            Text('共 ' + String(this.cards.length) + ' 张')
              .fontSize(12)
              .fontColor('#999999')
          }
          .width('100%')
          .padding(12)
          .backgroundColor('#FFFFFF')
        }

        if (this.cards.length === 0) {
          ListItem() {
            Column() {
              Text('🔑').fontSize(48).opacity(0.25)
              Text('暂无授权卡').fontSize(14).fontColor('#999999').margin({ top: 8 } as Margin)
              Text('点击右下角 + 添加新卡').fontSize(11).fontColor('#CCCCCC').margin({ top: 4 } as Margin)
            }
            .width('100%')
            .padding(60)
            .alignItems(HorizontalAlign.Center)
          }
        }

        ForEach(this.cards, (c: AuthorizedCard) => {
          ListItem() {
            Row() {
              // 左侧信息
              Column() {
                Text(c.holderName)
                  .fontSize(15)
                  .fontWeight(FontWeight.Medium)
                  .fontColor('#333333')
                Text('UID: ' + c.cardUid)
                  .fontSize(11)
                  .fontColor('#999999')
                  .margin({ top: 2 } as Margin)
                if (c.studentId) {
                  Text('学号: ' + c.studentId)
                    .fontSize(11)
                    .fontColor('#999999')
                    .margin({ top: 2 } as Margin)
                }
              }
              .alignItems(HorizontalAlign.Start)
              .layoutWeight(1)

              // 右侧状态 + 操作
              Column() {
                Text(c.isActive ? '已启用' : '已停用')
                  .fontSize(11)
                  .fontColor('#FFFFFF')
                  .backgroundColor(c.isActive ? '#4CAF50' : '#9E9E9E')
                  .borderRadius(4)
                  .padding({ left: 8, right: 8, top: 3, bottom: 3 } as Padding)
                Row() {
                  Text(c.isActive ? '停用' : '启用')
                    .fontSize(11)
                    .fontColor('#1565C0')
                    .padding({ left: 4, right: 4, top: 4, bottom: 4 } as Padding)
                    .onClick(() => { this.doToggleCard(c) })
                  Text('删除')
                    .fontSize(11)
                    .fontColor('#F44336')
                    .padding({ left: 4, right: 4, top: 4, bottom: 4 } as Padding)
                    .margin({ left: 4 } as Margin)
                    .onClick(() => { this.doDeleteCard(c) })
                }
                .margin({ top: 6 } as Margin)
              }
              .alignItems(HorizontalAlign.End)
            }
            .width('100%')
            .padding(12)
            .backgroundColor('#FFFFFF')
            .borderRadius(8)
            .margin({ left: 12, right: 12, top: 4 } as Margin)
          }
        })
      }
      .scrollBar(BarState.Off)
      .width('100%')
      .height('100%')
      .padding({ bottom: 80 } as Padding)

      // 添加按钮
      Button('＋')
        .fontSize(28)
        .width(56)
        .height(56)
        .borderRadius(28)
        .backgroundColor('#1565C0')
        .fontColor('#FFFFFF')
        .position({ x: '82%', y: '88%' } as Position)
        .onClick(() => { this.showAdd = true })
    }
    .width('100%')
    .height('100%')

    // ========== 添加对话框 ==========
    if (this.showAdd) {
      Column() {
        Column() {
          Text('添加授权卡')
            .fontSize(18)
            .fontWeight(FontWeight.Bold)
            .width('100%')
            .padding({ bottom: 16 } as Padding)

          TextInput({ placeholder: '卡号 UID（必填）' })
            .onChange((v: string) => { this.newUid = v })
            .margin({ bottom: 10 } as Margin)

          TextInput({ placeholder: '持卡人姓名（必填）' })
            .onChange((v: string) => { this.newName = v })
            .margin({ bottom: 10 } as Margin)

          TextInput({ placeholder: '学号（选填）' })
            .onChange((v: string) => { this.newSid = v })
            .margin({ bottom: 18 } as Margin)

          Row() {
            Button('取消')
              .backgroundColor('#F5F5F5')
              .fontColor('#666666')
              .layoutWeight(1)
              .onClick(() => { this.showAdd = false })
            Blank().width(12)
            Button('确认添加')
              .backgroundColor('#1565C0')
              .fontColor('#FFFFFF')
              .layoutWeight(1)
              .onClick(() => { this.doAddCard() })
          }
          .width('100%')
        }
        .padding(24)
        .backgroundColor('#FFFFFF')
        .borderRadius(16)
        .width('85%')
        .shadow({ radius: 16, color: '#30000000', offsetY: 8 } as ShadowOptions)
      }
      .width('100%')
      .height('100%')
      .backgroundColor('#50000000')
      .justifyContent(FlexAlign.Center)
      .alignItems(HorizontalAlign.Center)
    }
  }

  // ==================== 告警颜色工具 ====================
  alarmEmoji(t: string): string {
    if (t === 'PIR') return '🚶'
    if (t === 'TEMP') return '🔥'
    if (t === 'HUMIDITY') return '💧'
    if (t === 'UNAUTHORIZED') return '🚫'
    return '⚠️'
  }

  alarmBg(t: string): ResourceColor {
    if (t === 'PIR') return '#FFF3E0'
    if (t === 'TEMP') return '#FFEBEE'
    if (t === 'HUMIDITY') return '#E3F2FD'
    if (t === 'UNAUTHORIZED') return '#F3E5F5'
    return '#F5F5F5'
  }

  alarmTag(t: string): ResourceColor {
    if (t === 'PIR') return '#FF9800'
    if (t === 'TEMP') return '#F44336'
    if (t === 'HUMIDITY') return '#2196F3'
    if (t === 'UNAUTHORIZED') return '#9C27B0'
    return '#757575'
  }
}
