 #define F_CPU 8000000UL

#include <avr/io.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "LCD_20x4_H_file.h"
#include "USART_Interrupt.h"

#define SREG    _SFR_IO8(0x3F)

#define URL			"api.thingspeak.com/update"/* Server Name */
#define API_WRITE_KEY	"RHNEE35WXX0PL7LS"	
#define CHANNEL_ID		"2314887"	/* Channel ID */
#define APN			"Dialog internet"	/* APN of GPRS n/w provider */
#define USERNAME		"saumya"
#define PASSWORD		""  

#define DEFAULT_BUFFER_SIZE	200	/* Define default buffer size */
#define DEFAULT_TIMEOUT		20000	/* Define default timeout */
#define DEFAULT_CRLF_COUNT	2	/* Define default CRLF count */

#define POST			1	/* Define method */
#define GET			0

/* Select Demo */
#define GET_DEMO			/* Define GET demo */
#define POST_DEMO			/* Define POST demo */





void convert_time_to_UTC(void);
void convert_to_degrees(char *);

#define Buffer_Size 150
#define degrees_buffer_size 20

char Latitude_Buffer[15], Longitude_Buffer[15];
char degrees_buffer[degrees_buffer_size];

char GGA_Buffer[Buffer_Size];
uint8_t GGA_Pointers[20];
char GGA_CODE[3];

volatile uint16_t GGA_Index, CommaCounter;

bool IsItGGAString = false;

enum SIM900_RESPONSE_STATUS		/* Enumerate response status */
{
	SIM900_RESPONSE_WAITING,
	SIM900_RESPONSE_FINISHED,
	SIM900_RESPONSE_TIMEOUT,
	SIM900_RESPONSE_BUFFER_FULL,
	SIM900_RESPONSE_STARTING,
	SIM900_RESPONSE_ERROR
};

char Response_Status, CRLF_COUNT = 0;
uint16_t Counter = 0;
uint32_t TimeOut = 0;
char RESPONSE_BUFFER[DEFAULT_BUFFER_SIZE];
 



void get_latitude(uint16_t lat_pointer) {
    cli();
    uint8_t lat_index = 0;
    uint8_t index = lat_pointer + 1;

    while (GGA_Buffer[index] != ',' && GGA_Buffer[index] != '\0') {
        Latitude_Buffer[lat_index] = GGA_Buffer[index];
        lat_index++;
        index++;
    }

    Latitude_Buffer[lat_index] = '\0';

    if (GGA_Buffer[index] == ',') {
        Latitude_Buffer[lat_index++] = GGA_Buffer[index++];
        Latitude_Buffer[lat_index] = GGA_Buffer[index];
    }

    convert_to_degrees(Latitude_Buffer);
    sei();
}

void get_longitude(uint16_t long_pointer) {
    cli();
    uint8_t long_index = 0;
    uint8_t index = long_pointer + 1;

    for (; GGA_Buffer[index] != ','; index++) {
        Longitude_Buffer[long_index] = GGA_Buffer[index];
        long_index++;
    }

    Longitude_Buffer[long_index++] = GGA_Buffer[index++];
    Longitude_Buffer[long_index] = GGA_Buffer[index];

    convert_to_degrees(Longitude_Buffer);
    sei();
}





void convert_to_degrees(char *raw) {
    double value;
    float decimal_value, temp;
    int32_t degrees;
    float position;

    value = atof(raw);

    decimal_value = (value / 100);
    degrees = (int)(decimal_value);
    temp = (decimal_value - (int)decimal_value) / 0.6;
    position = (float)degrees + temp;

    dtostrf(position, 6, 4, degrees_buffer);
}

