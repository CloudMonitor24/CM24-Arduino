#ifndef SdDataFifo_h //this prevents problems if someone accidently #include's your library twice
#define SdDataFifo_h

#include <Arduino.h>
#include <DataFifo.h>

class SdDataFifo : public DataFifo
{
  public:
    
    SdDataFifo(String sdCardFilePath);

    // Method overridden
    boolean push_data(byte *data, byte size);
   	byte* pop_data(byte *size);
	  boolean increment_pointer( byte size );

  private:

    boolean set_last_sent(unsigned long offset);
    uint32_t get_last_sent();

    String _queue_file;
    String _pointer_file;
};

#endif