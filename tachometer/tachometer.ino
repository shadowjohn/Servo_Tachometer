#include <Servo.h>
Servo servo;

//用來偵測 D3 腳抓引擎轉速的部分
const int firePin = D3;
const int fireInt = 0;
const unsigned int pulsesPR = 1;
unsigned long lastPT = 0;
unsigned long rpm = 0;

unsigned long st = 0;
int isTested = 0;
unsigned int lastAngle = 0;
unsigned long Gnow = 0;
//取得的rpm不停的寫入 clean_rpms，0~9 陣列，排序後，取中間，減少判差
int clean_rpms[] = { 0,0,0,0,0,0,0,0,0,0 };
int rpm_step = 0;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  //pinMode(D0, OUTPUT);
  servo.attach(D0);    
  pinMode(firePin, INPUT);
  attachInterrupt(fireInt, fireIsr, RISING); 
}
void fireIsr()
{
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
  // 14000/180 = 77.778
  // 14000/270 = 51.851852
  // 14000/(270-100) = 82.353
  int angle = (int)(14000-rpm) / 77.778;
  return angle; 
}
void gotoS(int angle)
{
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
    //Serial.print("Gnow: ");
    //Serial.print(Gnow);
    
    int angle = rpmToServoAngle();
    gotoS(angle);    
    Serial.print(" , Angle: ");
    Serial.print(angle);
    Serial.print(" , Rpm: ");   
    Serial.print(rpm);
    Serial.println();
    st = Gnow;   
  }
  if (micros()-lastPT > 300000) 
  { 
    //Serial.println("超過N秒沒變動,rpm 刷 0");
    rpm=0;
    for(int i=0;i<10;i++)
    {
      clean_rpms[i]=0;   
    }
  }  
  /*Serial.print("RPM:");
  Serial.println(rpm);
  Gnow = millis() - st;
  if( Gnow >= 10000)
  {
    Serial.print("Gnow: ");
    Serial.print(Gnow);
    st = Gnow;   
    int angle = rpmToServoAngle();
    servo.write(angle);           
    Serial.print(" , Angle: ");
    Serial.print(angle);
    Serial.print(" , ");
    delay(10);
  }
  */ 
}
