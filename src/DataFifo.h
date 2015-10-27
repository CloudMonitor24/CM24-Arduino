#ifndef DataFifo_h //this prevents problems if someone accidently #include's your library twice
#define DataFifo_h

// NOTE: max value supported is 255!
#define CM24_MAX_MESSAGE_SIZE	14

#include <Arduino.h>

class DataFifo
{
  public:
    DataFifo();    

    virtual boolean push_data(byte *data, byte size);
   	virtual byte* pop_data(byte *size);
	virtual boolean increment_pointer( byte size );
};

#endif