#include "Screen.h"
#include "Button.h"
#include "Encoder.h"
#include "SD_CARD.h"
#include "KeyPad.h"
#include "Bluetooth.h"
#include "Grbl.h"

//Pins
#define __ok 5
#define __back  2
#define __buzzer  7

#define Xp 22
#define Xn 23
#define Yp 24
#define Yn 25
#define Zp 26
#define Zn 27
#define EN 36

Screen new_s;
Encoder encoder;
SD_CARD sd;
Push_Button ok_btn (__ok, __buzzer);
Push_Button back_btn (__back, __buzzer);
Push_Button zp_btn (Zp, __buzzer);
Push_Button zn_btn (Zn, __buzzer);
Push_Button xp_btn (Xp, __buzzer);
Push_Button xn_btn (Xn, __buzzer);
Push_Button yp_btn (Yp, __buzzer);
Push_Button yn_btn (Yn, __buzzer);
KeyPad key_pad;
Bluetooth bt;
Grbl grbl;

bool __ok_status = LOW;
bool __back_status = LOW;
bool __xp_status = LOW;
bool __xn_status = LOW;
bool __yp_status = LOW;
bool __yn_status = LOW;
bool __zp_status = LOW;
bool __zn_status = LOW;

int choice;
int currentMenu = 0; //0-> home screen 1->main menu 2->jog men 3->config1menu 4->featuresmenu
int currentSubMenu = 0;
float axis_Increment_value = 1;
float x = 0.00 , y = 0.00 , z = 0.00;

int menu_select = 0; //0-> normal select 1->multi menu select , 2->sd card-menu select
String files_names;
String files_sizes;
int files_num;
int column , row;

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
void setup(void) {
  Serial.begin(115200);
  Serial1.begin(115200);
  digitalWrite(EN , LOW);
  new_s.spindle_status = false;
  new_s.bt_status = false;
  new_s.setCoordniates(x , y , z);
  new_s.sdCard_status = sd.sdAvailable();
  new_s.homePage();
  sd.getFilesData();
  key_pad.feedRate_P = new_s.feed_rate;
  key_pad.speed_p = new_s.spindle_speed;
  files_names = sd.files_names;
  files_sizes = sd.files_sizes;
  files_num = sd.getNumber();
  grbl.load_settings_values();
  attachInterrupt(digitalPinToInterrupt(3), select , HIGH);
  attachInterrupt(digitalPinToInterrupt(__back), back , FALLING );
  Serial.println("Setup finished");
}

void loop() {
  new_s.homePage();
  nav();
  while (currentMenu == 1) {
    new_s.mainMenu();
    new_s.setSelection(choice);
    if (!ok_btn.get_current_status()) {
      mainMenuSelect();
    }
  }
}
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

void nav() {
  __ok_status = ok_btn.get_current_status();
  if (!__ok_status) {
    currentMenu++;
  }
}

void goBack() {
  switch (currentMenu) {
    case 1:
      new_s.homePage();
      currentMenu = 0;
      break;
    case 2:
      new_s.mainMenu();
      currentMenu = 1;
      break;
    case 3:
      new_s.mainMenu();
      currentMenu = 1;
      break;
    case 4:
      new_s.mainMenu();
      currentMenu = 1;
      break;
  }
  choice = 0;
  encoder.resetCounter();
}

void back() {
  __back_status = back_btn.get_current_status();
  if (!__back_status) {
    if (currentSubMenu ==  1 || currentSubMenu ==  2) {
      new_s.jogMenu();
      currentSubMenu = 0;
    } if (currentSubMenu ==  3 || currentSubMenu ==  4 || currentSubMenu ==  8) {
      new_s.features();
      currentSubMenu = 0;
    } if (currentSubMenu ==  5 || currentSubMenu ==  6 || currentSubMenu ==  7) {
      new_s.mcConfig();
      currentSubMenu = 0;
    }
    else {
      goBack();
    }
  }
}

void jogging_submenu_go() {
  switch (choice) {
    case 1:
      currentSubMenu = 1;
      while (currentSubMenu == 1) {
        new_s.moveAxis();
        getAxis();
      }
      break;
    case 2:
      grbl.homing();
      x = 0;
      y = 0;
      z = 0;
      new_s.setCoordniates(x , y , z);
      new_s.homePage();
      delay(5000);
      currentMenu = 0;
      break;
    case 3:
      currentSubMenu = 2;
      while (currentSubMenu == 2) {
        new_s.setOrigin();
        getAxis();
        __ok_status = ok_btn.get_current_status();
        if (!__ok_status) {
          //Send the new origin comand to grbl
          x = 0;
          y = 0;
          z = 0;
          new_s.setCoordniates(x , y , z);
          new_s.homePage();
          currentMenu = 0;
          break;
        }
      }
      break;
  }
}

void features_submenu() {
  switch (choice) {
    case 1:
      if (!new_s.bt_status) {
        currentSubMenu = 3;
        while (currentSubMenu == 3) {
          sd_card_Menu();
          menu_select = 2;
        }
        menu_select = 0;
      }
      break;
    case 2:
      if (!ok_btn.get_current_status()) {
        if (new_s.bt_status) {
          new_s.bt_status = false;
          bt.bt_close();
        } else {
          new_s.bt_status = true;
          bt.bt_open();
        }
      }
      break;
    case 3:
      currentSubMenu = 8;
      while (currentSubMenu == 8) {
        new_s.about();
      }
      break;
  }
}

