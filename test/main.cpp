#define F_CPU 1600000UL
#include <avr/io.h>
#include <util/twi.h> //--- Give Status of I2C Bus will operation
#include <util/delay.h>
#include <stdbool.h>       /* Include standard boolean library */
#include <string.h>        /* Include string library */
#include <stdio.h>         /* Include standard IO library */
#include <stdlib.h>        /* Include standard library */
#include <avr/interrupt.h> /* Include avr interrupt header file */
#include <math.h>

#define SENSOR_DDR DDRA
#define TEMPERATURE_SENSOR_PIN 1
#define MOISTURE_SENSOR_PIN 2
#define SENSOR_PORT_OUTPUT PORTA
#define SENSOR_PORT_INPUT PINA

#define WATER_MOTOR_PIN 4
#define PELTIER_MOTOR_PIN 5
#define ACTUATOR_COMMON_GROUND 3

#define TEMP_MODE 1
#define HUMIDITY_MODE 2
#define SOIL_MODE 3

#define DDR_MODE_SWITCH DDRB
#define MODE_SWITCH 5
#define INDICATOR_LED 6
#define INDICATOR_LED_2 7

#define SENSOR_DELAY 50
#define DATABASE_DELAY 200

#define SCL_CLK 10000L
#define BITRATE(TWSR) 21
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

#define PCF8574 0x27
#define WRITE 0
#define READ 1

#define SREG _SFR_IO8(0x3F)

#define DEFAULT_BUFFER_SIZE 250
#define DEFAULT_TIMEOUT 5000

/* Connection Mode */
#define SINGLE 0
#define MULTIPLE 1

/* Application Mode */
#define NORMAL 0
#define TRANSPERANT 1

/* Application Mode */
#define STATION 1
#define ACCESSPOINT 2
#define BOTH_STATION_AND_ACCESPOINT 3

/* Select Demo */
#define RECEIVE_DEMO

#define DOMAIN "api.thingspeak.com"
#define PORT "80"
#define API_WRITE_KEY "R92FOM2FF6I40JL1"
#define CHANNEL_ID "2264916"
#define CHANNEL_ID_THRES "1870593"

// #define SSID				"SLT_FIBRE HOME"
// #define PASSWORD			"home922305*7"

#define SSID "UOC_Staff"
#define PASSWORD "admin106"

#define SREG _SFR_IO8(0x3F)

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/twi.h> //--- Give Status of I2C Bus will operation
#include <util/delay.h>
#include <twi_lcd.h>

unsigned char numbers_in_binary[10] = {0b01001111, 0b00110001, 0b00110010, 0b00110011, 0b00110100, 0b00110101, 0b00110110, 0b00110111, 0b00111000, 0b00111001};
int RH_I = 0, RH_D = 0, temp_I = 0, temp_D = 0, checksum = 0, dataByte = 0, current_mode = 0, soil_moisture = 0, adc_value = 0;
int tempe_threshold, soil_threshold;

long execution_counter = 0;
int temperature_last_read_at = 0;
int database_transaction_last_occured_at = 0;

void setThrsholdValues(int *temp_thresh, int *soil_thresh);
void activate_actuators();
int change_sensor(int parameter);
void update_lcd_value_temp(int int_val, int deci_val);
void update_lcd_value_soil(int val);
void update_lcd_value_humidity(int int_val, int deci_val);
void init_sensors(void);
void request_temperature_sensor(void);
void response_temperature_sensor(void);
int read_DHT11(void);

char USART_RxChar();
void USART_TxChar(char);
void USART_SendString(char *);
void send_sensor_values_to_database(int temperature, int humidity, int s_moisture);

int8_t Response_Status;
volatile int16_t Counter = 0, pointer = 0;
uint32_t TimeOut = 0;
char RESPONSE_BUFFER[DEFAULT_BUFFER_SIZE];
char _buffer[150];

