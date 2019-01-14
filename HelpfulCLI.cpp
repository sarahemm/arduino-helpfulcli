/*
  HelpfulCLI.cpp - Arduino library for building a simple serial CLI - implementation
  Copyright (c) 2019 sen. Licensed under Creative Commons BY-NC-SA.
*/

// include this library's description file
#include "HelpfulCLI.h"

// Constructor /////////////////////////////////////////////////////////////////
// Function that handles the creation and setup of instances

// Description: Creates a variable of type HelpfulCLI.
// Syntax: HelpfulCLI()
// Parameter: none
// Returns: Instance of HelpfulCLI.
HelpfulCLI::HelpfulCLI(void) {
}

// Public Methods //////////////////////////////////////////////////////////////
// Functions available in Arduino sketches, this library, and other libraries

// Description: Initialize the HelpfulCLI system.
// Syntax: HelpfulCLIInstance.begin(speed);
// Parameter: speed - Speed to communicate at.
// Returns: nothing
void HelpfulCLI::begin(long speed, char *str_prompt) {
  Serial.begin(speed);
  prompt = malloc(strlen(str_prompt));
  strcpy(prompt, str_prompt);
  buffer[0] = (char)0;
  prompt_displayed = false;
}

// Description: Configure the specified digit to be font-decoded or raw
// Syntax: HelpfulCLIInstance.registerCommand(command, help);
// Parameter: command - command string to register
// Parameter: help - help for the command we're registering
// Returns: nothing
void HelpfulCLI::registerCommand(char *cmd, char *help, cli_callback callback) {
  struct cli_cmd *this_cmd;

  if(first_cmd.cmd == NULL) {
    this_cmd = &first_cmd;
  } else {
    this_cmd = (struct cli_cmd *)malloc(sizeof(struct cli_cmd));
    struct cli_cmd *last_cmd = findLastCommand(&first_cmd);
    last_cmd->next_cmd = this_cmd;
  }
  this_cmd->cmd = (char *)malloc(strlen(cmd) + 1);
  this_cmd->help = (char *)malloc(strlen(help) + 1);
  strcpy(this_cmd->cmd, cmd);
  strcpy(this_cmd->help, help);
  this_cmd->strings_in_flash = false;
  this_cmd->callback = callback;
  this_cmd->next_cmd = NULL;
}

void HelpfulCLI::registerCommand(const __FlashStringHelper *cmd, const __FlashStringHelper *help, cli_callback callback) {
  struct cli_cmd *this_cmd;

  if(first_cmd.cmd == NULL) {
    this_cmd = &first_cmd;
  } else {
    this_cmd = (struct cli_cmd *)malloc(sizeof(struct cli_cmd));
    struct cli_cmd *last_cmd = findLastCommand(&first_cmd);
    last_cmd->next_cmd = this_cmd;
  }
  this_cmd->f_cmd = cmd;
  this_cmd->f_help = help;
  this_cmd->strings_in_flash = true;
  this_cmd->callback = callback;  
  this_cmd->next_cmd = NULL;
}

// Description: Service the CLI and deal with any waiting commands.
// Syntax: HelpfulCLIInstance.service();
// Parameter: none
// Returns: nothing
void HelpfulCLI::service(void) {
  if(!prompt_displayed) {
    Serial.print(prompt);
    Serial.print(buffer);
    prompt_displayed = true;
  }
  if(Serial.available() == 0) return;
  
  while(Serial.available() != 0) {
    char new_char = Serial.read();
    switch(new_char) {
      case '\r':
        Serial.println("");
        executeCommand(String(buffer));
        buffer[0] = (char)0;
        prompt_displayed = false;
        break;
      case 0x7F: // backspace/DEL
        if(strlen(buffer) == 0) break;
        Serial.print("\b \b");
        buffer[strlen(buffer) - 1] = (char)0;
        break;
      case '?':
        Serial.println("?");
        displayHelp(String(buffer));
        prompt_displayed = false;
        break;
      default:
        buffer[strlen(buffer) + 1] = (char)0;
        buffer[strlen(buffer)] = new_char;
        Serial.print(new_char);
    }
  }
}