void sd_card_Menu() {
  new_s.sdFiles(files_names, files_num);
  new_s.sd_setSelection(row , column);
  if (!ok_btn.get_current_status()) {
    int file_number = encoder.sd_choice;
    String file_name = sd.getFileName(file_number);
    Serial.println(file_number);
    Serial.println(file_name);
    sd.openFile(file_name);
    sd.loadCurrentFile();
    sd.closeFile();
  }
}

void spindle_select() {
  switch (choice) {
    case 1:
      if (!ok_btn.get_current_status()) {
        if (new_s.spindle_status) {
          new_s.spindle_status = false;
          //turn spindle on (motor driver)
        } else {
          new_s.spindle_status = true;
          //turn spindle off (motor driver)
        }
      }
      break;
    case 2:
      if (ok_btn.get_current_status()) {
        new_s.spindle_speed = key_pad.getData_speed();
        key_pad.speed_p = new_s.spindle_speed;
        //send speed to Driver
      }
      break;
    case 3:
      if (!ok_btn.get_current_status()) {
        if (new_s.spindle_direction) {
          new_s.spindle_direction = 0;
          //spindle cw (motor driver)
        } else {
          new_s.spindle_direction = 1;
          //spindle ccw (motor driver)
        }
      }
      break;
  }
}
void settings_grbl() {
  new_s.grblSettings();
  new_s.grbl_settings = key_pad.getData("---");
  if (!ok_btn.get_current_status()) {
    settings_grbl_new();
  }
}

void settings_grbl_new() {
  key_pad.data = "";
  while (grbl.settings_exist(new_s.grbl_settings)) {
    new_s.grblSettings();
    new_s.grbl_value = grbl.get_grbl_value(grbl.get_settings_number(new_s.grbl_settings));
    new_s.grbl_value = key_pad.getData(new_s.grbl_value);
    //set data
    if (!ok_btn.get_current_status() || !back_btn.get_current_status()) {
      key_pad.data = "";
      new_s.grbl_settings = "---";
      new_s.grbl_value = "---";
      settings_grbl_new();
    }
  }
}
void config_submenu() {
  switch (choice) {
    case 1:
      currentSubMenu = 5;
      while (currentSubMenu == 5) {
        settings_grbl();
      }
      key_pad.data = "";
      break;
    case 2:
      currentSubMenu = 6;
      while (currentSubMenu == 6) {
        new_s.feedRate();
        if (ok_btn.get_current_status()) {
          new_s.feed_rate = key_pad.getData_feedRate();
          key_pad.feedRate_P = new_s.feed_rate;
          //send Feed rate to grbl
        }
      }
      key_pad.data = "";
      break;
    case 3:
      currentSubMenu = 7;
      encoder.resetCounter();
      encoder.resetChoice();
      choice = 0;
      while (currentSubMenu == 7) {
        new_s.spindleSettings();
        new_s.setSelection(choice);
        spindle_select();
      }
      break;
  }
}

void currentselect() {
  encoder.currentSelection();
  choice = encoder.getChoice();
}


void mainMenuSelect() {
  switch (choice) {
    case 1:
      if (!new_s.bt_status) {
        encoder.resetChoice();
        choice = 0;
        currentMenu = 2;
        while (currentMenu == 2) {
          new_s.jogMenu();
          new_s.setSelection(choice);
          if (!ok_btn.get_current_status()) {
            jogging_submenu_go();
            new_s.setSelection(choice);
          }
        }
      }
      break;
    case 2:
      if (!new_s.bt_status) {
        encoder.resetCounter();
        encoder.resetChoice();
        currentMenu = 3;
        choice = 0;
        while (currentMenu == 3) {
          menu_select = 1;
          new_s.mcConfig();
          new_s.setSelection(choice);
          if (!ok_btn.get_current_status()) {
            config_submenu();
            new_s.setSelection(choice);
          }
        }
      }
      break;
    case 3:
      encoder.resetCounter();
      encoder.resetChoice();
      choice = 0;
      currentMenu = 4;
      while (currentMenu == 4) {
        new_s.features();
        new_s.setSelection(choice);
        if (!ok_btn.get_current_status()) {
          features_submenu();
          new_s.setSelection(choice);
        }
      }
      break;
  }
}

void select() {
  if (menu_select == 1) {
    encoder.multiSelection();
    choice = encoder.getChoice();
  } else if (menu_select == 0) {
    currentselect();
  } else if (menu_select == 2) {
    encoder.sd_select();
    row = encoder.sd_row;
    column = encoder.sd_column;
  }
}

void getAxis() {
  __xp_status = xp_btn.get_current_status();
  __xn_status = xn_btn.get_current_status();
  __yp_status = yp_btn.get_current_status();
  __yn_status = yn_btn.get_current_status();
  __zp_status = zp_btn.get_current_status();
  __zn_status = zn_btn.get_current_status();
  if (!__xp_status) {
    x += axis_Increment_value;
  } else if (!__xn_status) {
    x -= axis_Increment_value;
  }
  if (!__yp_status) {
    y += axis_Increment_value;
  } else if (!__yn_status) {
    y -= axis_Increment_value;
  }
  if (!__zp_status) {
    z += axis_Increment_value;
  } else if (!__zn_status) {
    z -= axis_Increment_value;
  }
  new_s.setCoordniates(x , y , z);
}
