#define xorAB 2 //아두이노 2번핀, 인터럽트로 이용가능
#define phaseA 3 //아두이노 3번핀, 인터럽트로 이용가능 
#define phaseB 4 //아두이노 4번핀 
#define rEnCLK 5 //아두이노 5번핀 Rotary Encode module KY-040 CLK
#define rEnDT 6 //아두이노 6번핀 Rotary Encode module KY-040 DT
#define _SW 7 //아두이노 7번핀
#define _LED 8 //아두이노 8번핀
#define motorRotDir 9 //아두이노 9번핀 모터의 방향 결정
#define motorSW 10 //아두이노 10번핀 모터 스위칭

//-------------------------<변수 선언>----------------------------
int xorAB_in; //phaseA과 phaseB의 xor값
int phaseA_in; //phaseA값
int phaseB_in; //phaseB값
int rEnCLK_in; //Rotary Encode module KY-040 CLK값
int rEnDT_in; //Rotary Encode module KY-040 DT값
int mEncoder = 0;//모터 엔코더 값
int prevMEncoder = 0;//이전 모터 엔코더 값
int _SW_in; //스위치 값
int _SW_flag; //Switch 가 눌렸는지 확인

volatile float mDeg = 0; //모터 회전각
char state; //Command 상태
bool isClockWise; //Motor 회전방향, 시계방향이면 true

unsigned long time_i; //처음 시간
unsigned long time_f = 0; //나중 시간  

//-------------------------<함수 선언>----------------------------

int CalEncodeDeg(int enVal, int prevEnVal);
int InputNumber(String strInput);

void ControlM(int pwmVal, int RotDir);
void MotorDeg(float deg);
void MotorDirection();
void ReadA();
void ReadEncoder();
void InputCommand();

//----------------------- <setup()함수>----------------------------
void setup() {
  pinMode(xorAB,INPUT); //xorAB INPUT 설정
  pinMode(phaseA,INPUT); //phaseA INPUT 설정  
  pinMode(phaseB,INPUT); //phaseB INPUT 설정
  pinMode(rEnCLK,INPUT); //rEnCLK INPUT 설정
  pinMode(rEnDT,INPUT); //rEnDT INPUT 설정
  pinMode(_SW,INPUT); //_SW INPUT 설정
  pinMode(_LED,OUTPUT); //_LED OUTPUT 설정
  pinMode(motorRotDir,OUTPUT); //motorRotDir OUTPUT 설정
  pinMode(motorSW,OUTPUT); //motorSW OUTPUT 설정
  
  Serial.begin(9600);
}

//----------------------- <loop()함수>----------------------------
void loop() {     
  InputCommand(); //InputCommand() 실행하고 루프
}