void print_message_on_lcd(char print_str[])
{
    twi_lcd_clear();
    twi_lcd_msg(print_str);
    //_delay_ms(1000);
    // twi_lcd_clear();
}

enum ESP8266_RESPONSE_STATUS
{
    ESP8266_RESPONSE_WAITING,
    ESP8266_RESPONSE_FINISHED,
    ESP8266_RESPONSE_TIMEOUT,
    ESP8266_RESPONSE_BUFFER_FULL,
    ESP8266_RESPONSE_STARTING,
    ESP8266_RESPONSE_ERROR
};

enum ESP8266_CONNECT_STATUS
{
    ESP8266_CONNECTED_TO_AP,
    ESP8266_CREATED_TRANSMISSION,
    ESP8266_TRANSMISSION_DISCONNECTED,
    ESP8266_NOT_CONNECTED_TO_AP,
    ESP8266_CONNECT_UNKNOWN_ERROR
};

enum ESP8266_JOINAP_STATUS
{
    ESP8266_WIFI_CONNECTED,
    ESP8266_CONNECTION_TIMEOUT,
    ESP8266_WRONG_PASSWORD,
    ESP8266_NOT_FOUND_TARGET_AP,
    ESP8266_CONNECTION_FAILED,
    ESP8266_JOIN_UNKNOWN_ERROR
};

void Read_Response(char *_Expected_Response)
{
    int EXPECTED_RESPONSE_LENGTH = strlen(_Expected_Response);
    uint32_t TimeCount = 0, ResponseBufferLength = 0;
    char RECEIVED_CRLF_BUF[EXPECTED_RESPONSE_LENGTH];
    while (1)
    {
        if (TimeCount >= (DEFAULT_TIMEOUT + TimeOut))
        {
            TimeOut = 0;
            Response_Status = ESP8266_RESPONSE_TIMEOUT;
            // print_message_on_lcd("Timeout!");
            return;
        }
        if (Response_Status == ESP8266_RESPONSE_STARTING)
        {
            Response_Status = ESP8266_RESPONSE_WAITING;
        }
        ResponseBufferLength = strlen(RESPONSE_BUFFER);
        // print_message_on_lcd((unsigned char)ResponseBufferLength);
        if (ResponseBufferLength)
        {
            _delay_ms(1);
            TimeCount++;
            // print_message_on_lcd(RESPONSE_BUFFER);
            //_delay_ms(1000);
            if (ResponseBufferLength == strlen(RESPONSE_BUFFER))
            {
                // print_message_on_lcd(RESPONSE_BUFFER);
                for (uint16_t i = 0; i < ResponseBufferLength; i++)
                {
                    memmove(RECEIVED_CRLF_BUF, RECEIVED_CRLF_BUF + 1, EXPECTED_RESPONSE_LENGTH - 1);
                    RECEIVED_CRLF_BUF[EXPECTED_RESPONSE_LENGTH - 1] = RESPONSE_BUFFER[i];
                    if (!strncmp(RECEIVED_CRLF_BUF, _Expected_Response, EXPECTED_RESPONSE_LENGTH))
                    {
                        TimeOut = 0;
                        Response_Status = ESP8266_RESPONSE_FINISHED;
                        PORTC = PORTC | (1 << INDICATOR_LED_2);
                        // print_message_on_lcd("Finished!");
                        return;
                    }
                }
            }
        }
        _delay_ms(1);
        TimeCount++;
    }
}

void ESP8266_Clear()
{
    memset(RESPONSE_BUFFER, 0, DEFAULT_BUFFER_SIZE);
    Counter = 0;
    pointer = 0;
}

void Start_Read_Response(char *_ExpectedResponse)
{
    Response_Status = ESP8266_RESPONSE_STARTING;
    do
    {
        Read_Response(_ExpectedResponse);
    } while (Response_Status == ESP8266_RESPONSE_WAITING);
}