void Read_Response(void)		/* Read response */
{
   static char CRLF_BUF[2];
   static char CRLF_FOUND;
   uint32_t TimeCount = 0, ResponseBufferLength;
   while(1)
   {
      if(TimeCount >= (DEFAULT_TIMEOUT+TimeOut))
	{
	   CRLF_COUNT = 0; TimeOut = 0;
	   Response_Status = SIM900_RESPONSE_TIMEOUT;
	   return;
	}

      if(Response_Status == SIM900_RESPONSE_STARTING)
	{
	   CRLF_FOUND = 0;
	   memset(CRLF_BUF, 0, 2);
	   Response_Status = SIM900_RESPONSE_WAITING;
	}
      ResponseBufferLength = strlen(RESPONSE_BUFFER);
      if (ResponseBufferLength)
	{
	   _delay_ms(1);
	   TimeCount++;
	   if (ResponseBufferLength==strlen(RESPONSE_BUFFER))
	      {
		for (uint16_t i=0;i<ResponseBufferLength;i++)
		{
		   memmove(CRLF_BUF, CRLF_BUF + 1, 1);
		   CRLF_BUF[1] = RESPONSE_BUFFER[i];
		   if(!strncmp(CRLF_BUF, "\r\n", 2))
		   {
		      if(++CRLF_FOUND == (DEFAULT_CRLF_COUNT+CRLF_COUNT))
			{
			   CRLF_COUNT = 0; TimeOut = 0;
			   Response_Status = SIM900_RESPONSE_FINISHED;
			   return;
			}
		   }
		}
	   CRLF_FOUND = 0;
	}
      }
      _delay_ms(1);
      TimeCount++;
   }
}

void Start_Read_Response(void)
{
	Response_Status = SIM900_RESPONSE_STARTING;
	do {
		Read_Response();
	} while(Response_Status == SIM900_RESPONSE_WAITING);
}

void Buffer_Flush(void)		/* Flush all variables */
{
	memset(RESPONSE_BUFFER, 0, DEFAULT_BUFFER_SIZE);
	Counter=0;
}

/* Remove CRLF and other default strings from response */ 
void GetResponseBody(char* Response, uint16_t ResponseLength)
{
	uint16_t i = 12;
	char buffer[5];
	while(Response[i] != '\r' && i < 100)
		++i;

	strncpy(buffer, Response + 12, (i - 12));
	ResponseLength = atoi(buffer);

	i += 2;
	uint16_t tmp = strlen(Response) - i;
	memcpy(Response, Response + i, tmp);

	if(!strncmp(Response + tmp - 6, "\r\nOK\r\n", 6))
	memset(Response + tmp - 6, 0, i + 6);
}

bool WaitForExpectedResponse(char* ExpectedResponse)
{
	Buffer_Flush();
	_delay_ms(200);
	Start_Read_Response();		/* First read response */
	if((Response_Status != SIM900_RESPONSE_TIMEOUT) && (strstr(RESPONSE_BUFFER, ExpectedResponse) != NULL))
		return true;		/* Return true for success */
	return false;			/* Else return false */
}

bool SendATandExpectResponse(char* ATCommand, char* ExpectedResponse)
{
	USART_SendString(ATCommand);	/* Send AT command to SIM900 */
	UART_TxChar('\r');
	return WaitForExpectedResponse(ExpectedResponse);
}

bool HTTP_Parameter(char* Parameter, char* Value)/* Set HTTP parameter and return response */
{
	
	USART_SendString("AT+HTTPPARA=\"");
	USART_SendString(Parameter);
	USART_SendString("\",\"");
	USART_SendString(Value);
	USART_SendString("\"\r");
	return WaitForExpectedResponse("OK");
}

bool SIM900HTTP_Start(void)			/* Check SIM900 board */
{
	for (uint8_t i=0;i<5;i++)
	{
		if(SendATandExpectResponse("ATE0","OK")||SendATandExpectResponse("AT","OK"))
		{
			HTTP_Parameter("CID","1");
			return true;
		}
	}
	return false;
}
	
