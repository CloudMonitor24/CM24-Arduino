#include <DataFifo.h>


DataFifo::DataFifo()
{

}

boolean DataFifo::push_data(byte *data, byte size)
{
	return false;
}
   	
byte* DataFifo::pop_data(byte *size)
{
	*size = 0;
	return NULL;
}
	
boolean DataFifo::increment_pointer( byte size )
{
	return false;
}