//----------------------------<InputCommand()함수>-------------------------------
void InputCommand()//Command를 입력받고 실행하는 함수
{     
  int enVal = 0; //Rotary Encode module KY-040의 출력 값
  int prevEnVal = 0; //Rotary Encode module KY-040의 이전 출력 값
  int enDeg = 0; //Rotary Encode module KY-040의 각도
  int prevEnDeg = 0; //Rotary Encode module KY-040의 이전 각도 
  
  String strInput; //커맨드 입력값
  
  state = '0'; //커맨드 상태 초기화
  Serial.println("Enter your command."); 
  detachInterrupt(digitalPinToInterrupt(xorAB)); //xorAB 변화 감지하는 인터럽트 비활성화  
  detachInterrupt(digitalPinToInterrupt(phaseA)); //phaseA 변화 감지하는 인터럽트 비활성화
  do
  {
    strInput = Serial.readString(); //커맨드 입력     
  }while(strInput[0] == 0);//어떤 것이라도 입력될 때까지 반복    
  switch(strInput[0]) //입력한 커맨드의 첫글자를 가져옴
  {
    
    //-------------------------<Command C 실행>----------------------------
    case 'C': //입력한 커맨드의 첫글자가 C일때
      if(strInput[1] == char(10))// C 한문자만 입력받았을 때 실행, 두번째 글자부터 공백인 경우   
      {         
        Serial.println("C 입력"); //"C 입력"라고 문자 출력          
        state = 'C'; //커맨드 상태 'C' 
        ControlM(255,LOW); //pwm Value 255 초기토크, 시계방향  
        delay(1500); //초기토크가 주어질 충분한 시간을 제공
        ControlM(200,LOW);//pwm Value 200 
        _SW_flag = 0; //Switch 가 눌렸는지 확인하는 변수 초기화   
        //xorAB변화 감지하여 ReadEncoder() 실행하는 인터럽트 활성화               
        attachInterrupt(digitalPinToInterrupt(xorAB),ReadEncoder,CHANGE);
        do 
        {                      
          delay(0);         
        }while(_SW_flag == 0);//Switch가 눌리기 전까지 회전       
      }
      //조건에 맞지 않는 코맨드가 입력됐을 경우
      else
      {
        Serial.println("잘못입력"); //"잘못입력"라고 문자 출력    
      }      
      break;
      
    //-------------------------<Command F 실행>----------------------------
    case 'F': //입력한 커맨드의 첫글자가 F일때   
      if(InputNumber(strInput)!= -1)//InputNumber()에서 커맨드 입력 두번째 문자부터 숫자 반환, 각도가 숫자인 경우   
      {        
        Serial.print("F"); //"F"라고 문자 출력 
        Serial.print(InputNumber(strInput)); //각도 출력 
        Serial.println(" 입력"); //" 입력"라고 문자 출력      
        MotorDeg(InputNumber(strInput)); //MotorDeg()에 각도 입력
      }

      //조건에 맞지 않는 코맨드가 입력됐을 경우
      else
      {
        Serial.println("잘못입력");     
      } 
      break;
      
    //-------------------------<Command R 실행>----------------------------
    case 'R': //입력한 커맨드의 첫글자가 R일때
      if(InputNumber(strInput)!= -1)////InputNumber()에서 커맨드 입력 두번째 문자부터 숫자 반환, 각도가 숫자인 경우   
      {   
        Serial.print("R"); //"R"라고 문자 출력 
        Serial.print(InputNumber(strInput)); //각도 출력 
        Serial.println(" 입력"); //" 입력"라고 문자 출력      
        MotorDeg(-InputNumber(strInput)); //MotorDeg()에 각도 입력, 양의 값의 입력각을 음수로 입력
      }

      //조건에 맞지 않는 코맨드가 입력됐을 경우
      else
      {
        Serial.println("잘못입력");     
      } 
      break;
      
    //-------------------------<Command S 실행>----------------------------
    case 'S': //입력한 커맨드의 첫글자가 S일때  
      if(strInput[1] == char(10))// S 한문자만 입력받았을 때 실행, 두번째 글자부터 공백인 경우     
      {
        Serial.println("S 입력"); //"S"라고 문자 출력  
        rEnCLK_in = digitalRead(rEnCLK); //rEnCLK의 값을 읽음
        rEnDT_in = digitalRead(rEnDT); //rEnDT의 값을 읽음
        enVal = rEnCLK_in * 10 + rEnDT_in; //Rotary Encode module KY-040의 출력값을 0,1,10,11의 값으로 표시, 초기값
        enDeg = 0;//Rotary Encode module KY-040의 각도 초기화
        
        do
        {          
          time_i = millis(); //이 코드가 실행된 현재 시간 측정

          //Rotary Encode module KY-040를 0.5초 동안 돌리지 않을 경우에 해당 각도만큼 회전
          if(time_i - time_f >= 500)//0.5초의 시간 계산
          {                       
            time_f = time_i; //나중 시간에 처음 시간값을 입력
            MotorDeg(enDeg); //enDeg의 각도로 MotorDeg()실행
            enDeg=0; //enDeg값 초기화
          }
          
          _SW_in = digitalRead(_SW); //_SW값을 읽음
          rEnCLK_in = digitalRead(rEnCLK); //rEnCLK값을 읽음
          rEnDT_in = digitalRead(rEnDT); //rEnDT값을 읽음                  
          prevEnVal = enVal; //이전 출력값에 현재 출력값을 입력         
          enVal = rEnCLK_in * 10 + rEnDT_in; //출력값을 0,1,10,11의 값으로 표시 
          enDeg += CalEncodeDeg(enVal, prevEnVal); //CalEncodeDeg()에서 측정된 각도값을 반환하여 enDeg에 반영

          
          //0.5초 이내에 조작하면 enDeg값을 바꿀수 있음//
          if(prevEnDeg != enDeg) //현재값과 이전 출력값이 다르면 실행, 엔코더 입력 중
          {
             //시간을 처음시간과 동일하게 하여 0.5초 계산을 다시하도록 설계 
             time_f = time_i;//나중 시간에 처음 시간값을 입력
             prevEnDeg = enDeg;//이전 출력값에 현재 출력값을 입력 
          }
                             
        }while(_SW_in == 1);//스위치 누르면 종료        
      }

      //조건에 맞지 않는 코맨드가 입력됐을 경우
      else
      {
        Serial.println("잘못입력");     
      }
      break;
      
    default: //나머지 조건에 맞지 않는 코맨드가 입력됐을 경우
      Serial.println("잘못입력"); 
  } 
}

