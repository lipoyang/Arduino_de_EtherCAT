#include "EasyCAT.h"
#include <SPI.h>
#include <Servo.h>

// EasyCATクラスのインスタンス
EasyCAT easyCAT;

unsigned long Millis = 0;
unsigned long PreviousMillis = 0;

// 実験用のサーボ
Servo servo[4];

void setup()
{
  // 実験用サーボはポート3,5,6,9に接続
  servo[0].attach(3);
  servo[0].write(90);
  servo[1].attach(5);
  servo[1].write(90);
  servo[2].attach(6);
  servo[2].write(90);
  servo[3].attach(9);
  servo[3].write(90);
  
  Serial.begin(115200);
  Serial.print ("\nEasyCAT - Generic EtherCAT slave\n");

  // EasyCATの初期化
  if (easyCAT.Init() == true)
  {
    Serial.print ("initialized");
  }
  else
  {
    // 初期化に失敗した場合
    Serial.print ("initialization failed");
    
    pinMode(13, OUTPUT);
    while(1)
    {
      digitalWrite (13, LOW);
      delay(500);
      digitalWrite (13, HIGH);
      delay(500);
    }
  }
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
    
    // 出力バッファの0～3バイトめの値をサーボに出力
    for(int i=0;i<4;i++){
      int deg = easyCAT.BufferOut.Byte[i];
      //servo[i].write(deg);
      Serial.print(deg);
    }
    Serial.print("\n");

    int vol = analogRead(0);
    byte vol_h = (byte)(vol >> 8);
    byte vol_l = (byte)(vol & 0xFF);
    easyCAT.BufferIn.Byte[0] = vol_h;
    easyCAT.BufferIn.Byte[1] = vol_l;
  }
}
