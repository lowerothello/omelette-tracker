uint32_t pow32(uint32_t a, uint32_t b)
{
	if (!b) return 1;
	uint32_t c = a;
	for (uint32_t i = 1; i < b; i++)
		c *= a;
	return c;
}
