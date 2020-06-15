#include <Servo.h>

//轉速表指針，馬達訊號接角
const int RCServoPin = D0;
Servo servo;

//實來實作超轉燈 D0 沒有 PWM ，所以用 D1 來作呼吸燈
//詳見 : http://web.htjh.tp.edu.tw/B4/106iot/NodeMCU%E4%BD%BF%E7%94%A8%E4%BB%8B%E7%B4%B9.pdf
const int overRPMLedPin = D1;

//用來偵測 D3 腳抓引擎轉速的部分
const int firePin = D3;
const int fireInt = 0;
const unsigned int pulsesPR = 1;
unsigned long lastPT = 0;
unsigned long rpm = 0;

unsigned long st = 0;
int isTested = 0; // 開機的測試模式，跑過一次後，數值變成 1 ，下次再 loop 就不會跑了
unsigned int lastAngle = 0; //紀錄最後 servo 的角度
unsigned long Gnow = 0;
//取得的rpm不停的寫入 clean_rpms，0~9 陣列，排序後，取中間，減少判差
//可以用來作最後10次rpm變化量，作延遲觸發事件
int clean_rpms[] = { 0,0,0,0,0,0,0,0,0,0 };
int rpm_step = 0; //上面陣列儲存的 index，循序寫入，最後 % 陣列長度後回到 index 0
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);  

  //轉表指針伺服馬達
  servo.attach(RCServoPin);    

  //超轉燈
  pinMode(overRPMLedPin, OUTPUT);
  digitalWrite(overRPMLedPin,LOW); //預設不亮

  //轉速相關
  pinMode(firePin, INPUT);
  attachInterrupt(fireInt, fireIsr, RISING);   
}
void fireIsr()
{
  //取得轉速訊號的方法，這個寫法才能正常拿到 12000rpm 的以上的值
  unsigned long now = micros();
  unsigned long duringInt = now - lastPT;
  if (duringInt > 1000) //5000
  {
    //Serial.print("duringInt:");
    //Serial.println(duringInt);
    rpm = 60000000UL  / (duringInt * pulsesPR); //轉速訊號會在這取得!!!
    rpm -= rpm % 10;
    lastPT = now;
    clean_rpms[rpm_step]=rpm;    
    rpm_step++;
    rpm_step%=10;
  }  
}
int rpmToServoAngle()
{
  //將 rpm 轉成 servo 要用的角度
  // 14000/180 = 77.778
  int angle = 0;
  // 如果在 2000 rpm 以內
  // 角度要另外計算
  int r = 0;
  if( rpm >= 1500 && rpm < 2000)
  {
    // 2000 ~ 14000 -> 165~0度
    // 1500 ~  2000 -> 171~165度
    // 0 ~  1000 -> 180~177度
    //∵ FZR 的 2000rpm 以內非線性距離，1000rpm 大概只佔 1/3
    //∴ 1000rpm 約等於 177度，1500rpm 約等於 171度
    // (14000-1500)/ x = 171 , x = 73.099
    // (14000-1000)/ x = 177 , x = 73.45
    r = 73.099;
  }
  else if( rpm >=1000 && rpm < 1500)
  {
    r = 73.45;
  }
  else if( rpm < 1000)
  {
    // (14000-500)/ x = 178 , x = 75.842
    r = 75.842;
  }
  else
  {
    //2000 以上
    r = 77.778;
  }
  angle = (int)(14000-rpm) / r;
  return angle; 
}
void gotoS(int angle)
{
  //lastAngle : servo 最後的角度
  //當 servo 要轉動，要從最後 servo 的位置，慢慢走到新的角度，才不會造成 servo 跳動、損壞
  //如此旋轉，移動，看起來才會 smooth 順暢
  //delay 的時間不可低於 6ms，以免 servo 燒掉
  if(lastAngle>=angle)
  {
    for(int i= lastAngle;i>=angle;i--)
    {
      servo.write(i);           
      delay(10);
    }
  }
  else
  {
    for(int i= lastAngle;i<angle;i+=1)
    {
      servo.write(i);           
      delay(10);
    }
  }
  lastAngle = angle;
}
void loop() {
  //noInterrupts();    
  //interrupts();
  
  if(isTested==0)
  {
    for(int run_times = 0; run_times<1;run_times++) // 玩二次
    {
      for(int angle = 180-1; angle >= 0; angle--)
      {
        servo.write(angle);
        delay(10);
      }      
      for(int angle = 0; angle <=180; angle++)
      {
        servo.write(angle);
        delay(10);
      }
    }
    isTested=1;
    //測完了，歸零
    servo.write(270);
    lastAngle = 270;
    delay(300);
  }
  Gnow = millis() - st;
  if( Gnow >= 1000)
  {
    //利用 arduino 執行時間，微調，避免馬達過度通電損壞  
    int angle = rpmToServoAngle(); //取得新角度
    gotoS(angle); //馬達走到新角度    
    Serial.print(" , Angle: ");
    Serial.print(angle);
    Serial.print(" , Rpm: ");   
    Serial.print(rpm);
    Serial.println();
    st = Gnow;   
  }

  //當轉速訊號消失一段時間後，轉速表也要歸零
  if (micros()-lastPT > 300000) 
  { 
    //Serial.println("超過N秒沒變動,rpm 刷 0");
    rpm = 0;
    for(int i=0;i<10;i++)
    {
      clean_rpms[i] = 0; //陣列也要清 0 
    }
  }  
  //超轉燈，呼吸燈
  if( rpm >= 8000 && rpm < 12000)
  {
    // 詳見 https://www.youtube.com/watch?v=27GkMk8ct0s
    // 0~255，直接就從 半亮至全亮 128~255
    // 12000:255, 8000:128
    //digitalWrite(overRPMLedPin,HIGH);    
    int rpm_val = map(rpm, 8000, 12000, 128, 255);
    analogWrite (overRPMLedPin, rpm_val);
  }
  else if( rpm >= 12000) //超過12000轉就整顆全亮
  {
    analogWrite (overRPMLedPin,255);
  }
  else //低於 8000 以下，暗
  {
    analogWrite (overRPMLedPin,0);
  }
}
