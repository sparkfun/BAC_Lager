/**
 * Breathalyzer
 * Author: Shawn Hymel
 * Date: September 2, 2015
 * 
 * Every minute, sound a buzzer. User must breathe into sensor to shut sensor
 * off. Calculate, display, and post BAC to data.sparkfun.
 * 
 * Data channel: https://data.sparkfun.com/sfe_bac
 * 
 * License: http://opensource.org/licenses/MIT
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// This #include statement was automatically added by the Particle IDE.
#include "SparkFun-Spark-Phant/SparkFun-Spark-Phant.h"

#include "BAC_Lookup.h"

// Constants
#define V_AREF      3.323   // Measured with DMM
#define V_IN        4.655   // Measured with DMM
#define MSG_SOF     0xA5
#define MSG_EOF     0x5A
#define BAC_START   410     // Beginning ADC value of BAC chart
#define BAC_END     859     // Lsat ADC value in BAC chart
#define LED_LEVEL_0 20      // BAC 0.00 - 0.02
#define LED_LEVEL_1 50      // BAC 0.02 - 0.05
#define LED_LEVEL_2 60      // BAC 0.05 - 0.06
#define LED_LEVEL_3 70      // BAC 0.06 - 0.07
#define LED_LEVEL_4 80      // BAC 0.07 - 0.08
#define LED_LEVEL_5 90      // BAC 0.08 - 0.09
#define LED_LEVEL_6 100     // BAC 0.09 - 0.10
#define LED_LEVEL_7 200     // You're drunk. Go home.
#define TIME_WAIT   58000   // Time between readings (ms)

// data.sparkfun Parameters
const char server[] = "data.sparkfun.com";          // Phant destination server
const char publicKey[] = "6JLlKWw1zoTrzWmNyg7m";    // Phant public key
const char privateKey[] = "Wwx1G24bkWh7egqR94yq";   // Phant private key
const char field[] = "bac";                      // Field for stream

// Pins
const int sensor_pin = A0;
const int buzzer_pin_1 = A1;
const int buzzer_pin_2 = A2;
const int button_pin = A3;
const int led_level_0 = D3;
const int led_level_1 = D4;
const int led_level_2 = D5;
const int led_level_3 = D6;
const int led_level_4 = D7;
const int led_level_5 = D0;
const int led_level_6 = D1;
const int led_level_7 = D2;

// Global variables
Phant phant(server, publicKey, privateKey);
int sensor_read;
uint8_t bac_read;
uint8_t button_state;
uint8_t button_prev;
bool do_sound;
uint8_t buzzer_state;
uint8_t s_idx;

void setup() {

    // Initialize pins
    pinMode(sensor_pin, INPUT);
    pinMode(buzzer_pin_1, OUTPUT);
    pinMode(buzzer_pin_2, OUTPUT);
    pinMode(button_pin, INPUT_PULLUP);
    pinMode(led_level_0, OUTPUT);
    pinMode(led_level_1, OUTPUT);
    pinMode(led_level_2, OUTPUT);
    pinMode(led_level_3, OUTPUT);
    pinMode(led_level_4, OUTPUT);
    pinMode(led_level_5, OUTPUT);
    pinMode(led_level_6, OUTPUT);
    pinMode(led_level_7, OUTPUT);
    
    // Set initial pin states
    digitalWrite(buzzer_pin_1, LOW);
    digitalWrite(buzzer_pin_2, LOW);
    
    // Initialize display (MicroView) and clear (0.000)
    Serial1.begin(9600);
    Serial1.write(0xA5);
    Serial1.write(0);
    Serial1.write(0);
    Serial1.write(0x5A);
}

void loop() {
    
    // Sound buzzer until button push
    do_sound = true;
    button_state = HIGH;
    button_prev = HIGH;
    buzzer_state = 0;
    while ( do_sound ) {
        
        // Make sound in bursts
        for ( s_idx = 0; s_idx < 200; s_idx++ ) {
            digitalWrite(buzzer_pin_1, buzzer_state);
            buzzer_state ^= 0x01;
            delay(1);
        }
        
        // Read BAC from sensor
        readBAC();
            
        // Send BAC level to MicroView
        displayBAC(bac_read);
            
        // Update LEDs with BAC level
        updateLevel();
        
        // Look for button push
        button_state = digitalRead(button_pin);
        if ( (button_state == LOW) && (button_prev == HIGH) ) {
            do_sound = false;
        }
        button_prev = button_state;
        
        delay(100);
    }
    
    // Shut up buzzer
    digitalWrite(buzzer_pin_1, LOW);
    
    // Post BAC to data.sparkfun
    postToPhant();
    
    // Send BAC level to MicroView
    displayBAC(bac_read);
    
    // Wait some time before demanding next BAC reading
    delay(TIME_WAIT);
}

// Display LEDs correspoding to BAC
void updateLevel() {
    if ( bac_read >= 0 ) {
        digitalWrite(led_level_0, HIGH);
    } else {
        digitalWrite(led_level_0, LOW);
    }
    if ( bac_read >= LED_LEVEL_0 ) {
        digitalWrite(led_level_1, HIGH);
    } else {
        digitalWrite(led_level_1, LOW);
    }
    if ( bac_read >= LED_LEVEL_1 ) {
        digitalWrite(led_level_2, HIGH);
    } else {
        digitalWrite(led_level_2, LOW);
    }
    if ( bac_read >= LED_LEVEL_2 ) {
        digitalWrite(led_level_3, HIGH);
    } else {
        digitalWrite(led_level_3, LOW);
    }
    if ( bac_read >= LED_LEVEL_3 ) {
        digitalWrite(led_level_4, HIGH);
    } else {
        digitalWrite(led_level_4, LOW);
    }
    if ( bac_read >= LED_LEVEL_4 ) {
        digitalWrite(led_level_5, HIGH);
    } else {
        digitalWrite(led_level_5, LOW);
    }
    if ( bac_read >= LED_LEVEL_5 ) {
        digitalWrite(led_level_6, HIGH);
    } else {
        digitalWrite(led_level_6, LOW);
    }
    if ( bac_read >= LED_LEVEL_6 ) {
        digitalWrite(led_level_7, HIGH);
    } else {
        digitalWrite(led_level_7, LOW);
    }
   
}

// Read BAC from sensor and display it
void readBAC() {
    
    // Read sensor value and convert to voltage (12-bit ADC)
    sensor_read = analogRead(sensor_pin);
    
    // Scale 12-bit ADC value to 10-bit resolution
    sensor_read = sensor_read >> 2;
    
    // We're using a 1/2 voltage divider to scale (0-5V) to (0-2.5V)
    sensor_read = sensor_read * 2;
    
    // Calculate ppm. Regression fitting from MQ-3 datasheet.
    // Equation using 5V max ADC and RL = 4.7k. "v" is voltage.
    // PPM = 150.4351049*v^5 - 2244.75988*v^4 + 13308.5139*v^3 - 
    //       39136.08594*v^2 + 57082.6258*v - 32982.05333
    // Calculate BAC. See BAC/ppm chart from page 2 of:
    // http://sgx.cdistore.com/datasheets/sgx/AN4-Using-MiCS-Sensors-for-Alcohol-Detection1.pdf
    // All of this was put into the lookup table in BAC_Lookup.h
    if ( (sensor_read < BAC_START) || (sensor_read > BAC_END) ) {
        bac_read = 0;
    } else {
        sensor_read = sensor_read - BAC_START;
        bac_read = bac_chart[sensor_read];
    }
}

// Parse 16-bit value into 2 bytes and send to MicroView
void displayBAC(uint16_t val) {
    
    uint8_t msg[4];
    
    // Set SOF and EOF
    msg[0] = MSG_SOF;
    msg[3] = MSG_EOF;
    
    // Set payload
    msg[1] = (uint8_t)(val >> 8);
    msg[2] = (uint8_t)(val & 0xFF);
    
    // Send message to display
    Serial1.write(MSG_SOF);
    Serial1.write(msg[1]);
    Serial1.write(msg[2]);
    Serial1.write(MSG_EOF);
}

// Post BAC to data.sparkfun
int postToPhant()
{
    char val_str[10];
    
	// Convert valu to a string (3 decimal places)
	if ( bac_read < 10 ) {
	    sprintf(val_str, "0.00%d", bac_read);
	} else if ( (bac_read >= 10) && (bac_read < 99) ) {
        sprintf(val_str, "0.0%d", bac_read);
	} else {
	    sprintf(val_str, "0.%d", bac_read);
	}
    
	// Add weight to appropriate field before posting to Phant
    phant.add(field, val_str);
	
	// Make a connection to data.sparkfun and post data
    TCPClient client;
    char response[512];
    int i = 0;
    int retVal = 0;
    
    if (client.connect(server, 80)) // Connect to the server
    {
		// phant.post() will return a string formatted as an HTTP POST.
		// It'll include all of the field/data values we added before.
		// Use client.print() to send that string to the server.
        client.print(phant.post());
        delay(1000);
		// Now we'll do some simple checking to see what (if any) response
		// the server gives us.
        while (client.available())
        {
            char c = client.read();
            //Serial.print(c);	// Print the response for debugging help.
            if (i < 512)
                response[i++] = c; // Add character to response string
        }
		// Search the response string for "200 OK", if that's found the post
		// succeeded.
        if (strstr(response, "200 OK"))
        {
            retVal = 1;
        }
        else if (strstr(response, "400 Bad Request"))
        {	// "400 Bad Request" means the Phant POST was formatted incorrectly.
			// This most commonly ocurrs because a field is either missing,
			// duplicated, or misspelled.
            retVal = -1;
        }
        else
        {
			// Otherwise we got a response we weren't looking for.
            retVal = -2;
        }
    }
    else
    {
        retVal = -3;
    }
    client.stop();	// Close the connection to server.
    delay(1000);    // Allow user to see message on OLED
    
    return retVal;	// Return error (or success) code.
}