void GetResponseBody(char *Response, uint16_t ResponseLength)
{

    uint16_t i = 12;
    char buffer[5];
    while (Response[i] != '\r')
        ++i;

    strncpy(buffer, Response + 12, (i - 12));
    ResponseLength = atoi(buffer);

    i += 2;
    uint16_t tmp = strlen(Response) - i;
    memcpy(Response, Response + i, tmp);

    if (!strncmp(Response + tmp - 6, "\r\nOK\r\n", 6))
        memset(Response + tmp - 6, 0, i + 6);
}

bool WaitForExpectedResponse(char *ExpectedResponse)
{
    Start_Read_Response(ExpectedResponse);
    if ((Response_Status != ESP8266_RESPONSE_TIMEOUT))
        return true;
    return false;
}

bool SendATandExpectResponse(char *ATCommand, char *ExpectedResponse)
{
    ESP8266_Clear();
    USART_SendString(ATCommand); /* Send AT command to ESP8266 */
    USART_SendString("\r\n");
    _delay_ms(200);
    return WaitForExpectedResponse(ExpectedResponse);
}

bool ESP8266_ApplicationMode(uint8_t Mode)
{
    char _atCommand[20];
    memset(_atCommand, 0, 20);
    sprintf(_atCommand, "AT+CIPMODE=%d", Mode);
    _atCommand[19] = 0;
    return SendATandExpectResponse(_atCommand, "\r\nOK\r\n");
}

bool ESP8266_ConnectionMode(uint8_t Mode)
{
    char _atCommand[20];
    memset(_atCommand, 0, 20);
    sprintf(_atCommand, "AT+CIPMUX=%d", Mode);
    _atCommand[19] = 0;
    return SendATandExpectResponse(_atCommand, "\r\nOK\r\n");
}

bool ESP8266_Begin()
{
    for (uint8_t i = 0; i < 5; i++)
    {
        if (SendATandExpectResponse("AT", "\r\nOK\r\n"))
            return true;
    }
    return false;
}

bool ESP8266_Close()
{

    return SendATandExpectResponse("AT+CIPCLOSE=1", "\r\nOK\r\n");
}

bool ESP8266_WIFIMode(uint8_t _mode)
{
    char _atCommand[20];
    memset(_atCommand, 0, 20);
    sprintf(_atCommand, "AT+CWMODE=%d", _mode);
    _atCommand[19] = 0;
    return SendATandExpectResponse(_atCommand, "OK");
}

uint8_t ESP8266_JoinAccessPoint(char *_SSID, char *_PASSWORD)
{
    char _atCommand[60];
    memset(_atCommand, 0, 60);
    _atCommand[59] = 0;
    sprintf(_atCommand, "AT+CWJAP=\"%s\",\"%s\"", _SSID, _PASSWORD);
    if (SendATandExpectResponse(_atCommand, "WIFI CONNECTED"))
    {
        return ESP8266_WIFI_CONNECTED;
    }
    else
    {
        if (strstr(RESPONSE_BUFFER, "+CWJAP:1"))
            return ESP8266_CONNECTION_TIMEOUT;
        else if (strstr(RESPONSE_BUFFER, "+CWJAP:2"))
            return ESP8266_WRONG_PASSWORD;
        else if (strstr(RESPONSE_BUFFER, "+CWJAP:3"))
            return ESP8266_NOT_FOUND_TARGET_AP;
        else if (strstr(RESPONSE_BUFFER, "+CWJAP:4"))
            return ESP8266_CONNECTION_FAILED;
        else
            return ESP8266_JOIN_UNKNOWN_ERROR;
    }
}