bool SIM900HTTP_Connect(char* _APN, char* _USERNAME, char* _PASSWORD) /* Connect to GPRS */
{

	USART_SendString("AT+CREG?\r");
	if(!WaitForExpectedResponse("+CREG: 0,1"))
		return false;

	USART_SendString("AT+SAPBR=0,1\r");
	WaitForExpectedResponse("OK");

	USART_SendString("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"\r");
	WaitForExpectedResponse("OK");

	USART_SendString("AT+SAPBR=3,1,\"APN\",\"");
	USART_SendString(_APN);
	USART_SendString("\"\r");
	WaitForExpectedResponse("OK");

	USART_SendString("AT+SAPBR=3,1,\"USER\",\"");
	USART_SendString(_USERNAME);
	USART_SendString("\"\r");
	WaitForExpectedResponse("OK");

	USART_SendString("AT+SAPBR=3,1,\"PWD\",\"");
	USART_SendString(_PASSWORD);
	USART_SendString("\"\r");
	WaitForExpectedResponse("OK");

	USART_SendString("AT+SAPBR=1,1\r");
	return WaitForExpectedResponse("OK");
}

bool HTTP_Init(void)		/* Initiate HTTP */
{
	USART_SendString("AT+HTTPINIT\r");
	return WaitForExpectedResponse("OK");
}

bool HTTP_Terminate(void)		/* terminate HTTP */
{
	USART_SendString("AT+HTTPTERM\r");
	return WaitForExpectedResponse("OK");
}

bool HTTP_SetURL(char * url)	/* Set URL */
{
	return HTTP_Parameter("URL", url);
}

bool HTTP_Connected(void)		/* Check for connected */
{
	USART_SendString("AT+SAPBR=2,1\r");
	CRLF_COUNT = 2;										/* Make additional crlf count for response */
	return WaitForExpectedResponse("+SAPBR: 1,1");
}

bool HTTP_SetPost_json(void)	/* Set Json Application format for post */
{
	return HTTP_Parameter("CONTENT", "application/json");
}

bool HTTP_Save(void)		/* Save the application context */
{
	USART_SendString("AT+HTTPSCONT\r");
	return WaitForExpectedResponse("OK");
}

bool HTTP_Data(char* data)	/* Load HTTP data */
{
	char _buffer[25];
	sprintf(_buffer, "AT+HTTPDATA=%d,%d\r", strlen(data), 10000);
	USART_SendString(_buffer);
	
	if(WaitForExpectedResponse("DOWNLOAD"))
		return SendATandExpectResponse(data, "OK");
	else
		return false;
}

bool HTTP_Action(char method)	/* Select HTTP Action */
{
	if(method == GET)
		USART_SendString("AT+HTTPACTION=0\r");
	if(method == POST)
		USART_SendString("AT+HTTPACTION=1\r");
	return WaitForExpectedResponse("OK");
}

bool HTTP_Read(uint8_t StartByte, uint16_t ByteSize) /* Read HTTP response */
{
	char Command[25];
	sprintf(Command,"AT+HTTPREAD=%d,%d\r",StartByte,ByteSize);
	Command[24] = 0;
	USART_SendString(Command);

	CRLF_COUNT = 2;										/* Make additional crlf count for response */
	if(WaitForExpectedResponse("+HTTPREAD"))
	{
		GetResponseBody(RESPONSE_BUFFER, ByteSize);
		return true;
	}
	else
		return false;
}

uint8_t HTTP_Post(char* Parameters, uint16_t ResponseLength)
{
	HTTP_Parameter("CID","1");
	if(!(HTTP_Data(Parameters) && HTTP_Action(POST)))
	return SIM900_RESPONSE_TIMEOUT;

	bool status200 = WaitForExpectedResponse(",200,");

	if(Response_Status == SIM900_RESPONSE_TIMEOUT)
	return SIM900_RESPONSE_TIMEOUT;
	if(!status200)
	return SIM900_RESPONSE_ERROR;

	HTTP_Read(0, ResponseLength);
	return SIM900_RESPONSE_FINISHED;
}

uint8_t HTTP_get(char * _URL, uint16_t ResponseLength)
{
	HTTP_Parameter("CID","1");
	HTTP_Parameter("URL", _URL);
	HTTP_Action(GET);
	WaitForExpectedResponse("+HTTPACTION:0,");
	if(Response_Status == SIM900_RESPONSE_TIMEOUT)
	return SIM900_RESPONSE_TIMEOUT;

	HTTP_Read(0, ResponseLength);
	return SIM900_RESPONSE_FINISHED;
}

bool SIM900HTTP_Init(void)
{
	HTTP_Terminate();
	return HTTP_Init();
}




int main(void) {
    GGA_Index = 0;
    memset(GGA_Buffer, 0, Buffer_Size);
    memset(degrees_buffer, 0, degrees_buffer_size);

    char _buffer[100];

    LCD_Init();
    _delay_ms(3000);
    UART_Init(9600);
    sei();


    // Variables to track if data has been sent
    uint8_t dataSent = 0;

    while (1) {
        _delay_ms(1000);
        
        // If data has not been sent, read GPS data and send it
        if (!dataSent) {
            if (!HTTP_Connected()) /* Check whether GPRS connected */
            {
                SIM900HTTP_Connect(APN, USERNAME, PASSWORD);
                SIM900HTTP_Init();
            }

            get_latitude(GGA_Pointers[0]);
            get_longitude(GGA_Pointers[2]);

            /* Take local buffer to copy response from the server */
            uint16_t responseLength = 100;

            #ifdef POST_DEMO /* POST Sample data on the server */
            memset(_buffer, 0, 100);
            HTTP_SetURL(URL);
            HTTP_Save();
            sprintf(_buffer, "api_key=%s&field1=%s&field2=%s", API_WRITE_KEY, Latitude_Buffer, Longitude_Buffer);
            HTTP_Post(_buffer, responseLength);
            _delay_ms(15000); /* Thingspeak server delay */
            #endif

            dataSent = 1; // Mark data as sent
        }

        // Display the GPS data
        LCD_String_xy(1, 0, "Lat: ");
		get_latitude(GGA_Pointers[0]);
        LCD_String(degrees_buffer);
        memset(degrees_buffer, 0, degrees_buffer_size);

        if (strlen(Latitude_Buffer) > 0) {
            LCD_String_xy(2, 0, "Long: ");
            get_longitude(GGA_Pointers[2]);
            LCD_String(degrees_buffer);
            memset(degrees_buffer, 0, degrees_buffer_size);
        }

        _delay_ms(300000); // Delay before getting the next set of data

        // Reset the dataSent flag to get the next set of data
        dataSent = 0;
    }
}

ISR(USART_RXC_vect) {
    uint8_t oldsrg = SREG;
    cli();
    char received_char = UDR;
	
	RESPONSE_BUFFER[Counter] =received_char;	/* Copy data to buffer & increment counter */
	Counter++;
	if(Counter == DEFAULT_BUFFER_SIZE)
		Counter = 0;

    if (received_char == '$') {
        GGA_Index = 0;
        CommaCounter = 0;
        IsItGGAString = false;
    } else if (IsItGGAString == true) {
        if (received_char == ',') GGA_Pointers[CommaCounter++] = GGA_Index;
        GGA_Buffer[GGA_Index++] = received_char;
    } else if (GGA_CODE[0] == 'G' && GGA_CODE[1] == 'G' && GGA_CODE[2] == 'A') {
        IsItGGAString = true;
        GGA_CODE[0] = 0;
        GGA_CODE[1] = 0;
        GGA_CODE[2] = 0;
    } else {
        GGA_CODE[0] = GGA_CODE[1];
        GGA_CODE[1] = GGA_CODE[2];
        GGA_CODE[2] = received_char;
    }
    SREG = oldsrg;
}

  
		

