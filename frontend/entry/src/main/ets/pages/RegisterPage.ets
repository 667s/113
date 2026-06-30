/**
 * 注册页面
 * POST /api/auth/register → 成功 → 跳转登录页
 */
import { router } from '@kit.ArkUI'
import { http } from '@kit.NetworkKit'
import { getCaPath } from '../common/AuthUtil'

const BASE: string = 'https://10.219.46.147:8443'

@Entry
@Component
struct RegisterPage {
  @State username: string = ''
  @State password: string = ''
  @State passwordConfirm: string = ''
  @State displayName: string = ''
  @State errorMsg: string = ''
  @State loading: boolean = false

  build(): void {
    Column() {
      // 顶栏
      Row() {
        Text('← 返回')
          .fontSize(15)
          .fontColor('#1565C0')
          .onClick(() => { router.back() })
        Blank()
        Text('注册账号')
          .fontSize(18)
          .fontWeight(FontWeight.Bold)
      }
      .width('100%')
      .padding(16)
      .backgroundColor('#FFFFFF')

      // 表单区
      Column() {
        Text('创建您的账号')
          .fontSize(15)
          .fontColor('#333333')
          .width('100%')
          .padding({ top: 20, bottom: 16 } as Padding)

        // 显示名
        TextInput({ placeholder: '显示名（选填）' })
          .onChange((v: string) => { this.displayName = v })
          .margin({ bottom: 12 } as Margin)

        // 用户名
        TextInput({ placeholder: '用户名（必填）' })
          .onChange((v: string) => { this.username = v })
          .margin({ bottom: 12 } as Margin)

        // 密码
        TextInput({ placeholder: '密码（至少6位）' })
          .type(InputType.Password)
          .onChange((v: string) => { this.password = v })
          .margin({ bottom: 12 } as Margin)

        // 确认密码
        TextInput({ placeholder: '确认密码' })
          .type(InputType.Password)
          .onChange((v: string) => { this.passwordConfirm = v })
          .margin({ bottom: 16 } as Margin)

        // 错误提示
        if (this.errorMsg.length > 0) {
          Text(this.errorMsg)
            .fontSize(13)
            .fontColor('#F44336')
            .width('100%')
            .padding({ bottom: 12 } as Padding)
        }

        // 注册按钮
        Button(this.loading ? '注册中...' : '注册')
          .width('100%')
          .fontSize(16)
          .backgroundColor('#1565C0')
          .fontColor('#FFFFFF')
          .borderRadius(8)
          .enabled(!this.loading)
          .onClick(() => { this.doRegister() })

        // 去登录
        Row() {
          Text('已有账号？')
            .fontSize(13)
            .fontColor('#999999')
          Text('去登录')
            .fontSize(13)
            .fontColor('#1565C0')
            .onClick(() => { router.back() })
        }
        .width('100%')
        .justifyContent(FlexAlign.Center)
        .margin({ top: 16 } as Margin)
      }
      .width('100%')
      .padding({ left: 24, right: 24 } as Padding)
      .layoutWeight(1)
    }
    .width('100%')
    .height('100%')
    .backgroundColor('#F5F5F5')
  }

  async doRegister(): Promise<void> {
    // 校验
    if (!this.username.trim()) {
      this.errorMsg = '请输入用户名'
      return
    }
    if (this.password.length < 6) {
      this.errorMsg = '密码长度不能少于6位'
      return
    }
    if (this.password !== this.passwordConfirm) {
      this.errorMsg = '两次密码输入不一致'
      return
    }

    this.loading = true
    this.errorMsg = ''

    try {
      let caPath = await getCaPath()

      let r = http.createHttp()
      let body: Record<string, string> = {
        'username': this.username.trim(),
        'password': this.password
      }
      if (this.displayName.trim()) {
        body['displayName'] = this.displayName.trim()
      }

      let opts: http.HttpRequestOptions = {
        method: http.RequestMethod.POST,
        header: { 'Content-Type': 'application/json' },
        extraData: JSON.stringify(body),
        connectTimeout: 5000,
        readTimeout: 5000
      }
      if (caPath) opts.caPath = caPath
      let res = await r.request(BASE + '/api/auth/register', opts)
      r.destroy()

      if (res.responseCode === 200) {
        // 注册成功 → 跳回登录页
        router.back()
      } else {
        let data = JSON.parse(res.result as string) as Record<string, string>
        this.errorMsg = data['error'] || '注册失败'
      }
    } catch (e) {
      this.errorMsg = '网络错误，请检查后端是否启动'
    }
    this.loading = false
  }
}
