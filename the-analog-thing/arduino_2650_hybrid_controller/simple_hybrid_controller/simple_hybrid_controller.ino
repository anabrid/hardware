/* 
 *   This example implements a simple hybrid controller for use with THE ANALOG THING.
 *  Basically, there are three digital output control lines and four analog input
 *  lines to be taken care of:
 *  
 *  D2: ENABLE          Setting this to 0 enables hybrid mode, i.e. overrides the on-board
 *                      control logic of THE ANALOG THING. If set to 1, THE ANALOG THING
 *                      works as a stand-alone analog computer.
 *  D3: MIC             This controls the IC-control line of THE ANALOG THING.
 *  D4: MOP             This controls the OP-control line of THE ANALOG THING.
 *  A0/1/2/3: x/y/z/u   These are the level shifted output lines of THE ANALOG THING.
 *  
 *  13.10.2021  B. Ulmann Initial implementation
 */

#include <TimerThree.h>
#include <TimerFive.h>

#ifndef TRUE
#define TRUE 1
#define FALSE !TRUE
#endif

#define VERSION 0.1
#define BAUD_RATE           115200
#define STRING_LENGTH       81
#define SHORT_STRING_LENGTH 41
#define ENABLE              2         // Hybrid enable control line
#define MIC                 3         // MIC control line
#define MOP                 4         // MOP control line
#define IC                  0         // Mode IC
#define OP                  1         // Mode OP
#define HALT                2         // Mode HALT
#define DEFAULT_OP_TIME     1000      // Default OP-time in milliseconds
#define DEFAULT_IC_TIME     10        // Default IC-time in milliseconds
#define DEFAULT_CHANNELS    1         // Number of channels per default
#define MAX_CHANNELS        4         // Maximum number of ADC channels
#define MIN_INTERVAL        1         // Minimum sample interval in milliseconds

#define STATE_IDLE          0         // States of the control state machine
#define STATE_SR_START      1
#define STATE_SR_IC         2
#define STATE_SR_OP         3

unsigned int op_time = DEFAULT_OP_TIME,
  ic_time = DEFAULT_IC_TIME,
  state = STATE_IDLE,
  repop = FALSE,                      // True if repetitive operation is selected.
  channels = DEFAULT_CHANNELS,
  interval = MIN_INTERVAL,
  armed = FALSE;                      // True if data shall be logged during the next single run.

/*
** Local variant of strtok, just better. :-) The first call expects the string to be tokenized as its first argument.
** All subsequent calls only require the second argument to be set. If there is nothing left to be tokenized, a zero pointer
** will be returned. In contrast to strtok this routine will not alter the string to be tokenized since it
** operates on a local copy of this string.
*/
char *tokenize(char *string, char *delimiters) {
  static char local_copy[STRING_LENGTH], *position;
  char *token;

  if (string) { /* Initial call, create a copy of the string pointer */
    strcpy(local_copy, string);
    position = local_copy;
  } else { /* Subsequent call, scan local copy until a delimiter character will be found */
    while (*position && strchr(delimiters, *position)) /* Skip delimiters if there are any at the beginning of the string */
      position++;

    token = position; /* Now we are at the beginning of a token (or the end of the string :-) ) */

    if (*position == '\'') { /* Special case: Strings delimited by single quotes won't be split! */
      position++;
      while (*position && *position != '\'')
        position++;
    }

    while (*position) {
      position++;
      if (!*position || strchr(delimiters, *position)) { /* Delimiter found */
        if (*position)
          *position++ = (char) 0; /* Split string copy */
        return token;
      }
    }
  }

  return NULL;
}

void set_mode(int mode) {
  if (mode == IC) {
    digitalWrite(MIC, LOW);
    digitalWrite(MOP, HIGH);
  } else if (mode == OP) {
    digitalWrite(MIC, HIGH);
    digitalWrite(MOP, LOW);
  } else if (mode == HALT) {
    digitalWrite(MIC, HIGH);
    digitalWrite(MOP, HIGH);
  }
}

void sample_adc() {
  static unsigned int port[MAX_CHANNELS] = {A0, A1, A2, A3};

  for (unsigned int i = 0; i < channels; i++)
    Serial.print(String(analogRead(port[i])) + ";");
  Serial.print("\n");
}

void state_machine() {  // This implements the central state machine for run/rep-mode.
  unsigned int redo;
  
//  Serial.print("Repop = " + String(repop) + ", State = " + String(state) + "\n");

  if (state == STATE_IDLE) {
    Timer3.stop();
    repop = FALSE;
  }

  do {
    redo = FALSE;
    
    if (state == STATE_SR_START) {
      state = STATE_SR_IC;
      set_mode(IC);
      Timer3.initialize(1000 * (long) ic_time);
    } else if (state == STATE_SR_IC) {
      Timer3.stop();
      state = STATE_SR_OP;
      set_mode(OP);
            
      if (!repop && armed)  // Enable data logging if we are in single run mode and if the logger has been armed.
        Timer5.initialize(1000 * (long) interval);

      Timer3.initialize(1000 * (long) op_time); // Set Timer3 to the duration of the OP phase.
    } else if (state == STATE_SR_OP) {
      if (!repop) {
        Timer3.stop();
        Timer5.stop();
        armed = FALSE;
        set_mode(HALT);
        state = STATE_IDLE;
        Serial.print("Single run ended.\n");
      } else {
        state = STATE_SR_START;
        redo = TRUE;
      }
    }
  } while (redo);
}