uint8_t ESP8266_connected()
{
    SendATandExpectResponse("AT+CIPSTATUS", "\r\nOK\r\n");
    if (strstr(RESPONSE_BUFFER, "STATUS:2"))
        return ESP8266_CONNECTED_TO_AP;
    else if (strstr(RESPONSE_BUFFER, "STATUS:3"))
        return ESP8266_CREATED_TRANSMISSION;
    else if (strstr(RESPONSE_BUFFER, "STATUS:4"))
        return ESP8266_TRANSMISSION_DISCONNECTED;
    else if (strstr(RESPONSE_BUFFER, "STATUS:5"))
        return ESP8266_NOT_CONNECTED_TO_AP;
    else
        return ESP8266_CONNECT_UNKNOWN_ERROR;
}

uint8_t ESP8266_Start(uint8_t _ConnectionNumber, char *Domain, char *Port)
{
    bool _startResponse;
    char _atCommand[60] = "";
    memset(_atCommand, 0, 60);

    /*if(SendATandExpectResponse("AT+CIPMUX?", "CIPMUX:0"))
      sprintf(_atCommand, "AT+CIPSTART=\"TCP\",\"%s\",%s", Domain, Port);
    else
      sprintf(_atCommand, "AT+CIPSTART=\"%d\",\"TCP\",\"%s\",%s", _ConnectionNumber, Domain, Port);*/

    sprintf(_atCommand, "AT+CIPSTART=\"TCP\",\"%s\",%d", Domain, Port);
    // print_message_on_lcd(_atCommand);
    //_delay_ms(1000);
    _startResponse = SendATandExpectResponse(_atCommand, "\r\nCONNECT\r\n");

    if (!_startResponse)
    {
        if (Response_Status == ESP8266_RESPONSE_TIMEOUT)
        {
            print_message_on_lcd("RESPONSE TIMEOUT");
            _delay_ms(2000);
            return ESP8266_RESPONSE_TIMEOUT;
        }
        else
        {
            print_message_on_lcd("RESPONSE ERROR");
            _delay_ms(2000);
            return ESP8266_RESPONSE_ERROR;
        }
    }
    return ESP8266_RESPONSE_FINISHED;
}

uint8_t ESP8266_Send(char *Data)
{
    char _atCommand[20] = "";
    memset(_atCommand, 0, 20);
    sprintf(_atCommand, "AT+CIPSEND=%d", (strlen(Data) + 2));
    //_atCommand[19] = 0;
    SendATandExpectResponse(_atCommand, "\r\nOK\r\n");
    if (!SendATandExpectResponse(Data, "\r\nSEND OK\r\n"))
    {
        if (Response_Status == ESP8266_RESPONSE_TIMEOUT)
            return ESP8266_RESPONSE_TIMEOUT;
        return ESP8266_RESPONSE_ERROR;
    }
    return ESP8266_RESPONSE_FINISHED;
}

int16_t ESP8266_DataAvailable()
{
    return (Counter - pointer);
}

uint8_t ESP8266_DataRead()
{
    if (pointer < Counter)
        return RESPONSE_BUFFER[pointer++];
    else
    {
        ESP8266_Clear();
        return 0;
    }
}

int charArrayToInt(char *arr, int length)
{
    int i = 0, value = 0, r = 0;
    i = value = 0;
    for (i = 0; i < length; i++)
    {
        r = (int)arr[i];
        // twi_lcd_clear();
        // twi_lcd_dwr(r);
        //_delay_ms(2000);
        value = (value * 10) + r;
    }
    return value;
}

