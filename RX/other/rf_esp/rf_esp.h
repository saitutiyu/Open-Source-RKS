
//D0 = GPIO16; 
//D1 = GPIO5; 
//D2 = GPIO4;	LED on esp8266
//D3 = GPIO0;can not download when connected to low
//D4 = GPIO2;	
//D5 = GPIO14;  
//D6 = GPIO12;
//D7 = GPIO13;
//D8 = GPIO15;  can not start when high input
//D9 = GPIO3; UART RX
//D10 = GPIO1; UART TX
//LED_BUILTIN = GPIO16 (auxiliary constant for the board LED, not a board pin);



#include<ESP8266WiFi.h>
#include<WiFiUdp.h>
#include<ESP8266mDNS.h>
#include<ArduinoOTA.h>
#include<ESP8266WiFiMulti.h>
#include<time.h>
#define timezone 8

#define BUZZ				D7


const char* ssid = "frye";  //Wifi����
const char* password = "52150337";  //Wifi����
WiFiUDP m_WiFiUDP;


char *time_str;   
char H1,H2,M1,M2,S1,S2;



#include "RCSwitch.h"
RCSwitch RfSwitch315 = RCSwitch();
RCSwitch RfSwitch433 = RCSwitch();


#include "Z:\bt\web\datastruct.h"
tRfData RfData;
unsigned char RoomIndex = 255;
unsigned long TenthSecondsSinceStart = 0;

void TenthSecondsSinceStartTask();
void OnTenthSecond();
void OnSecond();
void MyPrintf(const char *fmt, ...);

#define RF_COMMAND_BASE 0xB37C64
#define RF_COMMAND_SEP 200 //ms

#define RF_WINDOW_COMMAND_LEN 5
#define RF_WINDOW_COMMAND_COUNTER 2
unsigned char RfWindow[RF_WINDOW_COMMAND_COUNTER*3][RF_WINDOW_COMMAND_LEN] = 
{
{0xF8, 0x41, 0xC1, 0x91, 0x11}
,{0xF8, 0x41, 0xC1, 0x91, 0x33}
,{0xF8, 0x41, 0xC1, 0x91, 0x55}

,{0x00, 0x00, 0x00, 0x00, 0x00}
,{0x00, 0x00, 0x00, 0x00, 0x00}
,{0x00, 0x00, 0x00, 0x00, 0x00}

};


void setup() 
{       


	RfData.DataType = 10;

	RfSwitch315.enableTransmit(D5);
	RfSwitch315.setRepeatTransmit(10);

	RfSwitch433.enableTransmit(D6);
	RfSwitch433.setRepeatTransmit(20);
	RfSwitch433.setProtocol(2);

	pinMode(BUZZ, OUTPUT);


	delay(50);                      
	Serial.begin(115200);


	WiFi.disconnect();
	WiFi.mode(WIFI_STA);//����ģʽΪSTA
	Serial.print("Is connection routing, please wait");  
	WiFi.begin(ssid, password); //Wifi���뵽����
	Serial.println("\nConnecting to WiFi");
	//���Wifi״̬����WL_CONNECTED�����ʾ����ʧ��
	while (WiFi.status() != WL_CONNECTED) {  
		Serial.print("."); 
		delay(1000);    //��ʱ�ȴ���������
	}



	//����ʱ���ʽ�Լ�ʱ�����������ַ
	configTime(timezone * 3600, 0, "pool.ntp.org", "time.nist.gov");
	Serial.println("\nWaiting for time");
	while (!time(nullptr)) {
		Serial.print(".");
		delay(1000);    
	}
	Serial.println("");




	byte mac[6];
	WiFi.softAPmacAddress(mac);
	MyPrintf("macAddress 0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X\r\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	//for (byte i=0;i<6;i++)
	//{
	//	RoomData.Mac[i] = mac[i];
	//}

	//for (unsigned char i = 0;i<ROOM_NUMBER;i++)
	//{
	//	if (memcmp(&RoomData.Mac[0],&RoomMacAddress[i][0],sizeof(unsigned long)*6) == 0)
	//	{
	//		MyPrintf("room ID=%d \r\n",i);
	//		RoomIndex = i;
	//		break;
	//	}
	//}


	m_WiFiUDP.begin(5050); 

	ArduinoOTA.onStart([]() {
		String type;
		if (ArduinoOTA.getCommand() == U_FLASH) {
			type = "sketch";
		} else { // U_SPIFFS
			type = "filesystem";
		}

		// NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
		Serial.println("Start updating " + type);
	});
	ArduinoOTA.onEnd([]() {
		Serial.println("\nEnd");
	});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
	});
	ArduinoOTA.onError([](ota_error_t error) {
		Serial.printf("Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) {
			Serial.println("Auth Failed");
		} else if (error == OTA_BEGIN_ERROR) {
			Serial.println("Begin Failed");
		} else if (error == OTA_CONNECT_ERROR) {
			Serial.println("Connect Failed");
		} else if (error == OTA_RECEIVE_ERROR) {
			Serial.println("Receive Failed");
		} else if (error == OTA_END_ERROR) {
			Serial.println("End Failed");
		}
	});
	ArduinoOTA.begin();
	Serial.println("Ready");
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());


}

