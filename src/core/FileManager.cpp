#include "FileManager.h"
// FileManager fileManager;
void FileManager::init()
{
  if (!LittleFS.begin())
  {
    Serial.println("An error has occurred while mounting LittleLittleFS");
    return;
  }
  // Serial.println("File mounted successfully");
  listDir("/", 3); // List the directories up to one level beginning at the root directory
}

// Read File from LittleLittleFS
String FileManager::readFile(const char *path)
{
  Serial.printf("Reading file: %s\n", path);

  File file = LittleFS.open(path, "r");
  if (!file)
  {
    Serial.println("- Failed to open file for reading");
    return String();
  }

  String fileContent;
  while (file.available())
  {
    fileContent += (char)file.read();
  }
  file.close();
  return fileContent;
}

// Write file to LittleLittleFS
void FileManager::writeFile(const char *path, const char *message)
{
  // log_i("Writing file: %s\r\n", path);

  File file = LittleFS.open(path, "w");
  if (!file)
  {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message))
  {
    Serial.println("- file written");
  }
  else
  {
#define LOG_ERROR(message) Serial.println(message)
  }
  file.close();
}

void FileManager::listDir(const char *dirname, uint8_t levels)
{
  // log_i("Listing directory: %s\r\n", dirname);

  Dir dir = LittleFS.openDir(dirname);
  while (dir.next())
  {
    Serial.print("  FILE: ");
    Serial.print(dir.fileName());
    Serial.print("\tSIZE: ");
    Serial.println(dir.fileSize());
  }
}

void FileManager::createDir(const char *path)
{
  // log_i("Creating Dir: %s\n", path);
  if (LittleFS.mkdir(path))
  {
    // Serial.println("Dir created");
  }
  else
  {
    // Serial.println("mkdir failed");
  }
}

void FileManager::removeDir(const char *path)
{
  // log_i("Removing Dir: %s\n", path);
  if (LittleFS.rmdir(path))
  {
    // Serial.println("Dir removed");
  }
  else
  {
    // Serial.println("rmdir failed");
  }
}

void FileManager::appendFile(const char *path, const char *message)
{
  // log_i("Appending to file: %s\r\n", path);

  File file = LittleFS.open(path, LFS_O_APPEND);
  if (!file)
  {
    // Serial.println("- failed to open file for appending");
    return;
  }
  if (file.print(message))
  {
    // Serial.println("- message appended");
  }
  else
  {
    // Serial.println("- append failed");
  }
  file.close();
}