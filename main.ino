#include <SPI.h>               //SPI协议库
#include <Adafruit_GFX.h>      //液晶屏图形库
#include <Adafruit_ST7735.h>   //st7735驱动库（SPI）
#include <Melopero_AMG8833.h>  //amg8833驱动库（I2C）
//按键引脚定义
#define SwitchOK 2
#define SwitchUP 3
#define SwitchDOWN 4
//液晶屏引脚定义
#define TFT_CS 15   //片选引脚
#define TFT_RST 17  //重置引脚
#define TFT_DC 16   //写数据/命令引脚
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
//如果不使用开发板默认的spi和sda引脚，可以使用下面的命令自定义引脚
//#define TFT_MOSI 13  // Data out
//#define TFT_SCLK 14  // Clock out
//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
Melopero_AMG8833 sensor;  //开启热成像传感器
#define MID (MAX_TEMP - MIN_TEMP)        //最大温度与最小温度的插值
#define GradationValue ((MID*10) / 120)  //1个温度值用1个颜色
#define MID_1_TEMP (((MID / 4)*3)+MIN_TEMP)
#define MID_2_TEMP (((MID / 4)*2)+MIN_TEMP)
#define MID_3_TEMP (((MID / 4)*1)+MIN_TEMP)

int x = 0, y = 0;  //x y 是AMG8833得到的温度像素坐标轴
int MAX_TEMP = 44, MIN_TEMP = 20;         //定义两个整形变量，分别对应设置传感器检测的最大温度和最小温度
int ResolutionMode = 14;//插值模式
int ClearCode = 1;//用于首次切换到主界面时刷新一次
int tempStep = 24;//温度步距
int Count = 0;//循环count次更新电池电压检测
int Temp0 = 0;//储存上次测量的中心温度值
int Temp1 = 0;//中心温度值
int Cx0 = 64;//储存上一次温度指针的位置
float InterpolationTemp[64][64] = { 0 };//储存插值后的温度