uint16_t Read_Data(char *_buffer)
{
    int len = 0;
    _delay_ms(100);
    while (ESP8266_DataAvailable() > 0)
    {
        _buffer[len++] = ESP8266_DataRead();
    }

    int temperature_int_value = 0;
    char temperature_threshold[5] = "";
    for (int c = 0; c < len; c++)
    {
        if (_buffer[c] == 'f' && _buffer[c + 1] == 'i' && _buffer[c + 2] == 'e' && _buffer[c + 3] == 'l' && _buffer[c + 4] == 'd' && _buffer[c + 5] == '1')
        {
            char current_character;
            int x = c + 9;
            int position = 0;
            current_character = _buffer[x];
            while (current_character != '"')
            {
                /*twi_lcd_clear();
                twi_lcd_dwr(current_character);
                _delay_ms(250);*/

                temperature_threshold[position] = current_character;
                x = x + 1;
                position = position + 1;
                current_character = _buffer[x];
            }
            // print_message_on_lcd(temperature_threshold);
            //_delay_ms(1000);
            int m;
            sscanf(temperature_threshold, "%d", &m);
            temperature_int_value = m;
            break;
        }
    }

    int soil_moisture_int_value = 0;
    char soil_moisture_threshold[5] = "";
    for (int c = 0; c < len; c++)
    {
        if (_buffer[c] == 'f' && _buffer[c + 1] == 'i' && _buffer[c + 2] == 'e' && _buffer[c + 3] == 'l' && _buffer[c + 4] == 'd' && _buffer[c + 5] == '2')
        {
            char current_character;
            int x = c + 9;
            int position = 0;
            current_character = _buffer[x];
            while (current_character != '"')
            {
                /*twi_lcd_clear();
                twi_lcd_dwr(current_character);
                _delay_ms(250);*/
                soil_moisture_threshold[position] = current_character;
                x = x + 1;
                position = position + 1;
                current_character = _buffer[x];
            }
            // print_message_on_lcd(soil_moisture_threshold);
            //_delay_ms(1000);
            int n;
            sscanf(soil_moisture_threshold, "%d", &n);
            soil_moisture_int_value = n;
            break;
        }
    }

    // print_message_on_lcd(tempe_threshold);
    //_delay_ms(2000);
    setThrsholdValues(temperature_int_value, soil_moisture_int_value);
    return len;
}

void USART_Init(long USART_BAUDRATE) /* USART initialize function */
{

    UCSRB = (1 << TXEN) | (1 << RXEN) | (1 << RXCIE);
    UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0);
    // UBRRL= (unsigned char)BAUD_PRESCALE;
    // UBRRH= (unsigned char)(BAUD_PRESCALE>>8);

    UBRRL = (unsigned char)0x0C;
    UBRRH = (unsigned char)(0x0C >> 8);
}

char USART_RxChar() /* Data receiving function */
{
    while (!(UCSRA & (1 << RXC)))
        ;         /* Wait until new data receive */
    return (UDR); /* Get and return received data */
}

void USART_TxChar(char data) /* Data transmitting function */
{
    UDR = data; /* Write data to be transmitting in UDR */
    while (!(UCSRA & (1 << UDRE)))
        ; /* Wait until data transmit and buffer get empty */
          // print_message_on_lcd(data);
}

void USART_SendString(char *str) /* Send string of USART data function */
{
    int i = 0;
    while (str[i] != 0)
    {
        // PORTC=PORTC|(1<<INDICATOR_LED);
        USART_TxChar(str[i]); /* Send each char of string till the NULL */
        i++;
        // PORTC &= ~(1<<INDICATOR_LED);
    }
}

void setThrsholdValues(int *temperature_threshold_param, int *soil_threshold_param)
{
    tempe_threshold = temperature_threshold_param;
    soil_threshold = soil_threshold_param;
}

void send_sensor_values_to_database(int temperature, int humidity, int s_moisture)
{
    print_message_on_lcd("POSTING VALUES..");
    ESP8266_Start(0, DOMAIN, PORT);
    _delay_ms(500);
    char _buffer[150] = "";
    memset(_buffer, 0, 150);
    sprintf(_buffer, "GET /update?api_key=%s&field1=%d&field2=%d&field3=%d", API_WRITE_KEY, temperature, humidity, s_moisture);
    ESP8266_Send(_buffer);
}

void get_threshold_values_from_database()
{
    print_message_on_lcd("GETTING LIMITS..");
    ESP8266_Start(0, DOMAIN, PORT);
    _delay_ms(500);
    char _buffer[200] = "";
    memset(_buffer, 0, 200);
    sprintf(_buffer, "GET /channels/%s/feeds/last.txt", CHANNEL_ID_THRES);
    ESP8266_Send(_buffer);
    _delay_ms(1000);
    Read_Data(_buffer);
}

