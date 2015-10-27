#include <SdDataFifo.h>
#include <CloudMonitor24Config.h>
#include <SD.h>

// Local defines
#define QUEUE_FILE_NAME       "/queue.txt"
#define POINTER_FILE_NAME     "/pointer.txt"
#define CM24_SYNC_CHARACTER   0x55

//#define SD_DATA_FIFO_DEBUG_ON

SdDataFifo::SdDataFifo(String workingFolderPath)
{

  //Remove end slash if exist
  if (workingFolderPath.endsWith("/"))
  {
    workingFolderPath.remove( workingFolderPath.length() - 1 );
  }

  _queue_file = workingFolderPath + QUEUE_FILE_NAME;
  _pointer_file = workingFolderPath + POINTER_FILE_NAME;
  
  // Note: the actual mechanism to acquire time, cannot grant time to be equal between
  // one boot and the next one. We just remove and lose data because it probably would have a wrong timestamp.

  //TODO: find a way to check if SD.begin is already called (it can’t be called multiple times!)
  // Create directory if missing
  SD.mkdir((char *)workingFolderPath.c_str());    
  SD.remove((char *) _pointer_file.c_str());
  SD.remove((char *) _queue_file.c_str());
}

boolean SdDataFifo::push_data(byte *data, byte size)
{
  File dataFile = SD.open((char *)_queue_file.c_str(), FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile)
  {
    // TODO: rollback in case of write error!
    dataFile.write(CM24_SYNC_CHARACTER);
    dataFile.write(size);
    dataFile.write(data, size);
    dataFile.close();
    return true;
  }
  else
  {
    //Serial.println("SdDataFifo::push_data SD.open( "+_queue_file+" ) failed!");
    return false;
  }  
}

byte* SdDataFifo::pop_data(byte *size)
{
  uint32_t offset = get_last_sent();
  byte i;
  static byte retval[CM24_MAX_MESSAGE_SIZE];

  File dataFile = SD.open((char *)_queue_file.c_str(), FILE_READ);

  *size = 0;
  if (!dataFile) 
  {
    return NULL;
  }

  if (offset > 0)
  {
    dataFile.seek(offset);  
  }
  
  // Read sync character
  if (!dataFile.available())
  {
    dataFile.close();
    return NULL;
  }

  i = dataFile.read();
  if (i != CM24_SYNC_CHARACTER)
  {
    dataFile.close();
    increment_pointer(1);
    return NULL;
  }

  // Read size
  if (!dataFile.available())
  {
    dataFile.close();
    return NULL;
  }
  *size = dataFile.read();
  if (*size > CM24_MAX_MESSAGE_SIZE)
  {
    *size = 0;
    dataFile.close();
    return NULL; 
  }

  // Read data
  for(i=0; i<*size; i++)
  {
    if (!dataFile.available())
    {
      *size = 0;
      dataFile.close();
      return NULL;
    }

    retval[i] = dataFile.read();
  }
  
  dataFile.close();
  return retval;
}

//
// Returns 
//  - true  if we could advance the message pointer
//  - false if we have reached the end of the queue
//
boolean SdDataFifo::increment_pointer( byte size )
{
  unsigned long last_sent = get_last_sent();

  // Note: +2 because we save also a sync char and the size
  return set_last_sent(last_sent + size + 2);
}


/*************************
* private methods
**************************/

boolean SdDataFifo::set_last_sent(unsigned long offset) 
{

  File queueFile = SD.open((char *)_queue_file.c_str(), FILE_READ);
  if(queueFile)
  {

    //Reading queueFile length
    uint32_t queueSize = queueFile.size();
    queueFile.close();

    //dele old pointer file
    SD.remove((char *)_pointer_file.c_str());
    File pointerFile = SD.open((char *)_pointer_file.c_str(), FILE_WRITE);

    if (pointerFile)
    {

      //check if we reach the end of the frame_queue to send
      if(offset >= queueSize)
      {
        pointerFile.println("0");
        pointerFile.close();

        SD.remove((char *)_queue_file.c_str());
      }
      else
      {
        pointerFile.println(String(offset));
        pointerFile.close();
      }
      
      return true;
    }  
    else 
    {
      return false;
    }

  }
  else
  {
    return false;
  }
  
}

uint32_t SdDataFifo::get_last_sent()
{

  File pointerFile = SD.open((char *)_pointer_file.c_str(), FILE_READ);

  char line[20];
  int i=0;

  if (pointerFile)
  {
    while( pointerFile.available() )
    {
      line[i++] = pointerFile.read();
    }
    line[i++]='\0';

    pointerFile.close();
    return atol(line);
  }
  else
  {
    return 0;
  }
}