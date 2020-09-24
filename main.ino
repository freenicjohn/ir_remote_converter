/* Code to intercept power-on/off command from fire TV stick and translate
 * it to crenova projector's power-on/off command
 * 
 * Nic John 9/17/2020
 */ 
 
#include <IRremote.h>

int IR_RECEIVE_PIN = 11;
int STATUS_PIN = LED_BUILTIN;

bool first_code;
bool second_code;

IRrecv irrecv(IR_RECEIVE_PIN);
IRsend irsend;

void setup() {
    Serial.begin(115200);
    delay(2000); // To be able to connect Serial monitor after reset and before first printout
    Serial.println(F("START " __FILE__ " from " __DATE__));

    irrecv.enableIRIn();  // Start the receiver
    irrecv.blink13(true); // Enable feedback LED

    pinMode(STATUS_PIN, OUTPUT);

    Serial.print(F("Ready to receive IR signals at pin "));
    Serial.println(IR_RECEIVE_PIN);
    Serial.print(F("Ready to send IR signals at pin "));
    Serial.println(IR_SEND_PIN);

    Serial.println(F("Waiting for fire tv power-on code..."));

    first_code = true;
    second_code = false;
}

// Storage for the recorded codes
unsigned long crenova_code;
unsigned long fire_code;
unsigned long current_code;
int crenova_code_len;
int fire_code_len;
int current_code_len;


// Stores a code
void storeCode(bool fire = false, bool crenova = false) {
    Serial.print("Received: ");
    if (irrecv.results.value == REPEAT) {
        // Don't record a NEC repeat value as that's useless.
        Serial.println("repeat; ignoring.");
        return;
    }
    Serial.println(irrecv.results.value);
    if (fire){
        fire_code = irrecv.results.value;
        fire_code_len = irrecv.results.bits;
    }
    else if (crenova){
        crenova_code = irrecv.results.value;
        crenova_code_len = irrecv.results.bits;
    }
    else {
        current_code = irrecv.results.value;
        current_code_len = irrecv.results.bits;
    }
}

// Sends the crenova code
void sendCode() {
    Serial.println("test");
    irsend.sendNEC(REPEAT, crenova_code_len);
    Serial.print("Sent: " + (crenova_code));
}

bool trigger_send = false;
int trigger_count = 0;

void loop() {
    if (trigger_send && trigger_count > 5) {
        trigger_send = false;
        trigger_count = 0;
        Serial.println("Done sending...");
        irrecv.enableIRIn(); // Re-enable receiver
    }
    if (trigger_send) {
        sendCode();
        trigger_count ++;
        delay(50); // Wait a bit between retransmissions
    }
    else if (irrecv.decode()) {
        digitalWrite(STATUS_PIN, HIGH);
        if (second_code) {  // crenova code
            storeCode(false, true);
            Serial.print("Stored crenova code: ");
            Serial.println(crenova_code);
            second_code = false;
        }
        else if (first_code) {  // fire tv code
            storeCode(true, false);
            Serial.print("Stored fire code: ");
            Serial.println(fire_code);
            Serial.println("Waiting for crenova tv power-on code...");
            first_code = false;
            second_code = true;
            delay(500);
        }
        else {
            storeCode();
            if (current_code == fire_code) {
                Serial.println("Received fire code. Sending crenova code...");
                trigger_send = true;
            }
        }
        irrecv.resume(); // resume receiver
        digitalWrite(STATUS_PIN, LOW);
    } 
}
