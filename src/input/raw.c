bool inputRawInit(void)
{
	if (strcmp(getenv("TERM"), "LINUX")) return 1;
	// ioctl(0, KDSKBMODE, K_RAW); /* TODO: */
	return 0;
}
void inputRawFree(void)
{
	// ioctl(0, KDSKBMODE, K_XLATE); /* TODO: */
}

const InputAPI raw_input_api =
{
	inputRawInit,
	inputRawFree,
	NULL,
	NULL,
	1
};
