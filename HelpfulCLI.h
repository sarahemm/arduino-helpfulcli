/*
  HelpfulCLI.h - Arduino library for building a simple serial CLI - description
  Copyright (c) 2019 sen.  Licensed under Creative Commons BY-SA.
*/

// ensure this library description is only included once
#ifndef HELPFULCLI_h
#define HELPFULCLI_h

#if ARDUINO < 100
  #error This library requires Arduino 1.0 or later.  Please upgrade your development environment.
#else // ARDUINO < 100

// include core Arduino API
#include "Arduino.h"

#define MAX_CMDLINE 32

typedef void (*cli_callback)(String);
struct cli_cmd {
  union {
    char *cmd;
    const __FlashStringHelper *f_cmd;
  };
  union {
    char *help;
    const __FlashStringHelper *f_help;
  };
  bool strings_in_flash;
  cli_callback callback;
  struct cli_cmd *next_cmd;
};

// library interface description
class HelpfulCLI {
  // user-accessible "public" interface
  public:
    HelpfulCLI(void);
    void begin(long, char *);
    void registerCommand(char *, char *, cli_callback);
    void registerCommand(const __FlashStringHelper *, const __FlashStringHelper *, cli_callback);
    void service(void);
    void listCommands(void);
    
  // library-accessible "private" interface
  private:
    struct cli_cmd *findLastCommand(struct cli_cmd *);
    void executeCommand(String);
    void displayHelp(String);
    struct cli_cmd first_cmd;
    char *prompt;
    bool prompt_displayed;
    char buffer[MAX_CMDLINE];
};

#endif // ARDUINO < 100
#endif // HELPFULCLI_h
