#include <Arduino.h> //needed for Serial.println
#include <string.h> //needed for memcpy

#define DEBUG 1
void consolePrint(const char* string);

int8_t sendATcommandUNI(char* ATcommand, char* expected_answer1,char* expected_answer2, unsigned int timeout, HardwareSerial *serial){
    uint8_t x=0,  answer=0;
    char response[512];
    unsigned long previous;
 
    memset(response, '\0', 512); // Clean response buffer
    
    delay(100); // Delay to be sure no passed commands interferee
    
    while( serial->available() > 0) serial->read();    // Wait for clean input buffer
    
    serial->println(ATcommand);    // Send the AT command 
    consolePrint(ATcommand);
    previous = millis();
 
    // this loop waits for the answer
    do{
        // if there are data in the UART input buffer, reads it and checks for the asnwer
        if(serial->available() != 0){    
            response[x] = serial->read();
            x++;
            // check if the desired answer is in the response of the module
            if (strstr(response, expected_answer1) != NULL){
                answer = 1;
            }
            if(expected_answer2){
               if (strstr(response, expected_answer2) != NULL){
                answer = 2;
               }
            }
        }
    // Waits for the answer with time out
    } while((answer == 0) && ((millis() - previous) < timeout));    
     consolePrint(response);
    return answer;
}

int8_t sendSMSATcommands(const char* text, const char* number,char* expected_answer, unsigned int timeout, HardwareSerial *serial){
    uint8_t x=0, answer=0;
    char response[512];
    unsigned long previous;
 
    memset(response, '\0', 512); // Clean response buffer
    
    delay(100); // Delay to be sure no passed commands interferee
    
    while( serial->available() > 0) serial->read();    // Wait for clean input buffer
    char ATcommand[50];
    sprintf(ATcommand, "AT+CMGS=\"%s\"\r", number);
    serial->println(ATcommand);    // Set number and AT command
    consolePrint(ATcommand);
    delay(100);
    serial->print(text);    // Set sms text
    consolePrint(text);
    delay(100);
    while( serial->available() > 0) serial->read();    // Wait for clean input buffer
    serial->println((char)26);   // Set escape char
    delay(100);
    
    previous = millis();
 
    // this loop waits for the answer
    do{
        // if there are data in the UART input buffer, reads it and checks for the asnwer
        if(serial->available() != 0){    
            response[x] = serial->read();
            x++;
            // check if the desired answer is in the response of the module
            if (strstr(response, expected_answer) != NULL){
                answer = 1;
            }
        }
    // Waits for the answer with time out
    } while((answer == 0) && ((millis() - previous) < timeout));    
    consolePrint(response);
    return answer;
}

void consolePrint(const char* string){
  if(DEBUG){
    Serial.println(string);
  }
}
