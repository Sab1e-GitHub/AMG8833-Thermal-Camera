# AMG8833-Thermal-Camera

## 此项目已经废弃，请查看新项目：[Kalama](https://github.com/Sab1e-GitHub/Kalama)

#ArduinoNano + AMG8833 +ST7735 热成像仪

本项目使用Arduino IDE开发 

屏幕采用1.8'TFT屏幕（主控ST7735） 

热成像传感器为AMG8833

展示：
![IMG_20230820_193253](https://github.com/Sab1e-GitHub/AMG8833-Thermal-Camera/assets/72060564/42d747c2-b0ba-45e5-8618-cf88834570f0)



======================================================================

本人第一次开发项目，写的不好请见谅。

如果发现什么问题可以写在issue里，我看见的话会回复的。

======================================================================

本项目附带简单的GUI可以实现以下功能：

  1.调整插值分辨率 （分辨率有 8*8 14*14 28*28 64*64）

  2.调整检测最大温度（最大80摄氏度）、最小温度（最小0摄氏度）、温度检测步距（72~12）


======================================================================

引脚定义：
  |Nano|~~~|
  |---|---|
  |15 | ST7735-CS|
  |16 | ST7735-DC|
  |17 | ST7735-RST|
  |MOSI  | ST7735-SDA|
  |SCK   | ST7735-SCL|
  |D2 | SwitchOK|
  |D3 | SwitchUP|
  |D4 | SwitchDOWN|
  |A5 | AMG8833-SCL|
  |A4 | AMG8833-SDA|

======================================================================

感谢以下开发者提供的驱动：

[Melopero]的 AMG8833驱动

[Adafruit]的 ST7735显示驱动
