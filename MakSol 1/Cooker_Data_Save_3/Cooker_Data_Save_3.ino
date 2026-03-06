#include "Arduino.h"

#include "FS.h"
#include <LITTLEFS.h>

void setup() {
  Serial.begin(115200);
 if(!LITTLEFS.begin(true)){
          Serial.println("An Error has occurred while mounting File System");
      return;
    }
          createDir(LITTLEFS, "Data/");
          appendFile(LITTLEFS, "/Mak2_3.txt", "\r\nCooking Round: 5,  Cooking Duration: XYZ");
       
        Serial.println("Now Reading:"); 
        readFile(LITTLEFS, "/Mak2_3.txt");

}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\r\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("- failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("- message appended");
    } else {
        Serial.println("- append failed");
    }
    file.close();

     File storage = LITTLEFS.open("/Mak2_3.txt", FILE_APPEND);
    
    if(!storage) {Serial.print("Error Opening Storage!"); return;}
    else {
      if(storage.print("\r\nCooking Round: 6,  Cooking Duration: XYZ\r\n")){ Serial.print("Data Saved!");}
      else Serial.println("Data Save Failed!");
    }
  storage.close();

}

void createDir(fs::FS &fs, const char * path){
    Serial.printf("Creating Dir: %s\n", path);
    if(fs.mkdir(path)){
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}


void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return;
    }

    Serial.println("- read from file:");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}


void loop() {
  // put your main code here, to run repeatedly:

}