// Description: List all the commands registered. Useful for debugging.
// Syntax: HelpfulCLIInstance.listCommands();
// Parameter: none
// Returns: nothing
void HelpfulCLI::listCommands() {
  struct cli_cmd *this_cmd = &first_cmd;
  
  while(this_cmd != NULL) {
    Serial.print("Command: ");
    if(this_cmd->strings_in_flash) {
      Serial.println(this_cmd->f_cmd);
      Serial.print("Help: ");
      Serial.println(this_cmd->f_help);
    } else {
      Serial.println(this_cmd->cmd);
      Serial.print("Help: ");
      Serial.println(this_cmd->help);
    }
    Serial.print("Next: ");
    Serial.println((int)this_cmd->next_cmd, HEX);
    Serial.println("");
    this_cmd = this_cmd->next_cmd;
  }  
}

void HelpfulCLI::executeCommand(String buf) {
  struct cli_cmd *this_cmd = &first_cmd;
  if(buf.length() == 0) return;

  Serial.println("");

  String search_buf = stripNumbers(buf);
  while(this_cmd != NULL) {
    String cmd_str = this_cmd->strings_in_flash ? String(this_cmd->f_cmd) : String(this_cmd->cmd);
    if(search_buf.startsWith(cmd_str)) {
      byte extra_nbr_chars = buf.length() - stripNumbers(buf).length();
      signed long nbr_buf[MAX_NBR_ARGS];
      extractNumbers(buf, nbr_buf);
      (*this_cmd->callback)(buf.substring(0, cmd_str.length() + extra_nbr_chars), buf.substring(cmd_str.length() + extra_nbr_chars + 1), nbr_buf);
      return;
    }        
    
    this_cmd = this_cmd->next_cmd;
  }
  Serial.println("Unknown command.");
  Serial.println(buf);  
}

void HelpfulCLI::displayHelp(String buf) {
  Serial.println("");
  struct cli_cmd *this_cmd = &first_cmd;
  buf = stripNumbers(buf);
  while(this_cmd != NULL) {
    if(this_cmd->strings_in_flash) {
      if(buf.compareTo(String(this_cmd->f_cmd).substring(0, buf.length())) == 0) {
        Serial.print(this_cmd->f_cmd);
        Serial.print(" - ");
        Serial.println(this_cmd->f_help);
      }
    } else {
      if(buf.compareTo(String(this_cmd->cmd).substring(0, buf.length())) == 0) {
        Serial.print(this_cmd->cmd);
        Serial.print(" - ");
        Serial.println(this_cmd->help);
      }
    }
    this_cmd = this_cmd->next_cmd;
  }
}

struct cli_cmd *HelpfulCLI::findLastCommand(struct cli_cmd *start_cmd) {
  struct cli_cmd *this_cmd = start_cmd;
  
  while(this_cmd->next_cmd != NULL) {
    this_cmd = this_cmd->next_cmd;
  }
  
  return this_cmd;
}

// replace all numbers with a single #
String HelpfulCLI::stripNumbers(String str) {
  // TODO: this seems inelegant and could use cleaning up
  byte i;
  for(i=0; i<=9; i++) {
    str.replace(String(i), "#");
  }
  str.replace("-#", "#");
  for(i=0; i<=9; i++) {
    str.replace("##", "#");
  }
  
  return str;
}
 
// extract any numbers from the string into an array
void HelpfulCLI::extractNumbers(String buf, signed long nbrs[]) {
  byte i;

  byte cur_nbr = 0;
  bool currently_in_nbr;
  for(i=0; i<buf.length(); i++) {
    if(
      (buf.charAt(i) >= '0' && buf.charAt(i) <= '9') ||
      (i < buf.length() && buf.charAt(i) == '-' && buf.charAt(i+1) >= '0' && buf.charAt(i+1) <= '9')
    ) {
      if(currently_in_nbr) continue;
      currently_in_nbr = true;
      nbrs[cur_nbr++] = buf.substring(i).toInt();
    } else {
      currently_in_nbr = false;
    }
  }
}