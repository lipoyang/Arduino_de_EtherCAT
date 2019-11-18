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
  // サーボはポート3,4,5,6に接続 (肩、肘、手首、指)
  const int SERVO_PORT[4] = {3,4,5,6};
  const int I_DEG[4] = { 90,  55,  90, 100}; // 初期角度 {中, 下, 中, 閉}
  for(int i=0;i<4;i++){
    int pos = I_DEG[i];
    servo[i].attach(SERVO_PORT[i]);
    servo[i].write(pos);
  }
  
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
    
    // 有効な受信データがあるか？
    int enable = easyCAT.BufferOut.Byte[4];
    if(enable == 0xA5){
      // 出力バッファの0～3バイトめの値をサーボに出力
      for(int i=0;i<4;i++){
        int deg = easyCAT.BufferOut.Byte[i];
        servo[i].write(deg);
        Serial.print(deg);
      }
      Serial.print("\n");
    }
  }
}
