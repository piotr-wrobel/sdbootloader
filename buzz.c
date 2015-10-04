#include "buzz.h"

void buzzDebug(uint8_t l_sygnalow, uint8_t s_sygnalow, uint8_t s_trwa)
{
	for(uint8_t i=0; i<l_sygnalow; i++)
	{
		BUZZ_PORT |= BUZZ;
		_delay_ms(500);
		BUZZ_PORT &=~ BUZZ;
		_delay_ms(500);
	}
	for(uint8_t i=0; i<s_sygnalow; i++)
	{
		BUZZ_PORT |= BUZZ;
		for(uint8_t j=0; j<s_trwa; j++)
			_delay_ms(1);
		BUZZ_PORT &=~ BUZZ;
		_delay_ms(200);
	}
}