int main(void)
{
    int Connect_Status = 0;

    sei();
    twi_init();
    twi_lcd_init();
    print_message_on_lcd("GROW SMART");
    _delay_ms(1000);
    USART_Init(4800);
    while (!ESP8266_Begin())
    {
    };
    print_message_on_lcd("ESP ONLINE");
    _delay_ms(1000);
    ESP8266_WIFIMode(STATION);
    print_message_on_lcd("WIFI MODE1 DONE");
    _delay_ms(1000);
    ESP8266_ConnectionMode(SINGLE);
    print_message_on_lcd("WIFI MODE2 DONE");
    _delay_ms(1000);
    ESP8266_ApplicationMode(NORMAL);
    print_message_on_lcd("WIFI MODE3 DONE");
    _delay_ms(1000);
    Connect_Status = ESP8266_JoinAccessPoint(SSID, PASSWORD);
    if (Connect_Status == ESP8266_CONNECTED_TO_AP)
    {
        print_message_on_lcd("Connected to WIFI!");
    }
    else if (Connect_Status == ESP8266_NOT_CONNECTED_TO_AP)
    {
        print_message_on_lcd("Not Connected to WIFI");
    }
    else
    {
        print_message_on_lcd("WIFI Error");
    }
    _delay_ms(7000);
    // current_mode=TEMP_MODE;
    // change_sensor(current_mode);
    /*ESP8266_Clear();
    get_threshold_values_from_database();
    _delay_ms(1000);*/
    // init_sensors();
    while (1)
    {
        /*if(calc_execute_status_using_time(temperature_last_read_at,SENSOR_DELAY))
        {
          get_sensor_values_and_control_actuators();
          temperature_last_read_at=execution_counter;
          change_sensor(current_mode);
        }

        if(calc_execute_status_using_time(database_transaction_last_occured_at,DATABASE_DELAY))
        {
          ESP8266_Clear();
          send_sensor_values_to_database(temp_I,RH_I,(1023-adc_value));
          _delay_ms(500);
          get_sensor_values_and_control_actuators();
          ESP8266_Clear();
          get_threshold_values_from_database();
          _delay_ms(500);
          get_sensor_values_and_control_actuators();
          database_transaction_last_occured_at=execution_counter;
          change_sensor(current_mode);
        }
        temp_I=28;
        temp_D=6;
        RH_I=77;
        RH_I=77;
        adc_value=ADC_Read();

        if(current_mode==TEMP_MODE){
          update_lcd_value_temp(temp_I,temp_D);
        }
        if(current_mode==HUMIDITY_MODE){
          update_lcd_value_humidity(RH_I,RH_D);
        }
        if(current_mode==SOIL_MODE){
          update_lcd_value_soil(1023-adc_value);
        }
        while(isModeSwitchPressed()){
          if(current_mode==TEMP_MODE){
            current_mode=HUMIDITY_MODE;
            change_sensor(current_mode);
          }
          else if(current_mode==HUMIDITY_MODE){
            current_mode=SOIL_MODE;
            change_sensor(current_mode);
          }
          else if(current_mode==SOIL_MODE){
            current_mode=TEMP_MODE;
            change_sensor(current_mode);
          }
          _delay_ms(100);
        }
        activate_actuators();
        execution_counter++;	*/
    }
    return 0;
}

ISR(USART_RXC_vect)
{

    PORTC = PORTC | (1 << INDICATOR_LED_2);
    RESPONSE_BUFFER[Counter] = UDR;
    Counter++;
    if (Counter == DEFAULT_BUFFER_SIZE)
    {
        Counter = 0;
        pointer = 0;
    }
    PORTC &= ~(1 << INDICATOR_LED_2);
    UCSRA |= (1 << RXC);
}