//----------------------------<InputNumber()함수>-------------------------------
int InputNumber(String strInput)//입력받은 문자열 첫번째 문자를 제외하고 숫자로 출력
{
  int temp = 0;//출력하게 될 숫자를 저장
  
  for(int i = 1;i < strInput.length()-1;i++)
  {
    if(int(strInput[i]) < 48 || int(strInput[i]) > 57)//아스키코드 값이 숫자가 아닐때
    {
      temp = -1;//숫자가 아니라는 의미로 -1 값 이용
      break;//for문 break
    }
    //i번째 문자를 숫자로 바꿈 (아스키코드 48번부터 0, -48을 빼면 해당 숫자가 됨)
    //자리수에 맞춰 temp에 저장
    temp = temp * 10 + int(strInput[i])-48; //숫자가 아니라는 의미로 -1 값 이용
  }
  return temp;//temp값 반환
}

//----------------------------<CalEncodeDeg()함수>-------------------------------
int CalEncodeDeg(int enVal, int prevEnVal)//Rotary Encode module KY-040의 회전각 계산 함수
{  
  int ans = 0;//Rotary Encode module KY-040의 회전각 반환
  switch(enVal)//0->10->11->1->0으로 시계방향, 회전 각도 계산(시계방향 +)
  {
    case 0: //enVal값이 0일 때
      if(prevEnVal == 1) //이전값이 1인경우
        ans = 6; //+6도              
      else if(prevEnVal == 10) //이전값이 10인경우
        ans = -6; //-6도
      break;

    //나머지도 동일
    case 1:
      if(prevEnVal == 11)
        ans = 6;              
      else if(prevEnVal == 0)
        ans = -6;
      break;
      
    case 11:
      if(prevEnVal == 10)
        ans = 6;
      else if(prevEnVal == 1)
        ans = -6;
      break;
      
    case 10:
      if(prevEnVal == 0)
        ans = 6;
      else if(prevEnVal == 11)
        ans = -6;
      break;            
  }   
  return ans;//결과 반환
}

//------------------------------<ControlM()함수>---------------------------------
void ControlM(int pwmVal, int RotDir)//모터제어 함수, int pwmVal: PWM값 입력, int RotDir: HIGH 반시계방향, LOW 시계방향
{ 
  digitalWrite(motorRotDir, RotDir);//RotDir값에 따라 회전 방향을 결정해줌 
  analogWrite(motorSW, pwmVal);//PWM value으로 동작
  
  if(RotDir == HIGH)//반시계방향으로 돌때
  {
    isClockWise = false; //시계방향으로 도는지 확인하는 불형 변수 false
  }
  else  //시계방향으로 돌때
  {
    isClockWise = true; //시계방향으로 도는지 확인하는 불형 변수 true
  }
}

//------------------------------<MotorDeg()함수>---------------------------------
void MotorDeg(float deg)//모터의 각도를 맞추는 함수, int deg: 회전할 모터 각도 입력
{
  mDeg = 0; //모터의 회전각도 초기화
  state = 'M'; //커맨드 상태 'M' 
  float prevDeg = 0;//모터를 회전시켜 측정한 각도
  //xorAB변화 감지하여 ReadEncoder() 실행하는 인터럽트 활성화          
  attachInterrupt(digitalPinToInterrupt(xorAB),ReadEncoder,CHANGE);
  //phaseA변화 감지하여 ReadA() 실행하는 인터럽트 활성화                     
  attachInterrupt(digitalPinToInterrupt(phaseA),ReadA,CHANGE);
  
  do
  {       
    if(deg-mDeg > 0)//입력각도와 현재까지 회전한 모터의 각도 차이가 양수일때
    {      
      ControlM(250,LOW);//시계방향으로 회전
      delay(30);
      ControlM(0,LOW);//모터 회전을 끔
      delay(500);  
    }
    
    else if(deg-mDeg < 0)//입력각도와 현재까지 회전한 모터의 각도 차이가 음수일때
    {      
      ControlM(240,HIGH);//반시계방향으로 회전
      delay(30);
      ControlM(0,HIGH);//모터 회전을 끔
      delay(500);
    }
  //주어진 모터가 안정적으로 멈춰 있을 수 있는 각도 단위가 60도 이므로 그 안의 범위에서
  //do-while문 나올 수 있도록 설계
  }while(!(deg-mDeg <= 60 && deg-mDeg >= -60));
}

