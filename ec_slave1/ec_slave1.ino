#include "EasyCAT.h"
#include <SPI.h>
#include <Servo.h>

// EasyCATクラスのインスタンス
EasyCAT easyCAT;

unsigned long Millis = 0;
unsigned long PreviousMillis = 0;

void setup()
{
  Serial.begin(115200);
  Serial.print ("\nEasyCAT - Generic EtherCAT slave\n");
  delay(100);
  
  // EasyCATの初期化
  if (easyCAT.Init() == true)
  {
    Serial.println ("initialized");
  }
  else
  {
    // 初期化に失敗した場合
    Serial.println ("initialization failed");
    
    pinMode(13, OUTPUT);
    while(1)
    {
      digitalWrite (13, LOW);
      delay(500);
      digitalWrite (13, HIGH);
      delay(500);
    }
  }
  delay(100);
  PreviousMillis = millis();
}

void loop()
{
  // EasyCATのメインタスク
  easyCAT.MainTask();
  // ユーザーアプリケーション
  Application();
}

void Application ()
{
  // 10msecごとに実行
  Millis = millis();
  if (Millis - PreviousMillis >= 10)
  {
    PreviousMillis = Millis;
    
    // A0～3ポートのアナログ入力値を入力バッファの0～7バイトめに格納
    for(int i=0;i<4;i++){
        int vol = analogRead(0+i);
        byte vol_h = (byte)(vol >> 8);
        byte vol_l = (byte)(vol & 0xFF);
        easyCAT.BufferIn.Byte[i*2+0] = vol_h;
        easyCAT.BufferIn.Byte[i*2+1] = vol_l;
    }
  }
}
