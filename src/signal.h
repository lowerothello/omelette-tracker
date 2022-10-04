/* communication between the main and process threads */

/* main semaphore */
/* communication is always initiated by the main thread */
#define M_SEM_OK           0 /* allow processing                            */
#define M_SEM_RELOAD_REQ   1 /* trigger downtime for a file reload          */
#define M_SEM_RELOAD       2 /* safe to reload the file                     */
#define M_SEM_SWAPINST_REQ 3 /* swap s->instrument and p->semarg atomically */
#define M_SEM_CALLBACK     4 /* ready to call p->semcallback()              */

/* channel semaphores */
#define C_SEM_OK 0

/* instrument semaphores */
#define I_SEM_OK 0