void setup() {
  Serial.begin(BAUD_RATE);

  pinMode(ENABLE, OUTPUT);
  digitalWrite(2, HIGH);
  
  pinMode(MIC, OUTPUT);
  pinMode(MOP, OUTPUT);

  Timer3.stop();
  Timer3.attachInterrupt(state_machine);

  Timer5.stop();
  Timer5.attachInterrupt(sample_adc);
}

void loop() {
  char input[STRING_LENGTH], command[SHORT_STRING_LENGTH], value[SHORT_STRING_LENGTH];

  if (Serial.available() > 0) { // Process command
    Serial.readString().toCharArray(input, STRING_LENGTH);
    
    int last = strlen(input);
    if (input[last - 1] == '\r' or input[last - 1] == '\n')
      input[last - 1] = (char) 0;
      
    tokenize(input, (char *) 0);
    strcpy(command, tokenize((char *) 0, (char *) " ="));

    if (!strcmp(command, "arm")) {
      armed = TRUE;
      Serial.print("Armed...\n");
    } else if (!strcmp(command, "channels")) {
      strcpy(value, tokenize((char *) 0, (char *) "="));
      unsigned int i = atoi(value);
      if (i < 1 || i > MAX_CHANNELS)
        Serial.print("Illegal number of channels!\n");
      else {
        channels = i;
        Serial.print("channels=" + String(channels) + "\n");
      }
    } else if (!strcmp(command, "disable")) { // Disable hybrid mode
      digitalWrite(ENABLE, HIGH);
      Serial.print("Hybrid mode disabled.\n");
    } else if (!strcmp(command, "enable")) {  // Enable hybrid mode
      set_mode(IC);
      digitalWrite(ENABLE, LOW);
      Serial.print("Hybrid mode enabled.\n");
    } else if (!strcmp(command, "halt")) {
      set_mode(HALT);
      state = STATE_IDLE;
      armed = FALSE;
      Timer3.stop();
      Timer5.stop();
      Serial.print("HALT\n");
    } else if (!strcmp(command, "help")) {
      Serial.print("\nCommands:\n\
        arm                 Arm the data logger\n\
        channels=<value>    Set the number of channels to be logged\n\
        disable             Disable the hybrid controller\n\
        enable              Enable the hybrid controller\n\
        halt                Switch THAT into HALT mode\n\
        help                Print this help text\n\
        ic                  Switch THAT to IC mode\n\
        ictime=<value>      Set IC-time to value milliseconds\n\
        interval=<value>    Set the sampling interval to value ms\n\
        op                  Switch THAT to OP mode\n\
        optime=<value>      Set the OP-time to value milliseconds\n\
        rep                 Start repetitive operation\n\
        run                 Start a single run (with logging if armed)\n\
        status              Return status information\n\
        \n");
    } else if (!strcmp(command, "ic")) {
      set_mode(IC);
      state = STATE_IDLE;
      armed = FALSE;
      Timer3.stop();
      Timer5.stop();
      Serial.print("IC\n");
    } else if (!strcmp(command, "ictime")) {
      strcpy(value, tokenize((char *) 0, (char *) "="));
      Serial.print("ictime=" + String(ic_time = atoi(value)) + "\n");
    } else if (!strcmp(command, "interval")) {
      strcpy(value, tokenize((char *) 0, (char *) "="));
      unsigned int i = atoi(value);
      if (i < MIN_INTERVAL)
        Serial.print("Illegal sample interval, must be >= " + String(MIN_INTERVAL) + "\n");
      else {
        interval = i;
        Serial.print("interval=" + String(interval) + "\n");
      }
    } else if (!strcmp(command, "op")) {
      set_mode(OP);
      state = STATE_IDLE;
      armed = FALSE;
      Timer3.stop();
      Timer5.stop();
      Serial.print("OP\n");
    } else if (!strcmp(command, "optime")) {
      strcpy(value, tokenize((char *) 0, (char *) "="));
      Serial.print("optime=" + String(op_time = atoi(value)) + "\n");
    } else if (!strcmp(command, "rep")) {
      state = STATE_SR_START;
      repop = TRUE;
      Serial.print("Repetitive operation...\n");
      state_machine();
    } else if (!strcmp(command, "run")) {
      state = STATE_SR_START;
      repop = FALSE;
      Serial.print("Single run...\n");
      state_machine();
    } else if (!strcmp(command, "status")) {
      Serial.print("version=" + String(VERSION) + 
                   ",optime=" + String(op_time) + 
                   ",ictime=" + String(ic_time) + 
                   ",channels=" + String(channels) + 
                   ",armed=" + String(armed) + 
                   ",interval=" + String(interval) + 
                   "\n");
    } else 
      Serial.print("Illegal command >>> " + String(command) + "\n");
  }
}
