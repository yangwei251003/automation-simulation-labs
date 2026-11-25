# TIA Portal V17 导入说明

以下面向已经安装 STEP 7、WinCC 和适用仿真组件的使用者。需要设置目标 CPU、HMI 设备和连接，再做相应的编译和运行检查。

1. 水系统使用 WaterControl.scl 编译控制块，再导入 graphics、WaterHmiTags.xml 和两个画面。浏览器 WaterSimulation.html 是独立的交互验证途径。
2. 红绿灯使用 PlcTags.xml、TrafficTimers.xml 和 Main.xml，再导入 graphics、HmiTags.xml 和 TrafficScreen.xml。确认四个相位灯的绑定。
3. 流水灯使用 Timers.xml 和 Main.xml，再导入 HmiTags.xml 和 Screen.xml。
4. 双路帧号能作为动画控制逻辑导入。风扇示例图形放在 fan-graphics，其他动画可自行给帧号绑定允许使用的图形。

XML 带有软件版本和对象依赖。导入后检查变量名、实例 DB、设备配置和编译消息。页面和代码仅用于教学仿真。
