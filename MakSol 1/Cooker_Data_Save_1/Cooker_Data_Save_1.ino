#include <Arduino.h>
#include "FS.h"
#include <LITTLEFS.h>

/* You only need to format LITTLEFS the first time you run a
   test or else use the LITTLEFS plugin to create a partition
   https://github.com/lorol/arduino-esp32littlefs-plugin
   
   If you test two partitions, you need to use a custom
   partition.csv file, see in the sketch folder */

//#define TWOPART

#define FORMAT_LITTLEFS_IF_FAILED true


void setup() {
   Serial.begin(115200);

#ifdef TWOPART
    if(!LITTLEFS.begin(FORMAT_LITTLEFS_IF_FAILED, "/lfs2", 5, "part2")){
    Serial.println("part2 Mount Failed");
    return;
    }
    appendFile(LITTLEFS, "/hello0.txt", "World0!\r\n");
    readFile(LITTLEFS, "/hello0.txt");
    LITTLEFS.end();

    Serial.println( "Done with part2, work with the first lfs partition..." );
#endif

    if(!LITTLEFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
        Serial.println("LITTLEFS Mount Failed");
        return;
    }
  
  Serial.println("Creating Directory: ");
      createDir(LITTLEFS, "/cooktime");
    listDir(LITTLEFS, "/", 1);
    listDir(LITTLEFS, "/", 3);
    
    //Serial.println("Writing to File File: ");
   //writeFile(LITTLEFS, "/day2/cookdata.txt", "5 hours, 2 hours, 3 hours");
    
  

Serial.println(); Serial.println("Appending to File: ");
    appendFile(LITTLEFS, "/cooktime/cookdata.txt", "77 hours for 3 years!\r\n");

Serial.println(); Serial.println("Reading File: ");
    readFile(LITTLEFS, "/day2/cookdata.txt");


  
    Serial.println( "Test complete" ); 

}

void loop() {
  // put your main code here, to run repeatedly:

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

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("- file written");
    } else {
        Serial.println("- write failed");
    }
    file.close();
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
}

void writeFile2(fs::FS &fs, const char * path, const char * message){
    if(!fs.exists(path)){
    if (strchr(path, '/')) {
            Serial.printf("Create missing folders of: %s\r\n", path);
      char *pathStr = strdup(path);
      if (pathStr) {
        char *ptr = strchr(pathStr, '/');
        while (ptr) {
          *ptr = 0;
          fs.mkdir(pathStr);
          *ptr = '/';
          ptr = strchr(ptr+1, '/');
        }
      }
      free(pathStr);
    }
    }

    Serial.printf("Writing file to: %s\r\n", path);
    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("- file written");
    } else {
        Serial.println("- write failed");
    }
    file.close();
}