void setup(void) {
  tft.initR(INITR_BLACKTAB);
  tft.setSPISpeed(80000000);//设置SPI速度 速度过高可能会导致屏幕黑屏
  tft.setRotation(1);//设置屏幕方向
  tft.fillScreen(ST77XX_WHITE);
  Wire.begin();
  sensor.initI2C();
  int statusCode = sensor.resetFlagsAndSettings();
  statusCode = sensor.setFPSMode(FPS_MODE::FPS_10);
  pinMode(SwitchOK, INPUT_PULLUP);
  pinMode(SwitchUP, INPUT_PULLUP);
  pinMode(SwitchDOWN, INPUT_PULLUP);
  tft.setTextWrap(false);//设置文字环绕
}
//主页面
void loop(void) {
  int statusCode = sensor.updateThermistorTemperature();
  statusCode = sensor.updatePixelMatrix();
  if (ClearCode == 1) {
    tft.fillScreen(ST77XX_WHITE);
    ColorStripe();
    BatteryStripe();
    ClearCode = 0;
  }
  if (Count == 500) {
    BatteryStripe();
    Count = 0;
  }
  ColorCursor();
  switch (ResolutionMode) {
    case 14: Interpolation14(); break;
    case 28: Interpolation28(); break;
    case 63: Interpolation63(); break;
    case 8: Pixel8();break;
  }
  Count++;
}
//函数作用：一级菜单
void menu1(void) {
menu1:
  ClearCode = 1;
  tft.fillScreen(ST77XX_WHITE);
  tft.setCursor(16, 8);
  tft.setTextColor(ST77XX_BLACK);
  tft.setTextSize(1);
  tft.print("Back");
  tft.setCursor(16, 18);
  tft.print("Resolution");
  tft.setCursor(16, 28);
  tft.print("temp Config");
  tft.setCursor(16, 38);
  tft.print("Info");
  int CurrentSelection = 1;
  int CursorY = 10;
  tft.fillRect(10, CursorY, 3, 3, ST77XX_BLACK);


  while (1) {
    if ((digitalRead(SwitchDOWN) == LOW) && CurrentSelection < 4) {
      delay(300);  //消除抖动
      while (digitalRead(SwitchOK) == LOW)
        ;  //长按静止不动
      CurrentSelection++;
      CursorY = CursorY + 10;
      tft.fillRect(10, 10, 3, 50, ST77XX_WHITE);
      tft.fillRect(10, CursorY, 3, 3, ST77XX_BLACK);
    }
    if ((digitalRead(SwitchUP) == LOW) && CurrentSelection > 1) {
      delay(300);
      while (digitalRead(SwitchOK) == LOW)
        ;
      CurrentSelection--;
      CursorY = CursorY - 10;
      tft.fillRect(10, 10, 3, 50, ST77XX_WHITE);
      tft.fillRect(10, CursorY, 3, 3, ST77XX_BLACK);
    }
    if (digitalRead(SwitchOK) == LOW) {
      delay(100);
      while (digitalRead(SwitchOK) == LOW)
        ;
      switch (CurrentSelection) {
        case 1: tft.fillScreen(ST77XX_WHITE); return 0;  //返回上级菜单
        case 2:
          tft.fillScreen(ST77XX_WHITE);
          menu2_resolution();
          goto menu1;  //分辨率菜单
        case 3:
          tft.fillScreen(ST77XX_WHITE);
          menu2_TempSelect();
          goto menu1;  //温度选择菜单
        case 4:
          tft.fillScreen(ST77XX_WHITE);
          menu2_info();
          goto menu1;  //信息菜单
      }
    }
  }
}
//函数作用：二级菜单-分辨率设置
void menu2_resolution(void) {
  tft.fillScreen(ST77XX_WHITE);
  tft.setTextWrap(false);
  tft.setCursor(16, 8);
  tft.setTextColor(ST77XX_BLACK);
  tft.setTextSize(1);
  tft.print("8  x 8  | 10FPS");
  tft.setCursor(16, 18);
  tft.print("14 x 14 | 8 FPS ");
  tft.setCursor(16, 28);
  tft.print("28 x 28 | 4 FPS");
  tft.setCursor(16, 38);
  tft.print("63 x 63 | 1 FPS");
  int CurrentSelection = 1;
  int CursorY = 10;
  tft.fillRect(10, CursorY, 3, 3, ST77XX_BLACK);
  while (1) {
    if ((digitalRead(SwitchDOWN) == LOW) && CurrentSelection < 4) {
      delay(300);  //消除抖动
      while (digitalRead(SwitchOK) == LOW)
        ;  //长按静止不动
      CurrentSelection++;
      CursorY = CursorY + 10;
      tft.fillRect(10, 10, 3, 40, ST77XX_WHITE);
      tft.fillRect(10, CursorY, 3, 3, ST77XX_BLACK);
    }
    if ((digitalRead(SwitchUP) == LOW) && CurrentSelection > 1) {
      delay(300);
      while (digitalRead(SwitchOK) == LOW)
        ;
      CurrentSelection--;
      CursorY = CursorY - 10;
      tft.fillRect(10, 10, 3, 40, ST77XX_WHITE);
      tft.fillRect(10, CursorY, 3, 3, ST77XX_BLACK);
    }
    if (digitalRead(SwitchOK) == LOW) {
      delay(300);
      while (digitalRead(SwitchOK) == LOW)
        ;
      switch (CurrentSelection) {
        case 1: ResolutionMode = 8; return 0;
        case 2: ResolutionMode = 14; return 0;  //
        case 3: ResolutionMode = 28; return 0;  //
        case 4: ResolutionMode = 63; return 0;  //
      }
    }
  }
}
//函数作用：二级菜单-温度设置
void menu2_TempSelect(void) {
menu2_TempSelect:
  tft.fillScreen(ST77XX_WHITE);
  tft.setTextWrap(false);
  tft.setCursor(16, 8);
  tft.setTextColor(ST77XX_BLACK);
  tft.setTextSize(1);
  tft.print("Back");
  tft.setCursor(16, 18);
  tft.print("Max temp :");
  tft.setCursor(90, 18);
  tft.print(MAX_TEMP);
  tft.setCursor(16, 28);
  tft.print("Min temp :");
  tft.setCursor(90, 28);
  tft.print(MIN_TEMP);
  tft.setCursor(16, 38);
  tft.print("temp Step :");
  tft.setCursor(90, 38);
  tft.print(tempStep);
  int CurrentSelection = 1;
  int CursorY = 10;
  tft.fillRect(10, CursorY, 3, 3, ST77XX_BLACK);
  while (1) {
    if ((digitalRead(SwitchDOWN) == LOW) && CurrentSelection < 4) {
      delay(300);  //消除抖动
      while (digitalRead(SwitchOK) == LOW)
        ;  //长按静止不动
      CurrentSelection++;
      CursorY = CursorY + 10;
      tft.fillRect(10, 10, 3, 50, ST77XX_WHITE);
      tft.fillRect(10, CursorY, 3, 3, ST77XX_BLACK);
    }
    if ((digitalRead(SwitchUP) == LOW) && CurrentSelection > 1) {
      delay(300);
      while (digitalRead(SwitchOK) == LOW)
        ;
      CurrentSelection--;
      CursorY = CursorY - 10;
      tft.fillRect(10, 10, 3, 50, ST77XX_WHITE);
      tft.fillRect(10, CursorY, 3, 3, ST77XX_BLACK);
    }
    if (digitalRead(SwitchOK) == LOW) {
      delay(300);
      while (digitalRead(SwitchOK) == LOW)
        ;
      switch (CurrentSelection) {
        case 1: return 0;
        case 2: menu3_MaxTemp(); goto menu2_TempSelect;
        case 3: menu3_MinTemp(); goto menu2_TempSelect;
        case 4: menu3_tempStep(); goto menu2_TempSelect;
      }
    }
  }
}
//函数作用：三级菜单-最大温度设置
void menu3_MaxTemp(void) {
  tft.fillRect(89, 17, 14, 9, ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(90, 18);
  tft.print(MAX_TEMP);
  while (1) {
    if ((digitalRead(SwitchDOWN) == LOW)&&MIN_TEMP>0) {
      delay(300);  //消除抖动
      while (digitalRead(SwitchOK) == LOW)
        ;  //长按静止不动
      MAX_TEMP = MAX_TEMP - 1;
      MIN_TEMP = MAX_TEMP - tempStep;
      tft.fillRect(89, 17, 14, 9, ST77XX_BLACK);
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(90, 18);
      tft.print(MAX_TEMP);
    }
    if ((digitalRead(SwitchUP) == LOW)&&MAX_TEMP<80) {
      delay(300);
      while (digitalRead(SwitchOK) == LOW)
        ;
      MAX_TEMP = MAX_TEMP + 1;
      MIN_TEMP = MAX_TEMP - tempStep;
      tft.fillRect(89, 17, 14, 9, ST77XX_BLACK);
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(90, 18);
      tft.print(MAX_TEMP);
    }
    if (digitalRead(SwitchOK) == LOW) {
      delay(300);
      while (digitalRead(SwitchOK) == LOW)
        ;
      return 0;
    }
  }
}
//函数作用：三级菜单-最小温度设置
void menu3_MinTemp(void) {
  tft.fillRect(89, 27, 14, 9, ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(90, 28);
  tft.print(MIN_TEMP);
  while (1) {
    if ((digitalRead(SwitchDOWN) == LOW)&&MIN_TEMP>0) {
      delay(300);  //消除抖动
      while (digitalRead(SwitchOK) == LOW)
        ;  //长按静止不动
      MIN_TEMP = MIN_TEMP - 1;
      MAX_TEMP = MIN_TEMP + tempStep;
      tft.fillRect(89, 27, 14, 9, ST77XX_BLACK);
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(90, 28);
      tft.print(MIN_TEMP);
    }
    if ((digitalRead(SwitchUP) == LOW)&&MAX_TEMP<80) {
      delay(300);
      MIN_TEMP = MIN_TEMP + 1;
      MAX_TEMP = MIN_TEMP + tempStep;
      tft.fillRect(89, 27, 14, 9, ST77XX_BLACK);
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(90, 28);
      tft.print(MIN_TEMP);
    }
    if (digitalRead(SwitchOK) == LOW) {
      delay(300);
      while (digitalRead(SwitchOK) == LOW)
        ;
      
      return 0;
    }
  }
}
//函数作用：三级菜单-温度测量步距设置
void menu3_tempStep(void) {
  tft.fillRect(89, 37, 14, 9, ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(90, 38);
  tft.print(tempStep);
  while (1) {
    if ((digitalRead(SwitchDOWN) == LOW)) {
      delay(300);  //消除抖动
      while (digitalRead(SwitchOK) == LOW)
        ;  //长按静止不动
      if (tempStep == 12) {
        tft.setTextColor(ST77XX_BLACK);
        tft.setCursor(16, 130);
        tft.print("Already minimum!");
        delay(1000);
        tft.fillRect(16, 130, 110, 8, ST77XX_WHITE);
      } else {
        tempStep = tempStep - 12;
        MAX_TEMP = MIN_TEMP + tempStep;
        tft.fillRect(89, 37, 14, 9, ST77XX_BLACK);
        tft.setTextColor(ST77XX_WHITE);
        tft.setCursor(90, 38);
        tft.print(tempStep);
      }
    }
    if ((digitalRead(SwitchUP) == LOW)) {
      delay(300);
      if (tempStep == 72) {
        tft.setTextColor(ST77XX_BLACK);
        tft.setCursor(16, 130);
        tft.print("Already maximum!");
        delay(1000);
        tft.fillRect(16, 130, 110, 8, ST77XX_WHITE);
      } else {
        tempStep = tempStep + 12;
        MAX_TEMP = MIN_TEMP + tempStep;
        tft.fillRect(89, 37, 14, 9, ST77XX_BLACK);
        tft.setTextColor(ST77XX_WHITE);
        tft.setCursor(90, 38);
        tft.print(tempStep);
      }
    }
    if (digitalRead(SwitchOK) == LOW) {
      delay(300);
      while (digitalRead(SwitchOK) == LOW)
        ;
      if(MAX_TEMP>80){
        MAX_TEMP=80;
        MIN_TEMP=80-tempStep;
      }
      return 0;
    }
  }
}
//函数作用：二级菜单-信息
void menu2_info(void) {
  tft.fillScreen(ST77XX_WHITE);
  tft.drawLine(7, 26, 121, 26, 0xC618);
  tft.drawLine(7, 46, 121, 46, 0xC618);
  tft.drawLine(7, 66, 121, 66, 0xC618);
  tft.drawLine(7, 76, 121, 76, 0xC618);
  tft.setTextWrap(false);
  tft.setCursor(8, 8);
  tft.setTextColor(ST77XX_BLACK);
  tft.setTextSize(1);
  tft.print("Battery Voltage :");
  tft.setCursor(32, 18);
  tft.print("V");
  tft.setCursor(8, 28);
  tft.print("Firmware Version :");
  tft.setCursor(8, 38);
  tft.print("v0.3 [Beta]");
  tft.setCursor(8, 48);
  tft.print("Update date : ");
  tft.setCursor(8, 58);
  tft.print("2023-8-17");
  tft.setCursor(8, 68);
  tft.print("Made by Sab1e");
  tft.setCursor(8, 98);
  tft.print("Life's not out to get you");

  while (1) {
    delay(300);
    tft.fillRect(8, 18, 23, 7, ST77XX_WHITE);
    tft.setCursor(8, 18);
    tft.print(analogRead(A0) * (5.0 / 1024.0));
    if (digitalRead(SwitchOK) == LOW) {
      delay(100);
      while (digitalRead(SwitchOK) == LOW)
        ;
      return 0;
    }
  }
}
//函数作用：画彩色色条
void ColorStripe(void) {
  for (int sy = 10; sy < 118; sy++) {  //18*7=126
    tft.drawLine(130, 126-sy, 140, 126-sy, GET_RYGB_Color((MIN_TEMP + ((sy - 10) * (MAX_TEMP - MIN_TEMP) / 108))));
  }
  tft.setTextColor(ST77XX_BLACK);
  tft.setCursor(130, 119);
  tft.print(MIN_TEMP);
  tft.setCursor(130, 1);
  tft.print(MAX_TEMP);
}

//函数作用：屏幕右侧显示电池电量
void BatteryStripe(void) {

  int Color = 0x0000;
  float voltA[10] = { 0 };
  for (int i = 0; i < 10; i++) {
    voltA[i] = analogRead(A0) * (5.0 / 1024.0);
  }
  float volt = (voltA[0] + voltA[1] + voltA[2] + voltA[3] + voltA[4] + voltA[5] + voltA[6] + voltA[7] + voltA[8] + voltA[9]) / 10.0;
  if (volt > 3.75) {
    Color = ST77XX_GREEN;
  } else {
    Color = ST77XX_RED;
  }
  int dy = (int)((volt - 3.65) * 128 / 0.5);
  tft.fillRect(158, 0, 2, 128-dy, 0x0000);
  tft.fillRect(158, 128-dy, 2, dy, Color);
}

//函数作用：显示温度指针
void ColorCursor(){
  int Cx;
  tft.drawPixel(141,Cx0,ST77XX_WHITE);

  tft.drawPixel(142,Cx0,ST77XX_WHITE);
  tft.drawPixel(142,Cx0+1,ST77XX_WHITE);
  tft.drawPixel(142,Cx0-1,ST77XX_WHITE);

  tft.drawPixel(143,Cx0,ST77XX_WHITE);
  tft.drawPixel(143,Cx0-1,ST77XX_WHITE);
  tft.drawPixel(143,Cx0+1,ST77XX_WHITE);
  tft.drawPixel(143,Cx0+2,ST77XX_WHITE);
  tft.drawPixel(143,Cx0-2,ST77XX_WHITE);

  tft.setCursor(145,Cx0-3);
  tft.setTextColor(ST77XX_WHITE);
  tft.print(Temp0);
  
  Cx = ((int)sensor.pixelMatrix[4][4]-MIN_TEMP)*108/(MAX_TEMP-MIN_TEMP);
  Cx = 112 - Cx;
  if(Cx<9)Cx=9;
    else if(Cx>116)Cx=116;
  Cx0 = Cx;
  tft.drawPixel(141,Cx,ST77XX_BLACK);

  tft.drawPixel(142,Cx,ST77XX_BLACK);
  tft.drawPixel(142,Cx+1,ST77XX_BLACK);
  tft.drawPixel(142,Cx-1,ST77XX_BLACK);

  tft.drawPixel(143,Cx,ST77XX_BLACK);
  tft.drawPixel(143,Cx+1,ST77XX_BLACK);
  tft.drawPixel(143,Cx-1,ST77XX_BLACK);
  tft.drawPixel(143,Cx+2,ST77XX_BLACK);
  tft.drawPixel(143,Cx-2,ST77XX_BLACK);

  
  tft.setCursor(145,Cx-3);
  tft.setTextColor(ST77XX_BLACK);
  Temp1 = sensor.pixelMatrix[4][4];
  tft.print(Temp1);
  Temp0 = Temp1;
}
//函数作用：在图像中心显示十字指针
void Cursor(void){
  tft.fillRect(64,61,2,8,ST77XX_BLACK);
  tft.fillRect(61,64,8,2,ST77XX_BLACK);
}
//函数作用：在图像右下角显示当前分辨率
void ShowResolution(void){
  tft.setTextColor(ST77XX_BLACK);
  tft.setCursor(109, 118);
  tft.print(ResolutionMode);
  tft.print("#");
}
//函数作用：原始图像 分辨率：8x8
void Pixel8(void){
  for(int row = 0;row<8;row++){
    for(int col = 0;col<8;col++){
      tft.fillRect((row * 16),112 -(col * 16), 16, 16, GET_RYGB_Color(sensor.pixelMatrix[row][col]));
      if((col>3&&col<5)&&(row>3&&row<5)){Cursor();}
      if(col>6&&row>6){ShowResolution();}
      if (digitalRead(SwitchOK) == LOW) {
        delay(300);
        menu1();
        return 0;
      }
    }
  }
}
//函数作用：双线性插值 分辨率：63x63
void Interpolation63(void) {
  int X = -1;
  float ku = 0, kv = 0;
  for (int row = 0; row < 63; row++) {
    if ((row == 0)||(row > 2 && row % 9 == 1)) { X = X + 1; }
    if (row % 9 == 0) { ku = 0; }
    else if (row % 9 == 1) { ku = 0.1111; }
    else if (row % 9 == 2) { ku = 0.2222; }
    else if (row % 9 == 3) { ku = 0.3333; }
    else if (row % 9 == 4) { ku = 0.4444; }
    else if (row % 9 == 5) { ku = 0.5555; }
    else if (row % 9 == 6) { ku = 0.6666; }
    else if (row % 9 == 7) { ku = 0.7777; }
    else if (row % 9 == 8) { ku = 0.8888; }
    if (row == 9 || row == 18 || row == 27 || row == 36 || row == 45 || row == 54 || row == 63) { ku = 1; }
    int Y = -1;
    for (int col = 0; col < 63; col++) {
      if ((col == 0)||(col > 2&&col % 9 == 1)) { Y = Y + 1; }
      if (col % 9 == 0) { kv = 0; }
      else if (col % 9 == 1) { kv = 0.1111; }
      else if (col % 9 == 2) { kv = 0.2222; }
      else if (col % 9 == 3) { kv = 0.3333; }
      else if (col % 9 == 4) { kv = 0.4444; }
      else if (col % 9 == 5) { kv = 0.5555; }
      else if (col % 9 == 6) { kv = 0.6666; }
      else if (col % 9 == 7) { kv = 0.7777; }
      else if (col % 9 == 8) { kv = 0.8888; }
      if (col == 9 || col == 18 || col == 27 || col == 36 || col == 45 || col == 54 || col == 63) { kv = 1; }
      InterpolationTemp[row][col] = (1 - ku) * (1 - kv) * sensor.pixelMatrix[X][Y] + (1 - ku) * kv * sensor.pixelMatrix[X][Y + 1] + ku * (1 - kv) * sensor.pixelMatrix[X + 1][Y] + ku * kv * sensor.pixelMatrix[X + 1][Y + 1];
      tft.fillRect((row * 2) + 1,124-(col * 2) + 1, 2, 2, GET_RYGB_Color(InterpolationTemp[row][col]));
      if((col>31&&col<34)&&(row>31&&row<34)){Cursor();}
      if(col>58&&row>58){ShowResolution();}
      if (digitalRead(SwitchOK) == LOW) {
        delay(300);
        menu1();
        return 0;
      }
    }
  }
}
//函数作用：双线性插值 分辨率：28x28
void Interpolation28(void) {
  int X = -1;
  float ku = 0, kv = 0;
  for (int row = 0; row < 28; row++) {
    if ((row == 0)||(row > 2 && row % 4 == 1)) { X = X + 1; }
    if (row % 4 == 0) { ku = 0; }
    else if (row % 4 == 1) { ku = 0.25; }
    else if (row % 4 == 2) { ku = 0.5; }
    else if (row % 4 == 3) { ku = 0.75; }
    if (row == 4 || row == 8 || row == 12 || row == 16 || row == 20 || row == 24 || row == 28) { ku = 1; }
    int Y = -1;
    for (int col = 0; col < 28; col++) {
      if ((col == 0)||(col > 2&&col % 4 == 1)) { Y = Y + 1; }
      if (col % 4 == 0) { kv = 0; }
      else if (col % 4 == 1) { kv = 0.25; }
      else if (col % 4 == 2) { kv = 0.5; }
      else if (col % 4 == 3) { kv = 0.75; }
      if (col == 4 || col == 8 || col == 12 || col == 16 || col == 20 || col == 24 || col == 28) { kv = 1; }
      InterpolationTemp[row][col] = (1 - ku) * (1 - kv) * sensor.pixelMatrix[X][Y] + (1 - ku) * kv * sensor.pixelMatrix[X][Y + 1] + ku * (1 - kv) * sensor.pixelMatrix[X + 1][Y] + ku * kv * sensor.pixelMatrix[X + 1][Y + 1];
      tft.fillRect( (row * 4) + 8,108-(col * 4) + 8, 4, 4, GET_RYGB_Color(InterpolationTemp[row][col]));
      if((col>13&&col<15)&&(row>13&&row<15)){Cursor();}
      if(col>26&&row>26){ShowResolution();}
      if (digitalRead(SwitchOK) == LOW) {
        delay(300);
        menu1();
        return 0;
      }
    }
  }
}
//函数作用：双线性插值 分辨率：14x14
void Interpolation14(void) {  
  int X = -1;
  float ku = 0, kv = 0;
  for (int row = 0; row < 14; row++) {
    if ((row == 0)||(row > 2 && row % 2 == 1)) { X = X + 1; }
    if (row % 2 == 0) { ku = 0; }
    else if (row % 2 == 1) { ku = 0.5; }
    if (row == 2 || row == 4 || row == 6 || row == 8 || row == 10 || row == 12 || row == 14) { ku = 1; }
    int Y = -1;
    for (int col = 0; col < 14; col++) {
      if ((col == 0)||(col > 2&&col % 2 == 1)) { Y = Y + 1; }
      if (col % 2 == 0) { kv = 0; }
      else if (col % 2 == 1) { kv = 0.5; }
      if (col == 2 || col == 4 || col == 6 || col == 8 || col == 10 || col == 12 || col == 14) { kv = 1; }
      InterpolationTemp[row][col] = (1 - ku) * (1 - kv) * sensor.pixelMatrix[X][Y] + (1 - ku) * kv * sensor.pixelMatrix[X][Y + 1] + ku * (1 - kv) * sensor.pixelMatrix[X + 1][Y] + ku * kv * sensor.pixelMatrix[X + 1][Y + 1];
      tft.fillRect((row * 9) + 1,117-(col * 9) + 1,  9, 9, GET_RYGB_Color(InterpolationTemp[row][col]));
      if((col>4&&col<9)&&(row>4&&row<9)){Cursor();}
      if(col>12&&row>12){ShowResolution();}
      if (digitalRead(SwitchOK) == LOW) {
        delay(100);
        while (digitalRead(SwitchOK) == LOW)
          delay(300);
        menu1();
        return 0;
      }
    }
  }
}

//函数作用：输入温度数值，返回温度对应的颜色
int GET_RYGB_Color(int Temp) {
  int color = 0x0000;
  if (Temp >= MID_1_TEMP && Temp < MAX_TEMP) {  //红色->黄色
    color = 0xF800;
    Temp = (MAX_TEMP - Temp) * 10;
    Temp = Temp / GradationValue * 64;
    color = color | Temp;
    return color;
  } else if (Temp >= MID_2_TEMP && Temp < MID_1_TEMP) {  //黄色->绿色
    color = 0xFFE0;
    Temp = (MID_1_TEMP - Temp) * 10;
    Temp = Temp / GradationValue * 2048;
    color = color ^ Temp;
    return color;
  } else if (Temp >= MID_3_TEMP && Temp < MID_2_TEMP) {  //绿色->青色
    color = 0x07E0;
    Temp = (MID_2_TEMP - Temp) * 10;
    Temp = Temp / GradationValue * 1;
    color = color | Temp;
    return color;
  } else if (Temp >= MIN_TEMP && Temp < MID_3_TEMP) {    //青色->蓝色
    color = 0x07FF;
    Temp = (MID_3_TEMP - Temp) * 10;
    Temp = Temp / GradationValue * 32;
    color = color ^ Temp;
    return color;
  }
  return 0x0000;
}