//----------------------------<ReadEncoder()함수>-------------------------------
void ReadEncoder()//인터럽트에서 실행될 함수
{    
  _SW_in = digitalRead(_SW); //바로 멈출 수 있도록 _SW값을 인터럽트에서 처리
  xorAB_in = digitalRead(xorAB); //xorAB값 읽음(모터 회전방향을 감지하기 위함)
  switch(state) //커맨드 상태
  {
    case 'C'://'C'인 경우 각속도 측정
      mDeg += 22.5;//인터럽트 활성화 될때마다 22.5도 추가
      time_i = millis(); //이 커맨드가 입력됐을 때의 시간값
      
      if(time_i - time_f >= 1000)//1초의 시간 계산이 넘으면
      {   
        //초당 회전각(degree/s) 출력        
        Serial.print(mDeg); //초당 회전각 이므로 1초동안 회전한 각도를 출력
        Serial.println(" degree/s"); 
        time_f = time_i; //나중 시간에 처음 시간값을 입력
        mDeg = 0; //모터 각도 값 초기화
      }
      
      if(_SW_in == 0)//_SW_in가 눌렸을 때
      {    
        detachInterrupt(digitalPinToInterrupt(xorAB)); //인터럽트 비활성화  
        ControlM(0,HIGH); //모터 회전을 끔   
        _SW_flag = 1; //Switch가 눌렸으면 1
      }
      break; 
      
    case 'M'://MotorDeg()함수 실행시
      MotorDirection(); //MotorDirection() 함수 실행
      break;    
      
    default: //그 외
      break;   
  }      
}

//-------------------------------<ReadA()함수>----------------------------------
void ReadA()//인터럽트에서 실행
{
  phaseA_in = digitalRead(phaseA);//phaseA값 읽음(모터 회전방향을 감지하기 위함)   
}

//-------------------------------<MotorDirection()함수>----------------------------------
void MotorDirection()//모터 회전방향과 각도에 관련된 함수
{
  mEncoder = 10*phaseA_in + xorAB_in;//출력값을 0,1,10,11의 값으로 표시    
  switch(mEncoder)//0->1->10->11->0 으로 반시계방향 
  {
    case 0:
      if(prevMEncoder == 1)
        {
        isClockWise = true;//시계방향으로 도는지 확인하는 불형 변수 true
        mDeg += 22.5; //22.5도 mDeg에 더함
        prevMEncoder = mEncoder;//이전값에 현재값을 대입
        }            
      else if(prevMEncoder == 11)        
        {
        isClockWise = false;//시계방향으로 도는지 확인하는 불형 변수 false
        mDeg -= 22.5; //22.5도 mDeg에 뺌 
        prevMEncoder = mEncoder;
        }
      break;

    //나머지 동일
    case 1:
      if(prevMEncoder == 10)
        {
        isClockWise = true;
        mDeg += 22.5;   
        prevMEncoder = mEncoder;
        }          
      else if(prevMEncoder == 0)
        {
        isClockWise = false;
        mDeg -= 22.5;   
        prevMEncoder = mEncoder;
        }
      break;
      
    case 11:
      if(prevMEncoder == 0)
        {
        isClockWise = true;
        mDeg += 22.5;   
        prevMEncoder = mEncoder;
        }
      else if(prevMEncoder == 10)
        {
        isClockWise = false;
        mDeg -= 22.5;   
        prevMEncoder = mEncoder;
        }
      break;
      
    case 10:
      if(prevMEncoder == 11)
      {
        isClockWise = true;
        mDeg += 22.5;   
        prevMEncoder = mEncoder;
      }
      else if(prevMEncoder == 1)
      {
        isClockWise = false;
        mDeg -= 22.5;   
        prevMEncoder = mEncoder;
      }
      break;            
  }
}
