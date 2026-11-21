# 小狼毫 · 多行候选「行焦点选词」版（个人二次开发）

> **本仓库不是官方发布版。** 它是基于 [rime/weasel](https://github.com/rime/weasel) 0.17.4 源码的
> 个人二次开发分支，只做了一件事：让 Windows 上的小狼毫也能像 macOS 系统拼音输入法那样，
> 在多行候选面板里**只用键盘数字键**就能选中任意一个字词并上屏。
>
> 上游原版：【小狼毫】输入法，基于 中州韵输入法引擎 / Rime Input Method Engine 等开源技术，
> 式恕堂「版权所无」。授权条款 GPLv3，项目主页 https://rime.im 。
> 本仓库沿用同一授权，上游版权声明与致谢信息全部保留。

[![Build Multiline Installer](https://github.com/Fanke-GH/weasel-multiline/actions/workflows/build-multiline.yml/badge.svg?branch=feature/multiline-candidate)](https://github.com/Fanke-GH/weasel-multiline/actions/workflows/build-multiline.yml)

| 项目 | 说明 |
|---|---|
| 开发分支 | `feature/multiline-candidate`（向此分支推送即自动云端编译） |
| 源码基线 | rime/weasel 0.17.4 |
| 适用系统 | Windows 8.1 ~ Windows 11 |
| 授权 | GPLv3（与上游一致） |
| 上游下载 / 文档 | https://github.com/rime/weasel 、https://rime.im |

---

## 一、这个版本改了什么

原版小狼毫的多行候选是「一页铺开」，序号 1~9、0 只覆盖前 10 个，第 11 个之后只能靠鼠标
或方向键逐个移动。本版改为 **行焦点 + 行内数字键**：

| 按键 | 行为 |
|---|---|
| 输入拼音 | 只显示 1 行（8 个候选） |
| `.`（首次按下） | 展开为 5 行 × 8 列，焦点落在第 1 行 |
| `.` | 焦点下移一行；已在第 5 行 → 翻到下一候选页，焦点回第 1 行 |
| `,` | 焦点上移一行；已在第 1 行 → 回上一候选页并停在其最后一行；第一页首行 → 收起回单行 |
| `1` ~ `8` | 选中**当前焦点行**的第 N 个候选并上屏 |
| 继续输入字母 | 编码变化，自动收起回单行 |
| `↑` `↓` / `PageUp` `PageDown` | 等价于 `,` / `.` 的备用键 |
| 没有候选时 | `.`、`,` 与数字键交回输入内核，可正常打出标点和数字 |

每页 8 × 5 = 40 个候选，**每一个都能用数字键直达**。序号风格可切换：
`focus_only`（只给焦点行标号，macOS 系统拼音风格）或 `all_rows`（每行都标号）。

改动集中在以下文件，librime、几何布局与 Direct2D 绘制代码未改动：

- `RimeWithWeasel/RimeWithWeasel.cpp`、`include/RimeWithWeasel.h`：行焦点状态机、按键拦截与候选信息生成
- `include/WeaselIPCData.h`：新增焦点行、每行个数、`.` / `,` 翻页键字段及 boost 序列化
- `WeaselUI/WeaselPanel.cpp`：`all_rows` 模式下焦点行淡高亮
- `WeaselDeployer/UIStyleSettings.cpp`、`include/WeaselStyleKeys.h`：读取新增配置项
- `output/data/weasel.yaml`：默认配置注释
- `build.bat`、`.gitignore`、`output/rime-install.bat`：云端构建与 NSIS 打包所需的补齐

---

## 二、配置

用户词库与配置文件位于 `%AppData%\Rime`（托盘右键 →「用户文件夹」可打开）。

`weasel.custom.yaml` 的 `patch:` 下（本次实际采用：14pt、霞鹜文楷 GB、演示页配色）：

```yaml
patch:
  "style/font_point": 14
  "style/font_face": "霞鹜文楷 GB"
  "style/label_font_face": "霞鹜文楷 GB"
  "style/comment_font_face": "霞鹜文楷 GB"
  "style/color_scheme": row_focus_demo
  "style/color_scheme_dark": row_focus_demo
  "style/layout/type": horizontal+multiline
  "style/layout/candidate_count_per_row": 8
  "style/layout/max_rows": 5
  "style/layout/row_focus": true
  "style/layout/label_mode": focus_only
  "style/layout/page_down_key": "."
  "style/layout/page_up_key": ","
  "style/hover_type": none
```

同时必须让 `menu/page_size` = 8 × 5 = **40**（写在 `default.custom.yaml` 及各输入方案的
`.custom.yaml`；若方案文件里写死了 6，全局设置不生效，需单独覆写）。

约束与注意：

- `style/hover_type` 保持 `none`，否则悬停高亮会盖掉焦点行高亮。
- `page_down_key` / `page_up_key` 可换成其它**单个字符**，写 `none` 禁用；不要写数字 0~9。
- 修改配置后须托盘右键「重新部署」才生效。
- **不要用「外观设置」GUI 保存**：它会重写 `weasel.custom.yaml`，弄丢 `row_focus_demo` 自定义配色。
- 若原先装过 Lua 版多行候选（`multiline_nav_processor.lua`、`multirow_pad_filter.lua`），
  **必须停用其中一套**，两套逻辑会互相打架。

---

## 三、和电脑里已有的输入法是否冲突

- **原版小狼毫**：使用同一个 TSF CLSID，属于**替换升级**，不会多出一个输入法；安装程序只覆盖程序目录，不删除 `%AppData%\Rime` 里的用户数据。

装前建议先备份用户目录，并确认已停用 Lua 版多行候选。

---

## 四、云端构建（为什么不在本机编译）

小狼毫是 MSVC `.vcxproj` 工程，界面依赖 ATL/WTL + Direct2D，并链接官方预编译的 MSVC 版
librime 二进制 —— MinGW64 无法出包，只能用它做纯 C++ 逻辑验证。因此改用 GitHub Actions
托管的 `windows-2022` 机器编译：

1. 向 `feature/multiline-candidate` 分支 push，自动触发 **Build Multiline Installer**
   （也可在 Actions 页手动 Run workflow）；
2. 工作流依次执行 `install_boost.bat` → `build.bat data` → `get-rime.ps1 -use dev`
   → `build.bat arm64 installer` → NSIS 打包；
3. 约 20~40 分钟，在该次 Run 的 **Artifacts** 下载 `weasel-multiline-xxx`，
   解压即得到 `.exe` 安装包（另附 `debug_symbols.7z`，普通用户不用管）。
   Artifacts 默认保留约 90 天，请及时下载。

公共仓库的 Actions 不消耗付费额度。仓库里的源码长期保存，二进制只存在于各次 Run 的
Artifacts（或手动创建的 Release 附件）中。

---

## 五、安装与使用

适用于 Windows 8.1 ~ Windows 11。管理员运行安装包 → 按提示完成 → **重启或注销重登** →
托盘右键「重新部署」。初次安装会出现「安装选项」对话框，可在「输入语言」栏选择要注册到的
键盘布局；安装完成后仍可从开始菜单再次打开该对话框更改。

日常使用：

- 从输入法指示器里选择【中】字图标切换到小狼毫；
- <kbd>Ctrl+&#96;</kbd> 或 <kbd>F4</kbd> 呼出方案选单、切换输入方式；
- 开始菜单 » 小狼毫输入法 可访问设定工具与常用位置；
- 定制 Rime 的通用方法见 [《定制指南》](https://github.com/rime/home/wiki/CustomizationGuide)，
  Weasel 专属样式与行为见[上游 Wiki](https://github.com/rime/weasel/wiki)。

其他平台的 Rime 发行版：Linux 用 ibus-rime / fcitx5-rime / fcitx-rime，macOS 用【鼠须管】。

---

## 六、验证情况

| 验证项 | 结果 |
|---|---|
| 行焦点状态机台架 `row_focus_test.cpp` | 381 项断言通过 |
| v4 按键分类 / 放行 / 8 宽 40 候选可达 `page_key_test.cpp` | 623 项断言通过 |
| 真实源码新增函数配桩件 `g++ -fsyntax-only` | 无错误 |
| 补丁 `git apply --check`、演示页 `node --check`、装前配置 91 项检查 | 全部通过 |
| GitHub Actions 云端 MSVC 编译 + NSIS 打包 | 已通过（0 Error） |

尚需在真机实测的是 Direct2D 焦点行绘制与 TSF 按键签收的实际手感：重点看按 `.` 能否展开 /
下移 / 翻页、按 `,` 能否上移 / 回退 / 收起，以及**没有候选时按 `.` `,` 能否正常打出标点**。

---

## 七、授权与致谢

基于 [rime/weasel](https://github.com/rime/weasel) 0.17.4，沿用 **GPLv3**。
上游版权声明、贡献者名单与开源软件清单全部保留（`LICENSE.txt`、`CHANGELOG.md`）。

**输入方案设计**：【朙月拼音】系列及【八股文】词典（部分数据来源于 CC-CEDICT、
Android 拼音、新酷音、opencc 等开源项目，维护者 佛振、瑾昀）；【注音／地球拼音】
维护者 佛振、瑾昀；【仓颉五代】发明人 朱邦复先生，码表源自 www.chinesecj.com，
构词码表作者 惜缘。【五笔】【粤拼】【上海／苏州吴语】【中古汉语拼音】【国际音标】等方案
不再以安装包预装形式提供，可由 https://github.com/rime/plum 下载安装。

**程序设计**：佛振、邹旭、Xiangyan Sun、Prcuvu、nameoverflow、fxliang、Azuk 443，
以及[全部代码贡献者](https://github.com/rime/weasel/graphs/contributors)。

**美术**：图标设计 Patricivs；配色方案 Aben、P1461、Patricivs、skoj、佛振、五磅兔。

**引用的开源软件**（授权不变）：Boost C++ Libraries (Boost Software License)、
curl (MIT/X derivate)、google-glog (BSD-3-Clause)、Google Test (BSD-3-Clause)、
LevelDB (BSD-3-Clause)、librime (BSD-3-Clause)、marisa-trie (BSD-2-Clause / LGPL 2.1)、
OpenCC 开放中文转换 (Apache-2.0)、plum (LGPL-3.0)、WinSparkle (MIT)、
yaml-cpp (MIT)、7-Zip (LGPL-2.1+，含 unRAR 限制)。

本版额外使用了本机已安装的「霞鹜文楷 GB」字体，候选面板配色 `row_focus_demo`
参照交互演示页设定。

---

## 八、问题与反馈

- **本改造版**（行焦点选词、`.` / `,` 翻页、云端构建）的问题 → 本仓库 Issues：
  https://github.com/Fanke-GH/weasel-multiline/issues
- **原版小狼毫**的 bug → https://github.com/rime/weasel/issues ，
  也欢迎向 https://github.com/rime/weasel/pulls 提交 pull request
- **Rime 通用**功能、使用方法与配置问题 → https://github.com/rime/home/issues

技术交流见 [Rime 代码之家](https://github.com/rime/home)，或致信 Rime 开发者
rimeime@gmail.com。请勿把本改造版的问题提到上游仓库，以免给官方维护者添乱。

谢谢！
