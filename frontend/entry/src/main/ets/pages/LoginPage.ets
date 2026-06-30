/**
 * 登录页面
 * POST /api/auth/login → 拿到 JWT → 存本地 → 跳转主页
 */
import { router } from '@kit.ArkUI'
import { http } from '@kit.NetworkKit'
import { saveAuth, isLoggedIn, getCaPath } from '../common/AuthUtil'

const BASE: string = 'https://10.219.46.147:8443'

@Entry
@Component
struct LoginPage {
  @State username: string = ''
  @State password: string = ''
  @State errorMsg: string = ''
  @State loading: boolean = false

  aboutToAppear(): void {
    // 如果已登录，直接跳主页
    isLoggedIn().then((loggedIn: boolean) => {
      if (loggedIn) {
        router.replaceUrl({ url: 'pages/Index' })
      }
    })
  }

  build(): void {
    Column() {
      // Logo 区域
      Column() {
        Text('🏠')
          .fontSize(64)
        Text('宿舍安防系统')
          .fontSize(22)
          .fontWeight(FontWeight.Bold)
          .fontColor('#FFFFFF')
          .margin({ top: 12 } as Margin)
        Text('校园宿舍智能安防门')
          .fontSize(13)
          .fontColor('#BBDEFB')
          .margin({ top: 4 } as Margin)
      }
      .width('100%')
      .padding({ top: 60, bottom: 40 } as Padding)
      .backgroundColor('#1565C0')

      // 表单区
      Column() {
        // 用户名
        TextInput({ placeholder: '用户名' })
          .onChange((v: string) => { this.username = v })
          .margin({ bottom: 14 } as Margin)

        // 密码
        TextInput({ placeholder: '密码' })
          .type(InputType.Password)
          .onChange((v: string) => { this.password = v })
          .onSubmit(() => { this.doLogin() })
          .margin({ bottom: 8 } as Margin)

        // 错误提示
        if (this.errorMsg.length > 0) {
          Text(this.errorMsg)
            .fontSize(13)
            .fontColor('#F44336')
            .width('100%')
            .padding({ top: 4, bottom: 4 } as Padding)
        }

        // 登录按钮
        Button(this.loading ? '登录中...' : '登 录')
          .width('100%')
          .fontSize(16)
          .backgroundColor('#1565C0')
          .fontColor('#FFFFFF')
          .borderRadius(8)
          .enabled(!this.loading)
          .margin({ top: 20, bottom: 16 } as Margin)
          .onClick(() => { this.doLogin() })

        // 注册入口
        Row() {
          Text('没有账号？')
            .fontSize(13)
            .fontColor('#999999')
          Text('立即注册')
            .fontSize(13)
            .fontColor('#1565C0')
            .onClick(() => {
              router.pushUrl({ url: 'pages/RegisterPage' })
            })
        }
        .width('100%')
        .justifyContent(FlexAlign.Center)
      }
      .width('100%')
      .padding({ left: 32, right: 32, top: 30 } as Padding)
      .layoutWeight(1)
    }
    .width('100%')
    .height('100%')
    .backgroundColor('#F5F5F5')
  }

  async doLogin(): Promise<void> {
    if (!this.username.trim()) {
      this.errorMsg = '请输入用户名'
      return
    }
    if (!this.password) {
      this.errorMsg = '请输入密码'
      return
    }

    this.loading = true
    this.errorMsg = ''

    try {
      let caPath = await getCaPath()

      let r = http.createHttp()
      let opts: http.HttpRequestOptions = {
        method: http.RequestMethod.POST,
        header: { 'Content-Type': 'application/json' },
        extraData: JSON.stringify({
          username: this.username.trim(),
          password: this.password
        }),
        connectTimeout: 5000,
        readTimeout: 5000
      }
      if (caPath) opts.caPath = caPath
      let res = await r.request(BASE + '/api/auth/login', opts)
      r.destroy()

      if (res.responseCode === 200) {
        let data = JSON.parse(res.result as string) as Record<string, string>

        // 保存认证信息
        await saveAuth(
          data['token'] as string,
          data['username'] as string,
          data['displayName'] as string,
          data['role'] as string
        )

        // 跳转主页（替换，不允许返回登录页）
        router.replaceUrl({ url: 'pages/Index' })
      } else {
        let data = JSON.parse(res.result as string) as Record<string, string>
        this.errorMsg = data['error'] || '用户名或密码错误'
      }
    } catch (e) {
      this.errorMsg = '网络错误，请检查后端是否启动'
    }
    this.loading = false
  }
}
