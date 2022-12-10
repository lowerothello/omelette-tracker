typedef struct {
	unsigned long len;
	unsigned long ptr;
	unsigned long offset; /* significant offset */
	unsigned long siglen; /* significant length */
	float data[]; /* alloc(sizeof(float) * (len<<1)) */
} DelayBuffer;

DelayBuffer *allocDelayBuffer(unsigned long maxlen)
{
	DelayBuffer *ret = calloc(1, sizeof(DelayBuffer) + sizeof(float) * (maxlen<<1));

	ret->len = ret->siglen = maxlen;
	return ret;
}

void freeDelayBuffer(DelayBuffer *buf) { free(buf); } /* wow what a neccesary function */

/* set the delay buffer significant range */
/* .newsiglen must be <= buf->len */
/* reset by calling with .newsiglen as buf->len */
void setDelayBufferSig(DelayBuffer *buf, unsigned long newsiglen)
{
	buf->offset = (buf->offset + buf->ptr) % buf->len;
	buf->ptr = 0;
	buf->siglen = newsiglen;
}

void walkDelayBuffer(DelayBuffer *buf, float l, float r)
{
	buf->data[(((buf->offset + buf->ptr) % buf->len)<<1) + 0] = l;
	buf->data[(((buf->offset + buf->ptr) % buf->len)<<1) + 1] = r;

	buf->ptr++;
	if (buf->ptr >= buf->siglen) /* could be a while loop, doesn't really have to be though */
		buf->ptr -= buf->siglen;
}

/* .offset is how far in the past to read */
/* .offset must be < buf->len, otherwise segfault (the good shit, love a good SIGSEGV) */
float readDelayBuffer(DelayBuffer *buf, unsigned long offset, char channel)
{
	/* adding buf->len where it will always be eaten by the modulo is to prevent underflow */
	return buf->data[(((buf->len + buf->offset + buf->ptr - offset) % buf->len)<<1) + channel];
}
