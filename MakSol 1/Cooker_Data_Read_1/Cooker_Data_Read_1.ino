#include "Arduino.h"

#include "FS.h"
#include <LITTLEFS.h>




void setup() {
  // put your setup code here, to run once:
Serial.begin(115200);
 if(!LITTLEFS.begin(true)){
          Serial.println("An Error has occurred while mounting File System");
      return;
    }
  
Serial.println("Now Reading:"); 
  readFile(LITTLEFS, "/Mak2_3.txt");

  listDir(LITTLEFS, "/", 3);

}


void loop() { 
  // put your main code here, to run repeatedly:
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




void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}
