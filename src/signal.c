/* communication between the main and process threads */
/* just a bunch of defines rn */

/* main semaphore */
#define M_SEM_OK         0 /* allow processing */
#define M_SEM_RELOAD_REQ 1 /* trigger downtime for a file reload */
#define M_SEM_RELOAD     2 /* safe to reload the file            */
#define M_SEM_SWAPINST_REQ 3  /* swap s->instrumentv and p->semarg atomically   */
#define M_SEM_SWAPINST_DONE 4 /* s->instrumentv and p->semarg have been swapped */

/* channel semaphores */
#define C_SEM_OK 0

/* instrument semaphores */
#define I_SEM_OK 0