void loop() 
{
	ArduinoOTA.handle();

	TenthSecondsSinceStartTask();


	m_WiFiUDP.parsePacket(); 
	unsigned int UdpAvailable = m_WiFiUDP.available();
	if (UdpAvailable == sizeof(tRfCommand))
	{
		//MyPrintf(" m_WiFiUDP.available() = %d\r\n",UdpAvailable);
		tRfCommand tempRfCommand;
		m_WiFiUDP.read((char *)&tempRfCommand,sizeof(tRfCommand));
		if (tempRfCommand.Triger == true)
		{
			unsigned long RfCommand24Bit;
			unsigned char * pRfCommand24Bit = (unsigned char *)&RfCommand24Bit;
			MyPrintf("get RfCommand trig.OpCode = %d \r\n",tempRfCommand.RfOpCode);
			switch(tempRfCommand.RfOpCode)
			{
			case RF_OP_LIGHT_ALL_ON:
				for(int i = 12; i >= 0; i--)
				{
					if ((i !=4 )&&(i<11))
					{
						digitalWrite(BUZZ, HIGH);
						delay(RF_COMMAND_SEP);
						digitalWrite(BUZZ, LOW);
						RfCommand24Bit = RF_COMMAND_BASE+i*3;
						RfSwitch315.send(pRfCommand24Bit,24);
						yield();
					}
				}
				break;
			case RF_OP_LIGHT_ALL_OFF:
				for(int i = 12; i >= 0; i--)
				{
					digitalWrite(BUZZ, HIGH);
					delay(RF_COMMAND_SEP);
					digitalWrite(BUZZ, LOW);
					RfCommand24Bit = RF_COMMAND_BASE+i*3+1;
					RfSwitch315.send(pRfCommand24Bit,24);
					yield();
				}
				break;

			case RF_OP_ON:
				digitalWrite(BUZZ, HIGH);
				if (tempRfCommand.RfIndex< 13)
				{
					RfCommand24Bit = RF_COMMAND_BASE+tempRfCommand.RfIndex*3;
					RfSwitch315.send(pRfCommand24Bit,24);	
				}
				else if (tempRfCommand.RfIndex >= 13)
				{
					RfSwitch433.send_duya(RfWindow[(tempRfCommand.RfIndex-13)*3],39);
				}
				digitalWrite(BUZZ, LOW);


				break;
			case RF_OP_OFF:
				digitalWrite(BUZZ, HIGH);
				if (tempRfCommand.RfIndex< 13)
				{
					RfCommand24Bit = RF_COMMAND_BASE+tempRfCommand.RfIndex*3+1;
					RfSwitch315.send(pRfCommand24Bit,24);
				}
				else if (tempRfCommand.RfIndex >= 13)
				{
					RfSwitch433.send_duya(RfWindow[(tempRfCommand.RfIndex-13)*3+1],39);
				}
				digitalWrite(BUZZ, LOW);


				break;
			case RF_OP_ON_OFF:

				if (tempRfCommand.RfIndex< 13)
				{
					//digitalWrite(BUZZ, HIGH);
					RfCommand24Bit = RF_COMMAND_BASE+tempRfCommand.RfIndex*3+2;
					RfSwitch315.send(pRfCommand24Bit,24);
					digitalWrite(BUZZ, LOW);
				}
				else if (tempRfCommand.RfIndex >= 13)
				{
					RfSwitch433.send_duya(RfWindow[(tempRfCommand.RfIndex-13)*3+2],39);
				}


				break;
			//case RF_OP_433:

			//	break;
			}


		}
	}
}


unsigned long LastMillis = 0;
void TenthSecondsSinceStartTask()
{
	unsigned long CurrentMillis = millis();
	if (abs(CurrentMillis - LastMillis) > 100)
	{
		LastMillis = CurrentMillis;
		TenthSecondsSinceStart++;
		OnTenthSecond();
	}
}

void OnSecond()
{
	time_t now = time(nullptr); //��ȡ��ǰʱ��
	time_str = ctime(&now);
	H1 = time_str[11];
	H2 = time_str[12];
	M1 = time_str[14];
	M2 = time_str[15];
	S1 = time_str[17];
	S2 = time_str[18];
	//printf("%c%c:%c%c:%c%c\n",H1,H2,M1,M2,S1,S2);
	//Serial.printf(time_str);

	struct   tm     *timenow;
	timenow   =   localtime(&now);
	unsigned char Hour = timenow->tm_hour;
	unsigned char Minute = timenow->tm_min;



	//RfSwitch315.send(12811368, 24);//��̨1�ŵ�
	//RfSwitch315.send(0xC37C64, 24);//��̨2�ż̵���



	//every minute
	if (now%60 == 0)
	{

	}


	m_WiFiUDP.beginPacket("192.168.0.17", 5050);
	m_WiFiUDP.write((const char*)&RfData, sizeof(tRfData));
	m_WiFiUDP.endPacket(); 

}

void OnTenthSecond()
{

	if (TenthSecondsSinceStart%10 == 0)
	{
		OnSecond();
	}


}



void MyPrintf(const char *fmt, ...)
{


	static char send_buf[1024];
	char * sprint_buf = send_buf+sizeof(tDebugData);
	tDebugData * pDebugData = (tDebugData*)send_buf;

	int n;
	va_list args;

	va_start(args,fmt);
	n = vsprintf(sprint_buf, fmt, args);
	va_end(args);

	printf(sprint_buf);

	pDebugData->DataType = 3;
	pDebugData->RoomId = RoomIndex;
	pDebugData->Length = n;

	m_WiFiUDP.beginPacket("192.168.0.17", 5050);
	m_WiFiUDP.write((const char*)send_buf, sizeof(tDebugData)+n);
	m_WiFiUDP.endPacket(); 


}